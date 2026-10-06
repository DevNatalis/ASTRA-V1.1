"""Package the validated Release; never run the application."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
backup = root / 'build/security-update/before'
dist = root / 'dist'
archive = root / 'ASTRA-v1.1-win64.zip'
for path in (dist / 'ASTRA.exe', archive, dist / 'LEIA-ME.txt'):
    target = backup / path.relative_to(root)
    if path.exists() and not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
shutil.copy2(root / 'src/x64/Release/ASTRA.exe', dist / 'ASTRA.exe')
(dist / 'licenses').mkdir(exist_ok=True)
shutil.copy2(root / 'src/Security/Api/curl/COPYING', dist / 'licenses/curl-COPYING.txt')
shutil.copy2(root / 'src/Includes/ImGui/Files/FreeType/FTL.TXT', dist / 'licenses/FreeType-FTL.txt')
readme = dist / 'LEIA-ME.txt'
content = readme.read_text(encoding='utf-8-sig')
note = '\nAtualizacao de seguranca: libcurl 8.22.0 / Schannel; FreeType 2.12.1.\nDescoberta de nomes exige HTTPS com certificado valido; servidores somente HTTP nao fornecem esses metadados.\nLicencas de dependencias em licenses/.\n'
if note not in content:
    readme.write_text(content + note, encoding='utf-8')
files = ['ASTRA.exe', 'd3dx9_43.dll', 'd3dx11_43.dll', 'D3DCompiler_43.dll',
         'LEIA-ME.txt', 'licenses/curl-COPYING.txt', 'licenses/FreeType-FTL.txt']
hashes = {name: hashlib.sha256((dist / name).read_bytes()).hexdigest() for name in files}
(dist / 'SHA256.json').write_text(json.dumps(hashes, indent=2) + '\n', encoding='utf-8')
temp = archive.with_suffix('.zip.tmp')
with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED) as output:
    for name in files + ['SHA256.json']:
        output.write(dist / name, name)
with zipfile.ZipFile(temp) as output:
    assert output.testzip() is None
    for name, digest in hashes.items():
        assert hashlib.sha256(output.read(name)).hexdigest() == digest
assert hashes['ASTRA.exe'] == hashlib.sha256((root / 'src/x64/Release/ASTRA.exe').read_bytes()).hexdigest()
temp.replace(archive)
print('Validated ZIP: 8 files; Release/dist/ZIP executable hashes match.')
print('ASTRA.exe SHA256:', hashes['ASTRA.exe'])
