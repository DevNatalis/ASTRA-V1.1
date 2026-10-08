<#
.SYNOPSIS
  Generates the RSA-2048 update signing keypair (offline, maintainer machine).
.DESCRIPTION
  Creates update_priv.pem (KEEP SECRET, never commit) and prints the C++ blob
  to paste into Core/Update/UpdateConfig.hpp (UpdatePublicKey::Exponent/Modulus).
  Requires .NET (Windows PowerShell 5.1+ is enough) and openssl only for the
  optional fingerprint check.
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts/new_update_keys.ps1
#>
$ErrorActionPreference = 'Stop'

$outDir = Join-Path $PSScriptRoot 'update_keys'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$rsa = [System.Security.Cryptography.RSA]::Create(2048)
try {
    $privPem = $rsa.ExportRSAPrivateKey()
    # -----BEGIN RSA PRIVATE KEY----- (PKCS#1 DER -> PEM)
    $b64 = [Convert]::ToBase64String($privPem, [Base64FormattingOptions]::InsertLineBreaks)
    $pem = "-----BEGIN RSA PRIVATE KEY-----`r`n$b64`r`n-----END RSA PRIVATE KEY-----`r`n"
    $privPath = Join-Path $outDir 'update_priv.pem'
    [IO.File]::WriteAllText($privPath, $pem)
    Write-Host "Private key written to: $privPath"
    Write-Host 'BACK IT UP OFFLINE AND NEVER COMMIT IT.' -ForegroundColor Red

    $params = $rsa.ExportParameters($false)
    if ($params.Modulus.Length -ne 256) { throw 'Unexpected modulus size.' }

    function To-CppArray([byte[]]$bytes, [string]$name) {
        $rows = for ($i = 0; $i -lt $bytes.Length; $i += 16) {
            $chunk = $bytes[$i..([Math]::Min($i + 15, $bytes.Length - 1))] |
                ForEach-Object { '0x{0:X2}' -f $_ }
            '        ' + ($chunk -join ', ') + ','
        }
        "    static constexpr unsigned char $name[] = {`r`n$($rows -join "`r`n")`r`n    };"
    }

    # Strip leading zero padding the exporter may add.
    $exp = $params.Exponent
    while ($exp.Length -gt 1 -and $exp[0] -eq 0) { $exp = $exp[1..($exp.Length - 1)] }

    $cpp = @'
// Paste into Core/Update/UpdateConfig.hpp, UpdatePublicKey:
' + [Environment]::NewLine + (To-CppArray $exp 'kExp') + [Environment]::NewLine +
        (To-CppArray $params.Modulus 'kMod')
    $cppPath = Join-Path $outDir 'public_key_snippet.txt'
    [IO.File]::WriteAllText($cppPath, $cpp)
    Write-Host "C++ snippet written to: $cppPath"

    $sha = [System.Security.Cryptography.SHA256]::Create()
    $fp = ($sha.ComputeHash($params.Modulus) | ForEach-Object { '{0:x2}' -f $_ }) -join ''
    Write-Host "Modulus SHA-256 fingerprint: $fp"
    Write-Host 'Compare this fingerprint in CI logs after wiring UPDATE_SIGNING_KEY.'
} finally {
    $rsa.Dispose()
}
