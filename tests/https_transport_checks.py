"""Exercise the actual static libcurl against an isolated local TLS fixture."""
from pathlib import Path
import base64
import datetime
import hashlib
import http.server
import ipaddress
import ssl
import subprocess
import threading
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/security-update/tls-checks'
OUT.mkdir(parents=True, exist_ok=True)
code = r'''
#define CURL_STATICLIB
#include <Security/Api/curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <string>
size_t Discard(char*, size_t size, size_t count, void*) { return size * count; }
int main(int argc, char** argv) {
    if (argc != 5 || curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 2;
    CURL* curl = curl_easy_init();
    if (!curl) return 2;
    bool configured = true;
    char error[CURL_ERROR_SIZE] = {};
    auto option = [&](CURLoption key, auto value) {
        configured = curl_easy_setopt(curl, key, value) == CURLE_OK && configured;
    };
    option(CURLOPT_URL, argv[1]);
    option(CURLOPT_ERRORBUFFER, error);
    option(CURLOPT_PROTOCOLS_STR, "https");
    option(CURLOPT_REDIR_PROTOCOLS_STR, "https");
    option(CURLOPT_FOLLOWLOCATION, 0L);
    option(CURLOPT_PROXY, ""); // Only the local fixture; never send it via a proxy.
    option(CURLOPT_SSL_VERIFYPEER, 1L);
    option(CURLOPT_SSL_VERIFYHOST, 2L);
    option(CURLOPT_TIMEOUT, 5L);
    option(CURLOPT_CONNECTTIMEOUT, 3L);
    option(CURLOPT_WRITEFUNCTION, Discard);
    if (std::string(argv[2]) != "-") option(CURLOPT_CAINFO, argv[2]);
    if (std::string(argv[3]) != "-") option(CURLOPT_PINNEDPUBLICKEY, argv[3]);
    const CURLcode result = configured ? curl_easy_perform(curl) : CURLE_FAILED_INIT;
    std::cout << "CURLcode=" << result << " expected=" << argv[4] << '\n';
    if (static_cast<int>(result) != std::atoi(argv[4])) std::cerr << error << '\n';
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return static_cast<int>(result) == std::atoi(argv[4]) ? 0 : 1;
}
'''
(OUT / 'probe.cpp').write_text(code, encoding='utf-8')
vcvars = r'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
(OUT / 'build.cmd').write_text(
    f'@call "{vcvars}" >nul\n'
    f'@cl /nologo /std:c++17 /EHsc /W4 /MD /I"{ROOT / "src"}" probe.cpp /Fe:probe.exe /link /LIBPATH:"{ROOT / "src/Security/Api/curl"}" libcurl.lib ws2_32.lib crypt32.lib normaliz.lib advapi32.lib secur32.lib bcrypt.lib iphlpapi.lib\n'
    '@exit /b %errorlevel%\n', encoding='utf-8')
subprocess.run(['cmd.exe', '/d', '/c', str(OUT / 'build.cmd')], cwd=OUT, check=True)

# Test-only CA/key. Never installed in the Windows trust store or copied to dist.
key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, 'ASTRA local TLS test')])
now = datetime.datetime.now(datetime.timezone.utc)
certificate = (x509.CertificateBuilder().subject_name(name).issuer_name(name)
    .public_key(key.public_key()).serial_number(x509.random_serial_number())
    .not_valid_before(now-datetime.timedelta(minutes=5)).not_valid_after(now+datetime.timedelta(days=1))
    .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
    .add_extension(x509.SubjectAlternativeName([x509.DNSName('localhost')]), critical=False)
    .sign(key, hashes.SHA256()))
cert_path = OUT / 'fixture-cert.pem'
key_path = OUT / 'fixture-key.pem'
cert_path.write_bytes(certificate.public_bytes(serialization.Encoding.PEM))
key_path.write_bytes(key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
spki = key.public_key().public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
pin = 'sha256//' + base64.b64encode(hashlib.sha256(spki).digest()).decode()
bad_pin = 'sha256//' + base64.b64encode(bytes(32)).decode()

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Length', '10')
        self.end_headers()
        self.wfile.write(b'local test')
    def log_message(self, *args):
        pass

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(cert_path, key_path)
server.socket = context.wrap_socket(server.socket, server_side=True)
worker = threading.Thread(target=server.serve_forever, daemon=True)
worker.start()
port = server.server_port
cases = [
    ('reject HTTP', f'http://127.0.0.1:{port}/', '-', '-', 1),
    ('accept trusted fixture and matching pin', f'https://localhost:{port}/', str(cert_path), pin, 0),
    ('reject wrong pin', f'https://localhost:{port}/', str(cert_path), bad_pin, 90),
    ('reject untrusted certificate', f'https://localhost:{port}/', '-', '-', 60),
    ('reject wrong hostname', f'https://127.0.0.1:{port}/', str(cert_path), pin, 60),
]
try:
    with (OUT / 'results.log').open('w', encoding='utf-8') as log:
        for label, url, ca, expected_pin, expected in cases:
            result = subprocess.run([str(OUT / 'probe.exe'), url, ca, expected_pin, str(expected)],
                                    capture_output=True, text=True, timeout=10)
            line = label + ': ' + result.stdout.strip()
            print(line, flush=True)
            if result.stderr: print(result.stderr, flush=True)
            log.write(line + '\n' + result.stderr)
            if result.returncode: raise SystemExit(result.returncode)
finally:
    server.shutdown()
    server.server_close()
    worker.join()
print('5 TLS/protocol cases passed; no application credentials or public endpoints used.')
