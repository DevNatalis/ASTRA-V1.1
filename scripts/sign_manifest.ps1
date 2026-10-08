<#
.SYNOPSIS
  Signs a release payload and emits manifest.json (maintainer machine or CI).
.DESCRIPTION
  Recomputes SHA-256 of the built ASTRA.exe, builds the canonical payload
    version + "`n" + sha256 + "`n" + url
  signs it with the RSA private key (openssl), and writes manifest.json.
  The private key NEVER enters the repo: pass it via file path (local) or the
  UPDATE_SIGNING_KEY secret (CI writes it to a temp file).
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts/sign_manifest.ps1 `
    -ExePath src\x64\Release\ASTRA.exe -Version 1.1.0 `
    -Url https://github.com/acme/astra/releases/download/v1.1.0/ASTRA.exe `
    -KeyPath scripts\update_keys\update_priv.pem -Out manifest.json
#>
param(
    [Parameter(Mandatory)] [string]$ExePath,
    [Parameter(Mandatory)] [string]$Version,
    [Parameter(Mandatory)] [string]$Url,
    [Parameter(Mandatory)] [string]$KeyPath,
    [string]$Out = 'manifest.json',
    [bool]$Mandatory = $false,
    [string]$MinVersion = '',
    [string]$Status = 'Disponivel',
    [string]$Notes = ''
)
$ErrorActionPreference = 'Stop'

if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw "Version '$Version' is not strict semver." }
if ($Url -notmatch '^https://') { throw 'URL must be HTTPS.' }
if (-not (Test-Path $ExePath)) { throw "Executable not found: $ExePath" }
if (-not (Test-Path $KeyPath)) { throw "Private key not found: $KeyPath" }

$hash = (Get-FileHash -Path $ExePath -Algorithm SHA256).Hash.ToLower()
$payload = "$Version`n$hash`n$Url"
$payloadPath = [IO.Path]::GetTempFileName()
$sigPath = [IO.Path]::GetTempFileName()
try {
    [IO.File]::WriteAllText($payloadPath, $payload)
    & openssl dgst -sha256 -sign $KeyPath -out $sigPath $payloadPath
    if ($LASTEXITCODE -ne 0) { throw 'openssl signing failed.' }
    $sigBytes = [IO.File]::ReadAllBytes($sigPath)
    $sigB64 = [Convert]::ToBase64String($sigBytes)

    $manifest = [ordered]@{
        version   = $Version
        url       = $Url
        sha256    = $hash
        signature = $sigB64
        mandatory = [bool]$Mandatory
    }
    if ($MinVersion) { $manifest['min_version'] = $MinVersion }
    if ($Status) { $manifest['status'] = $Status }
    if ($Notes) { $manifest['notes'] = $Notes }
    ($manifest | ConvertTo-Json -Compress) | Out-File -FilePath $Out -Encoding utf8NoBOM
    Write-Host "manifest.json written to: $Out"
    Write-Host "payload: $($payload -replace "`n", '\n')"
} finally {
    Remove-Item $payloadPath, $sigPath -Force -ErrorAction SilentlyContinue
}
