#pragma once
// ASTRA update channel: single maintenance point for product identity,
// version, support links and the remote manifest endpoint.
//
// HOW TO MAINTAIN:
//   - Bump kAppVersion on every release (must match the release tag vX.Y.Z).
//   - kManifestUrl points to the manifest.json of the LATEST release.
//     GitHub Releases layout:
//       https://github.com/<owner>/<repo>/releases/latest/download/manifest.json
//     (frozen per-version alternative:
//       https://github.com/<owner>/<repo>/releases/download/vX.Y.Z/manifest.json)
//   - kUpdatePublicKey_* authenticates manifests. Generate with
//     scripts/new_update_keys.ps1 (NEVER commit the private key).
//   - kDiscordUrl is shown on the launcher Description tab.
//   - kServiceStatusDefault is shown until a signed manifest overrides it.
//
// This header is intentionally dependency-free (no ImGui, no curl) so the
// standalone Updater.exe can share it.
#include <cstddef>
#include <cstdint>
#include <string>

namespace Update {

// Single source of truth for the installed version. Displayed on the
// login screen ("ASTRA vX.Y.Z") and the launcher Description tab.
// CI may override it WITHOUT editing this file:
//   set CL=/DASTRA_APP_VERSION="\"1.2.3\""   (MSVC CL env passthrough)
#ifdef ASTRA_APP_VERSION
inline const char* InstalledVersion() { return ASTRA_APP_VERSION; }
#else
inline const char* InstalledVersion() { return "1.0.1"; }
#endif

inline std::string InstalledVersionStr() { return InstalledVersion(); }
inline std::string InstalledDisplay() { return std::string("v") + InstalledVersion(); }

struct ChannelConfig {
    // Remote manifest endpoint (HTTPS only; enforced by UpdateManager).
    // GitHub Releases layout for this repository.
    static const char* ManifestUrl() {
        return "https://github.com/DevNatalis/ASTRA-V1.1/releases/latest/download/manifest.json";
    }
    static const char* ProductName() { return "ASTRA"; }
    static const char* DiscordUrl() { return "https://discord.gg/<invite>"; }
    static const char* AppDescription() {
        return "ASTRA external panel with assisted aim, visuals and local "
               "utilities. This launcher keeps the build updated through a "
               "signed channel: every package is hash- and signature-checked "
               "before install, and the previous version is preserved on failure.";
    }
    // Shown until a signed manifest provides a newer status value.
    // Allowed values: "Disponivel" | "Manutencao" | "Indisponivel".
    static const char* ServiceStatusDefault() { return "Disponivel"; }
};

// RSA-2048 public key (exponent + modulus, big-endian) that verifies
// manifest signatures. Generated offline with scripts/new_update_keys.ps1;
// the private half lives OUTSIDE the repo and in the UPDATE_SIGNING_KEY
// secret only. Modulus SHA-256 fingerprint:
//   7a7af9f148f7466bc6382c330a08e585699dab61885a3c44b4b201c69688c73a
struct UpdatePublicKey {
    static constexpr unsigned long kBitLength = 2048;
    // 0x01 0x00 0x01 (65537).
    static const unsigned char* Exponent(size_t& outLen) {
        static constexpr unsigned char kExp[] = {
            0x01, 0x00, 0x01,
        };
        outLen = sizeof(kExp);
        return kExp;
    }
    // 256-byte modulus, big-endian.
    static const unsigned char* Modulus(size_t& outLen) {
        static constexpr unsigned char kMod[] = {
            0xBC, 0xDF, 0xAC, 0xE3, 0xB2, 0xAF, 0x11, 0xDF, 0x34, 0x4A, 0x7D, 0x08, 0x25, 0xAA, 0x63, 0x61,
            0xA5, 0x39, 0x46, 0xA3, 0xB9, 0xAC, 0xD2, 0x1F, 0xEE, 0x26, 0x56, 0x03, 0x0A, 0xB7, 0x07, 0xA9,
            0x21, 0x6A, 0x36, 0x9E, 0x70, 0x1D, 0x13, 0xDF, 0x02, 0x44, 0xDA, 0x55, 0x7C, 0x04, 0x70, 0x27,
            0x61, 0x9D, 0x2A, 0xC4, 0xBF, 0x53, 0x20, 0x67, 0xB9, 0x3D, 0x88, 0x1E, 0x21, 0x9A, 0xE3, 0xC0,
            0xE6, 0xC8, 0x62, 0xF7, 0x99, 0x13, 0x34, 0xAE, 0x83, 0x8C, 0xB3, 0x79, 0x11, 0xAE, 0xF9, 0x5E,
            0x28, 0x3F, 0x9F, 0x62, 0xC9, 0x2A, 0x3B, 0x42, 0x39, 0x8A, 0xFB, 0x9B, 0xB1, 0x0B, 0x6D, 0xAE,
            0x53, 0xD8, 0x8F, 0x1B, 0xED, 0x95, 0x5E, 0xD1, 0xC2, 0xEF, 0x1D, 0xBF, 0xED, 0xE3, 0xFB, 0x22,
            0x0C, 0xB5, 0x63, 0x2E, 0xF5, 0xF5, 0x95, 0x94, 0xE6, 0x7A, 0xB0, 0x6C, 0xC8, 0x22, 0x05, 0x31,
            0x62, 0x17, 0x41, 0x6F, 0x80, 0xBC, 0x7F, 0xFA, 0xC3, 0x47, 0x33, 0x3E, 0x69, 0xF3, 0x63, 0xBD,
            0x32, 0xFD, 0x89, 0x05, 0xE0, 0x27, 0x45, 0xB2, 0x13, 0x2C, 0x11, 0x61, 0x47, 0x0A, 0xA1, 0xAB,
            0xDE, 0x9B, 0xDB, 0xB0, 0xAE, 0x0B, 0xA2, 0x75, 0x11, 0xD2, 0x63, 0x79, 0xEB, 0x70, 0x77, 0x68,
            0x4B, 0x23, 0x69, 0x30, 0x3E, 0xA1, 0x2D, 0x70, 0x70, 0x62, 0x83, 0x6A, 0x04, 0x51, 0x26, 0x78,
            0x7A, 0x2F, 0xAB, 0x6F, 0xC6, 0xA1, 0xFB, 0x62, 0x44, 0x37, 0x44, 0xD8, 0x82, 0xCD, 0x55, 0x7C,
            0x24, 0xDC, 0x57, 0xD1, 0x85, 0x2F, 0xEA, 0x76, 0x76, 0xDB, 0x5B, 0xA3, 0xAF, 0x6D, 0xFB, 0xDE,
            0x78, 0xEE, 0xED, 0x81, 0x34, 0xAB, 0x0A, 0xF8, 0xA7, 0xA5, 0xAD, 0x27, 0xD0, 0xF5, 0xF9, 0x01,
            0x9F, 0x0C, 0x2E, 0x8F, 0x75, 0x0E, 0xB8, 0xF6, 0xF8, 0xBE, 0xF9, 0x65, 0x00, 0x2A, 0x92, 0xED,
        };
        outLen = sizeof(kMod);
        return kMod;
    }
    static bool IsConfigured() {
        size_t len = 0;
        const unsigned char* mod = Modulus(len);
        if (len != 256) return false;
        for (size_t i = 0; i < len; ++i)
            if (mod[i] != 0) return true;
        return false;
    }
};

} // namespace Update
