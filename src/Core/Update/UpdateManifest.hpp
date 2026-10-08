#pragma once
// UpdateManifest: pure manifest logic (no network, no threads).
// Shared by ASTRA.exe, Updater.exe and the regression test.
//
// Manifest schema (docs/UPDATES.md):
//   {
//     "version": "1.1.0",            // required, strict semver triple
//     "url": "https://.../ASTRA-1.1.0.exe",
//     "sha256": "<64 hex chars>",
//     "signature": "<base64 RSA-PKCS1v15-SHA256 over payload below>",
//     "mandatory": false,            // optional, default false
//     "min_version": "1.0.0",        // optional; below it => mandatory
//     "status": "Disponivel",        // optional service status
//     "notes": "..."                 // optional changelog
//   }
// Signature payload (canonical bytes): version + "\n" + sha256 + "\n" + url
#include <Core/Update/UpdateCrypto.hpp>
#include <Security/Api/json.hpp>

#include <cctype>
#include <string>
#include <vector>

namespace Update {

struct SemVer {
    int major = -1, minor = -1, patch = -1;
    bool valid = false;

    static SemVer Parse(const std::string& s) {
        SemVer v;
        size_t p1 = s.find('.'), p2 = std::string::npos;
        if (p1 == std::string::npos) return v;
        p2 = s.find('.', p1 + 1);
        if (p2 == std::string::npos || s.find('.', p2 + 1) != std::string::npos) return v;
        auto part = [](const std::string& t, int& out) -> bool {
            if (t.empty() || t.size() > 5) return false;
            for (char c : t)
                if (!isdigit((unsigned char)c)) return false;
            if (t.size() > 1 && t[0] == '0') return false; // no leading zeros
            out = 0;
            for (char c : t) out = out * 10 + (c - '0');
            return out <= 99999;
        };
        if (!part(s.substr(0, p1), v.major)) return v;
        if (!part(s.substr(p1 + 1, p2 - p1 - 1), v.minor)) return v;
        if (!part(s.substr(p2 + 1), v.patch)) return v;
        v.valid = true;
        return v;
    }

    // -1: this < o, 0: equal, +1: this > o. Invalid versions sort below all.
    int Compare(const SemVer& o) const {
        if (!valid && !o.valid) return 0;
        if (!valid) return -1;
        if (!o.valid) return 1;
        if (major != o.major) return major < o.major ? -1 : 1;
        if (minor != o.minor) return minor < o.minor ? -1 : 1;
        if (patch != o.patch) return patch < o.patch ? -1 : 1;
        return 0;
    }

    std::string Str() const {
        return valid ? std::to_string(major) + "." + std::to_string(minor) + "." +
                           std::to_string(patch)
                     : std::string();
    }
};

// HTTPS-only URL gate. Rejects credentials, non-default ports are allowed
// only over TLS; never trust file://, http:// or UNC paths.
inline bool IsAllowedHttpsUrl(const std::string& url, std::string* hostOut = nullptr) {
    if (url.rfind("https://", 0) != 0) return false;
    size_t hostBeg = 8;
    size_t hostEnd = url.find_first_of("/?#", hostBeg);
    std::string host = url.substr(hostBeg, hostEnd == std::string::npos
                                               ? std::string::npos
                                               : hostEnd - hostBeg);
    if (host.empty() || host.size() > 253) return false;
    if (host.find('@') != std::string::npos) return false; // no userinfo
    if (host.find('\\') != std::string::npos) return false;
    if (host == "." || host == "..") return false;
    for (char c : host) {
        if (!(isalnum((unsigned char)c) || c == '.' || c == '-' || c == ':' ||
              c == '[' || c == ']'))

            return false;
    }
    if (hostOut) *hostOut = host;
    return true;
}

struct Manifest {
    SemVer version;
    SemVer minVersion; // invalid => no floor
    std::string url;
    std::string sha256;    // lowercase hex, 64 chars
    std::string signature; // base64
    bool mandatory = false;
    std::string status; // "Disponivel" | "Manutencao" | "Indisponivel" | ""
    std::string notes;
    bool ok = false;
    std::string error;

    static std::string SignaturePayload(const std::string& version,
            const std::string& sha256, const std::string& url) {
        return version + "\n" + sha256 + "\n" + url;
    }

    std::string Payload() const { return SignaturePayload(version.Str(), sha256, url); }

    static Manifest Parse(const std::string& body, size_t sizeCap = 65536) {
        Manifest m;
        if (body.empty() || body.size() > sizeCap) {
            m.error = "Empty or oversized manifest.";
            return m;
        }
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(body);
        } catch (...) {
            m.error = "Manifest is not valid JSON.";
            return m;
        }
        if (!j.is_object()) {
            m.error = "Manifest root must be an object.";
            return m;
        }
        auto req = [&](const char* key, std::string& out) -> bool {
            auto it = j.find(key);
            if (it == j.end() || !it->is_string()) {
                m.error = std::string("Manifest is missing '") + key + "'.";
                return false;
            }
            out = it->get<std::string>();
            if (out.empty() || out.size() > 2048) {
                m.error = std::string("Manifest field '") + key + "' has a bad size.";
                return false;
            }
            return true;
        };
        std::string verStr;
        if (!req("version", verStr) || !req("url", m.url) || !req("sha256", m.sha256) ||
            !req("signature", m.signature))
            return m;
        m.version = SemVer::Parse(verStr);
        if (!m.version.valid) {
            m.error = "Manifest has an invalid version.";
            return m;
        }
        if (!IsAllowedHttpsUrl(m.url)) {
            m.error = "Manifest URL must be HTTPS.";
            return m;
        }
        if (m.sha256.size() != 64) {
            m.error = "Manifest sha256 must be 64 hex characters.";
            return m;
        }
        std::vector<unsigned char> hashBytes;
        if (!Crypto::HexToBytes(m.sha256, hashBytes) || hashBytes.size() != 32) {
            m.error = "Manifest sha256 is not valid hex.";
            return m;
        }
        for (char& c : m.sha256) c = (char)tolower((unsigned char)c);
        if (auto it = j.find("mandatory"); it != j.end()) {
            if (!it->is_boolean()) {
                m.error = "Manifest 'mandatory' must be boolean.";
                return m;
            }
            m.mandatory = it->get<bool>();
        }
        if (auto it = j.find("min_version"); it != j.end()) {
            if (!it->is_string()) {
                m.error = "Manifest 'min_version' must be a string.";
                return m;
            }
            m.minVersion = SemVer::Parse(it->get<std::string>());
            if (!m.minVersion.valid) {
                m.error = "Manifest has an invalid min_version.";
                return m;
            }
        }
        if (auto it = j.find("status"); it != j.end()) {
            if (!it->is_string()) {
                m.error = "Manifest 'status' must be a string.";
                return m;
            }
            m.status = it->get<std::string>();
            if (m.status.size() > 32) m.status.resize(32);
        }
        if (auto it = j.find("notes"); it != j.end() && it->is_string()) {
            m.notes = it->get<std::string>();
            if (m.notes.size() > 2048) m.notes.resize(2048);
        }
        m.ok = true;
        return m;
    }

    // Cryptographic authentication: signature over the canonical payload.
    // The embedded public key is the ONLY trust root — the hash from the
    // same server is never trusted on its own.
    bool VerifySignature() const {
        if (!ok) return false;
        std::vector<unsigned char> sig;
        if (!Crypto::Base64Decode(signature, sig)) return false;
        size_t expLen = 0, modLen = 0;
        const unsigned char* exp = UpdatePublicKey::Exponent(expLen);
        const unsigned char* mod = UpdatePublicKey::Modulus(modLen);
        const std::string payload = Payload();
        return Crypto::RsaPkcs1Sha256Verify(
            mod, modLen, exp, expLen, payload.data(), payload.size(),
            sig.data(), sig.size());
    }
};

} // namespace Update
