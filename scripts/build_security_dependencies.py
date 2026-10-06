"""Rebuild the pinned Windows x64 dependencies; no downloads or application launch."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'build/security-update'
CMAKE = Path(r'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
INSTANCE = r'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
SOURCES = {
    'curl': ('curl-8.22.0.tar.xz', 'curl-8.22.0',
             'f7ef3ae8a22e521f289803fe93543eb64c329b58aa73a9e224dfd915a2a5f4f7',
             'https://curl.se/download/curl-8.22.0.tar.xz'),
    'freetype': ('freetype-2.12.1.tar.gz', 'freetype-VER-2-12-1',
                 '0e72cae32751598d126cfd4bceda909f646b7231ab8c52e28abb686c20a2bea1',
                 'https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-12-1'),
}

def run(label, args):
    print(label, flush=True)
    with (WORK / (label + '.log')).open('wb') as log:
        result = subprocess.run([str(CMAKE), *args], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((WORK / (label + '.log')).read_text(errors='replace')[-6000:])
        raise SystemExit(result.returncode)

def install(source, target):
    target = ROOT / target
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        backup = WORK / 'before' / target.relative_to(ROOT)
        if not backup.exists():
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(target, backup)
    shutil.copy2(source, target)

for name, (archive, directory, expected, _) in SOURCES.items():
    path = WORK / 'downloads' / archive
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise SystemExit(f'Unexpected SHA256: {path}')
    if not (WORK / directory).exists():
        with tarfile.open(path) as bundle:
            bundle.extractall(WORK, filter='data')
    options = [
        '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143',
        f'-DCMAKE_GENERATOR_INSTANCE={INSTANCE}',
        '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', '-DCMAKE_POLICY_DEFAULT_CMP0091=NEW',
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL',
        '-DBUILD_SHARED_LIBS=OFF', '-DCMAKE_DEBUG_POSTFIX=',
    ]
    if name == 'curl':
        options += [
            '-DBUILD_STATIC_LIBS=ON', '-DBUILD_CURL_EXE=OFF', '-DBUILD_TESTING=OFF',
            '-DCURL_USE_SCHANNEL=ON', '-DCURL_USE_OPENSSL=OFF', '-DCURL_STATIC_CRT=OFF',
            '-DHTTP_ONLY=ON', '-DCURL_USE_LIBPSL=OFF', '-DCURL_USE_LIBSSH2=OFF',
            '-DUSE_LIBIDN2=OFF', '-DUSE_WIN32_IDN=ON', '-DUSE_NGHTTP2=OFF',
            '-DCURL_ZLIB=OFF', '-DCURL_BROTLI=OFF', '-DCURL_ZSTD=OFF',
            '-DLIBCURL_OUTPUT_NAME=libcurl',
        ]
    else:
        options += [f'-DFT_DISABLE_{dep}=ON' for dep in ('ZLIB', 'BZIP2', 'PNG', 'HARFBUZZ', 'BROTLI')]
        options += ['-DDISABLE_FORCE_DEBUG_POSTFIX=ON']
    build = WORK / (name + '-build')
    run(name + '-configure', ['-S', str(WORK / directory), '-B', str(build), *options])
    for config in ('Release', 'Debug'):
        run(name + '-' + config, ['--build', str(build), '--config', config, '--parallel', '4'])

# Install only after both dependencies and both configurations compiled.
curl_headers = WORK / SOURCES['curl'][1] / 'include/curl'
for header in curl_headers.glob('*.h'):
    install(header, Path('src/Security/Api/curl') / header.name)
for config in ('Release', 'Debug'):
    curl_lib = WORK / 'curl-build/lib' / config / 'libcurl.lib'
    ft_lib = WORK / 'freetype-build' / config / 'freetype.lib'
    suffix = Path('Debug') if config == 'Debug' else Path('.')
    install(curl_lib, Path('src/Security/Api/curl') / suffix / 'libcurl.lib')
    install(ft_lib, Path('src/Includes/ImGui/Files/FreeType/win64') / suffix / 'freetype.lib')
install(WORK / SOURCES['curl'][1] / 'COPYING', Path('src/Security/Api/curl/COPYING'))
install(WORK / SOURCES['freetype'][1] / 'docs/FTL.TXT', Path('src/Includes/ImGui/Files/FreeType/FTL.TXT'))
(WORK / 'dependencies.json').write_text(json.dumps({
    name: {'archive': info[0], 'sha256': info[2], 'url': info[3]}
    for name, info in SOURCES.items()
}, indent=2), encoding='utf-8')
print('Installed libcurl 8.22.0 and FreeType 2.12.1: Release /MD, Debug /MDd.', flush=True)
