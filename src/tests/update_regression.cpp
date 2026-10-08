#include <Core/Update/UpdateConfig.hpp>
#include <Core/Update/UpdateCrypto.hpp>
#include <Core/Update/UpdateManifest.hpp>
#include <iostream>
#include <stdexcept>

static void Check(bool condition, const char* name) {
    if (!condition) throw std::runtime_error(name);
    std::cout << "PASS: " << name << '\n';
}

struct UpdateRegression {
    static void SemVer() {
        auto v = Update::SemVer::Parse("1.2.3");
        Check(v.valid && v.major == 1 && v.minor == 2 && v.patch == 3, "semver parses triple");
        Check(!Update::SemVer::Parse("1.2").valid, "semver rejects partial");
        Check(!Update::SemVer::Parse("1.2.3.4").valid, "semver rejects quad");
        Check(!Update::SemVer::Parse("v1.2.3").valid, "semver rejects v prefix");
        Check(!Update::SemVer::Parse("1.02.3").valid, "semver rejects leading zeros");
        Check(!Update::SemVer::Parse("1.2.x").valid, "semver rejects non-numeric");
        Check(!Update::SemVer::Parse("").valid, "semver rejects empty");
        Check(Update::SemVer::Parse("1.10.0").Compare(Update::SemVer::Parse("1.9.9")) > 0,
            "semver compares numerically, not lexicographically");
        Check(Update::SemVer::Parse("2.0.0").Compare(Update::SemVer::Parse("2.0.0")) == 0,
            "semver equality");
        Check(Update::SemVer::Parse("1.0.0").Compare(Update::SemVer::Parse("1.0.1")) < 0,
            "semver orders patch");
        Check(Update::SemVer::Parse("bogus").Compare(Update::SemVer::Parse("0.0.1")) < 0,
            "invalid versions sort below everything");
    }

    static void Urls() {
        Check(Update::IsAllowedHttpsUrl("https://example.com/a.exe"), "https url allowed");
        Check(!Update::IsAllowedHttpsUrl("http://example.com/a.exe"), "http rejected");
        Check(!Update::IsAllowedHttpsUrl("file:///C:/a.exe"), "file scheme rejected");
        Check(!Update::IsAllowedHttpsUrl("https://user:pass@example.com/a.exe"), "userinfo rejected");
        Check(!Update::IsAllowedHttpsUrl("https://"), "empty host rejected");
        Check(!Update::IsAllowedHttpsUrl(""), "empty url rejected");
        Check(!Update::IsAllowedHttpsUrl("https://example.com\\..\\a.exe"), "backslash rejected");
    }

    static void Manifest() {
        const char* good = R"({
            "version": "1.1.0",
            "url": "https://example.com/ASTRA-1.1.0.exe",
            "sha256": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "signature": "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
            "mandatory": true,
            "min_version": "1.0.0",
            "status": "Disponivel",
            "notes": "Bug fixes."
        })";
        Update::Manifest m = Update::Manifest::Parse(good);
        Check(m.ok, "valid manifest parses");
        Check(m.version.Str() == "1.1.0", "manifest version parsed");
        Check(m.mandatory, "manifest mandatory parsed");
        Check(m.minVersion.Str() == "1.0.0", "manifest min_version parsed");
        Check(m.status == "Disponivel", "manifest status parsed");
        Check(m.Payload() == "1.1.0\nba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\nhttps://example.com/ASTRA-1.1.0.exe",
            "signature payload is canonical");
        // A forged/garbage signature must never verify against the embedded key.
        Check(!m.VerifySignature(), "forged signature never verifies");

        Check(!Update::Manifest::Parse("not json").ok, "garbage rejected");
        Check(!Update::Manifest::Parse("{}").ok, "empty object rejected");
        Check(!Update::Manifest::Parse(
            R"({"version":"bad","url":"https://example.com/a.exe","sha256":"00","signature":"AA=="})").ok,
            "invalid version rejected");
        Check(!Update::Manifest::Parse(
            R"({"version":"1.0.1","url":"http://example.com/a.exe","sha256":"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","signature":"AA=="})").ok,
            "http url rejected");
        // Downgrade protection data: installed newer than manifest.
        auto installed = Update::SemVer::Parse(Update::InstalledVersionStr());
        Check(installed.valid, "installed version is valid semver");
        Check(installed.Compare(m.version) < 0, "fixture manifest is newer than installed");
    }

    static void Hashing() {
        unsigned char digest[32];
        const char* abc = "abc";
        Check(Update::Crypto::Sha256(abc, 3, digest), "sha256 computes");
        Check(Update::Crypto::HexOf(digest, 32) ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "sha256 matches FIPS-180 test vector");
        std::vector<unsigned char> bytes;
        Check(Update::Crypto::HexToBytes("00ff", bytes) && bytes.size() == 2 &&
                bytes[0] == 0 && bytes[1] == 0xFF,
            "hex decode works");
        Check(!Update::Crypto::HexToBytes("zz", bytes), "bad hex rejected");
        // Base64 round-trip through the OS codec used for signatures.
        const unsigned char raw[] = { 0x01, 0x02, 0xFF, 0x00, 0x7F };
        std::string enc = Update::Crypto::Base64Encode(raw, sizeof(raw));
        std::vector<unsigned char> dec;
        Check(!enc.empty() && Update::Crypto::Base64Decode(enc, dec) &&
                dec.size() == sizeof(raw) &&
                memcmp(dec.data(), raw, sizeof(raw)) == 0,
            "base64 round-trips");
        Check(!Update::Crypto::Base64Decode("!!!", dec), "bad base64 rejected");
    }

    // Proves the BCrypt RSA PKCS#1 v1.5 + SHA-256 verify path end to end with
    // an ephemeral keypair (no secrets in the repo): sign with the private
    // half, verify with the exported public half, then verify tampering fails.
    static void RsaRoundTrip() {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        Check(BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, nullptr, 0)),
            "rsa provider opens");
        Check(BCRYPT_SUCCESS(BCryptGenerateKeyPair(hAlg, &hKey, 2048, 0)), "rsa key generates");
        Check(BCRYPT_SUCCESS(BCryptFinalizeKeyPair(hKey, 0)), "rsa key finalizes");

        const char* msg = "1.1.0\nabc123\nhttps://example.com/ASTRA-1.1.0.exe";
        unsigned char digest[32];
        Check(Update::Crypto::Sha256(msg, strlen(msg), digest), "payload hashes");

        BCRYPT_PKCS1_PADDING_INFO pad{};
        pad.pszAlgId = BCRYPT_SHA256_ALGORITHM;
        DWORD sigLen = 0;
        Check(BCRYPT_SUCCESS(BCryptSignHash(hKey, &pad, digest, sizeof(digest),
                    nullptr, 0, &sigLen, BCRYPT_PAD_PKCS1)),
            "signature size queries");
        std::vector<unsigned char> sig(sigLen);
        DWORD got = 0;
        Check(BCRYPT_SUCCESS(BCryptSignHash(hKey, &pad, digest, sizeof(digest),
                    sig.data(), sigLen, &got, BCRYPT_PAD_PKCS1)),
            "payload signs");

        // Export the public half in BCRYPT_RSAPUBLIC_BLOB layout.
        DWORD blobLen = 0;
        Check(BCRYPT_SUCCESS(BCryptExportKey(hKey, nullptr, BCRYPT_RSAPUBLIC_BLOB,
                    nullptr, 0, &blobLen, 0)),
            "public blob size queries");
        std::vector<unsigned char> blob(blobLen);
        Check(BCRYPT_SUCCESS(BCryptExportKey(hKey, nullptr, BCRYPT_RSAPUBLIC_BLOB,
                    blob.data(), blobLen, &got, 0)),
            "public key exports");
        auto* hdr = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
        const unsigned char* exp = blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
        const unsigned char* mod = exp + hdr->cbPublicExp;

        Check(Update::Crypto::RsaPkcs1Sha256Verify(mod, hdr->cbModulus, exp,
                    hdr->cbPublicExp, msg, strlen(msg), sig.data(), sig.size()),
            "valid signature verifies through UpdateCrypto");

        sig[10] ^= 0xFF; // tamper
        Check(!Update::Crypto::RsaPkcs1Sha256Verify(mod, hdr->cbModulus, exp,
                    hdr->cbPublicExp, msg, strlen(msg), sig.data(), sig.size()),
            "tampered signature rejected");

        // Wrong public key: the same (payload, signature) pair verified
        // against a different modulus must fail (key-substitution defense).
        std::vector<unsigned char> wrongMod(mod, mod + hdr->cbModulus);
        wrongMod[0] ^= 0xFF;
        wrongMod[wrongMod.size() - 1] ^= 0xFF;
        Check(!Update::Crypto::RsaPkcs1Sha256Verify(wrongMod.data(), wrongMod.size(),
                    exp, hdr->cbPublicExp, msg, strlen(msg), sig.data(), sig.size()),
            "signature rejected under the wrong public key");

        // Placeholder (all-zero) modulus must fail closed.
        static const unsigned char kZeroMod[256] = { 0 };
        static const unsigned char kExp[] = { 0x01, 0x00, 0x01 };
        Check(!Update::Crypto::RsaPkcs1Sha256Verify(kZeroMod, sizeof(kZeroMod),
                    kExp, sizeof(kExp), msg, strlen(msg), sig.data(), sig.size()),
            "zeroed key fails closed");

        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    static void Run() {
        SemVer();
        Urls();
        Manifest();
        Hashing();
        RsaRoundTrip();
    }
};

int main() {
    try {
        UpdateRegression::Run();
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "ALL UPDATE TESTS PASSED\n";
    return 0;
}
