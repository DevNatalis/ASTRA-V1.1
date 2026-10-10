#pragma once
// UpdateCrypto: hashing + RSA signature verification for the update channel.
// Uses Windows CNG (BCrypt) only — no third-party crypto dependency, so this
// header is shared between svchost.exe and the standalone Updater.exe.
//
// Signature scheme (documented in docs/UPDATES.md):
//   payload   = version + "\n" + sha256_hex + "\n" + url
//   signature = RSA_PKCS1v15_SHA256(payload), base64-encoded
// Signing (offline, maintainer machine):
//   openssl dgst -sha256 -sign update_priv.pem -out sig.bin payload.bin
#include <windows.h>
#include <bcrypt.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib") // CryptStringToBinaryA (base64)

namespace Update {
namespace Crypto {

inline bool Sha256(const void* data, size_t len, unsigned char out[32]) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    bool ok = false;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) {
        DWORD cbHash = 0, cbOut = 0;
        if (BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
                reinterpret_cast<PUCHAR>(&cbHash), sizeof(cbHash), &cbOut, 0)) && cbHash == 32 &&
            BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)) &&
            BCRYPT_SUCCESS(BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0))) {
            ok = BCRYPT_SUCCESS(BCryptFinishHash(hHash, out, 32, 0));
        }
        if (hHash) BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (!ok) memset(out, 0, 32);
    return ok;
}

inline bool Sha256File(const std::wstring& path, unsigned char out[32], uint64_t sizeCap = 0) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    bool ok = false;
    if (GetFileSizeEx(h, &sz) && sz.QuadPart > 0 &&
        (sizeCap == 0 || (uint64_t)sz.QuadPart <= sizeCap)) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0)) &&
            BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0))) {
            ok = true;
            // 64KB chunk buffer on the heap (C6262: never park this on the stack).
            std::vector<unsigned char> buf(65536);
            DWORD got = 0;
            LONGLONG left = sz.QuadPart;
            while (ok && left > 0) {
                DWORD want = left > (LONGLONG)buf.size() ? (DWORD)buf.size() : (DWORD)left;
                if (!ReadFile(h, buf.data(), want, &got, nullptr) || got == 0) { ok = false; break; }
                if (!BCRYPT_SUCCESS(BCryptHashData(hHash, buf.data(), got, 0))) { ok = false; break; }
                left -= got;
            }
            if (ok) ok = BCRYPT_SUCCESS(BCryptFinishHash(hHash, out, 32, 0));
            if (hHash) BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
        }
    }
    CloseHandle(h);
    if (!ok) memset(out, 0, 32);
    return ok;
}

inline std::string HexOf(const unsigned char* data, size_t len) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(kHex[data[i] >> 4]);
        out.push_back(kHex[data[i] & 0xF]);
    }
    return out;
}

inline bool HexToBytes(const std::string& hex, std::vector<unsigned char>& out) {
    out.clear();
    if (hex.size() % 2 != 0) return false;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = nib(hex[i]), lo = nib(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back((unsigned char)((hi << 4) | lo));
    }
    return true;
}

inline bool Base64Decode(const std::string& in, std::vector<unsigned char>& out) {
    out.clear();
    if (in.empty() || in.size() % 4 != 0) return false;
    DWORD need = 0;
    if (!CryptStringToBinaryA(in.c_str(), (DWORD)in.size(), CRYPT_STRING_BASE64_ANY,
            nullptr, &need, nullptr, nullptr) || need == 0)
        return false;
    out.resize(need);
    DWORD got = need;
    if (!CryptStringToBinaryA(in.c_str(), (DWORD)in.size(), CRYPT_STRING_BASE64_ANY,
            out.data(), &got, nullptr, nullptr))
        return false;
    out.resize(got);
    return !out.empty();
}

inline std::string Base64Encode(const unsigned char* data, size_t len) {
    if (!data || len == 0) return {};
    DWORD need = 0;
    if (!CryptBinaryToStringA(data, (DWORD)len,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &need) || need == 0)
        return {};
    std::string out(need, '\0');
    if (!CryptBinaryToStringA(data, (DWORD)len,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &need))
        return {};
    while (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

// Verifies RSA PKCS#1 v1.5 + SHA-256 over the raw payload bytes.
// mod/exp are big-endian (UpdatePublicKey layout). Returns false on any
// error AND when the key is the unconfigured placeholder.
inline bool RsaPkcs1Sha256Verify(const unsigned char* mod, size_t modLen,
        const unsigned char* exp, size_t expLen,
        const void* payload, size_t payloadLen,
        const unsigned char* sig, size_t sigLen) {
    if (!mod || !exp || !payload || !sig) return false;
    if (modLen != 256 || expLen == 0 || expLen > 8 || sigLen != 256) return false;
    bool nonzero = false;
    for (size_t i = 0; i < modLen; ++i) nonzero = nonzero || (mod[i] != 0);
    if (!nonzero) return false; // placeholder key: fail closed

    unsigned char digest[32];
    if (!Sha256(payload, payloadLen, digest)) return false;

    // BCRYPT_RSAPUBLIC_BLOB layout: header + exponent + modulus.
    std::vector<unsigned char> blob(sizeof(BCRYPT_RSAKEY_BLOB) + expLen + modLen);
    BCRYPT_RSAKEY_BLOB* hdr = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
    hdr->Magic = BCRYPT_RSAPUBLIC_MAGIC;
    hdr->BitLength = 2048;
    hdr->cbPublicExp = (ULONG)expLen;
    hdr->cbModulus = (ULONG)modLen;
    hdr->cbPrime1 = 0;
    hdr->cbPrime2 = 0;
    memcpy(blob.data() + sizeof(BCRYPT_RSAKEY_BLOB), exp, expLen);
    memcpy(blob.data() + sizeof(BCRYPT_RSAKEY_BLOB) + expLen, mod, modLen);

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    bool ok = false;
    BCRYPT_PKCS1_PADDING_INFO pad{};
    pad.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, nullptr, 0)) &&
        BCRYPT_SUCCESS(BCryptImportKeyPair(hAlg, nullptr, BCRYPT_RSAPUBLIC_BLOB,
            &hKey, blob.data(), (ULONG)blob.size(), 0))) {
        ok = BCRYPT_SUCCESS(BCryptVerifySignature(hKey, &pad, digest, sizeof(digest),
            (PUCHAR)sig, (ULONG)sigLen, BCRYPT_PAD_PKCS1));
    }
    if (hKey) BCryptDestroyKey(hKey);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    SecureZeroMemory(digest, sizeof(digest));
    return ok;
}

} // namespace Crypto
} // namespace Update
