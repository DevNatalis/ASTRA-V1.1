# Dependency rebuild (2026-10-06)

`build_security_dependencies.py` verifies pinned archive SHA-256 values before extracting and rebuilding with CMake, VS 2022 v143 x64. Download locations and hashes are declared in the script; archives must be placed in `build/security-update/downloads` first. Outputs and previous-library backups remain under `build/security-update`.

- curl 8.22.0: official curl.se release archive; static library, Windows Schannel TLS, HTTP/HTTPS only, Windows IDN. Production request options restrict transport to HTTPS. Optional compression and HTTP/2 dependencies are disabled.
- FreeType 2.12.1: official freetype/freetype GitHub tag VER-2-12-1. Version preserved for font compatibility; rebuilt without optional zlib, bzip2, PNG, HarfBuzz or Brotli dependencies. This is not an upgrade to a newer FreeType release.
- Release uses /MD; Debug uses /MDd and separate Debug library directories. `src/src.vcxproj` selects the matching libraries. Licenses are adjacent to installed libraries and included in the distribution.

`tests/security_report_checks.py` queries versions from linked libraries. `tests/https_transport_checks.py` uses temporary localhost certificates to verify HTTPS-only transport, trust, hostname and pin rejection; it does not change the system trust store or contact the authentication service. Test keys remain in ignored build output and are never packaged.

`package_security_update.py` backs up old artifacts, copies the Release executable and packages an explicit file list. It verifies every archived file against SHA-256 and checks ZIP integrity. It does not compile or execute ASTRA; run it only after successful Release validation.
