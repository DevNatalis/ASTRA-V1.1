<#
.SYNOPSIS
  End-to-end matrix for the ASTRA auto-update channel (isolated, no install touched).
.DESCRIPTION
  Spins a local TLS fixture server (self-signed localhost cert, cleaned up
  afterwards), signs fixtures with the PRODUCTION private key kept OUTSIDE the
  repo ($env:USERPROFILE\.astra-update-keys\update_priv.pem â€” read-only use,
  never copied or logged), and drives:
    - update_e2e.exe (real UpdateManager: TLS check, download, verify)
    - Updater.exe    (real swap: backup, replace, rollback, relaunch)
    - UpdaterCore    (step-level rollback with real file ops)
  Scenarios: happy path, interrupted download, tampered manifest, forged
  signature, older remote, swap failure, rollback, mandatory update.
  Everything happens under src\x64\Tests\e2e (git-ignored). The real ASTRA.exe
  installation is NEVER touched.
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File src/tests/run_update_e2e.ps1
#>
$ErrorActionPreference = 'Stop'

$root = Split-Path (Split-Path $PSCommandPath -Parent) -Parent   # src\
$bin = Join-Path $root 'x64\Tests'
$e2e = Join-Path $bin 'e2e'
$prodKey = Join-Path $env:USERPROFILE '.astra-update-keys\update_priv.pem'
if (-not (Test-Path $prodKey)) { throw "Production key not found: $prodKey" }

$script:failures = @()
function First-Value([array]$lines, [string]$prefix) {
    $m = @($lines) | Where-Object { $_ -match ('^' + $prefix + '=') } | Select-Object -First 1
    if ($null -eq $m) { return '' }
    return ($m -replace ('^' + $prefix + '='), '')
}
function Ok([string]$name, [bool]$cond, [string]$detail = '') {
    if ($cond) { Write-Host "PASS: $name" }
    else { $script:failures += $name; Write-Host "FAIL: $name $detail" -ForegroundColor Red }
}

# ---------- build test binaries (single cmd session keeps vcvars env) ----------
Write-Host '--- building e2e binaries ---'
if (-not (Test-Path $e2e)) { New-Item -ItemType Directory -Force -Path $e2e | Out-Null }
$vcvars = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$buildCmd = "`"$vcvars`" >nul && cd /d `"$root`" && " +
    "cl /nologo /std:c++latest /EHsc /O2 /MD /I. tests\restart_probe.cpp " +
    "/Fo:$e2e\restart_probe.obj /Fe:$e2e\restart_probe.exe /link /IGNORE:4099 && " +
    "cl /nologo /std:c++latest /EHsc /O2 /MD /DCURL_STATICLIB /I. tests\update_e2e.cpp " +
    "/Fo:$e2e\update_e2e.obj /Fe:$e2e\update_e2e.exe " +
    "/link bcrypt.lib crypt32.lib ws2_32.lib advapi32.lib secur32.lib normaliz.lib wldap32.lib iphlpapi.lib /LIBPATH:Security\Api\curl /IGNORE:4099"
cmd /c $buildCmd
if ($LASTEXITCODE -ne 0) { throw 'e2e test build failed' }
$driver = Join-Path $e2e 'update_e2e.exe'
$probe = Join-Path $e2e 'restart_probe.exe'
$updater = Join-Path $root 'x64\Release\Updater.exe'
if (-not (Test-Path $updater)) { throw "Updater.exe missing: build Release x64 first" }

# ---------- production-key signer (PKCS#1 DER parse, test-only) ----------
function Read-Tlv([byte[]]$der, [int]$pos) {
    $tag = $der[$pos]; $p = $pos + 1
    $len = $der[$p]; $p++
    if ($len -band 0x80) {
        $n = $len -band 0x7F; $len = 0
        for ($i = 0; $i -lt $n; $i++) { $len = ($len -shl 8) -bor $der[$p]; $p++ }
    }
    return @{ Tag = $tag; Value = $der[$p..($p + $len - 1)]; Next = $p + $len }
}
function Get-ProdRsa {
    $pem = ((Get-Content $prodKey -Raw) -replace '-----[^-]+-----', '').Trim() -replace '\s+', ''
    $der = [Convert]::FromBase64String($pem)
    $seq = Read-Tlv $der 0
    if ($seq.Tag -ne 0x30) { throw 'Not a PKCS#1 SEQUENCE' }
    $items = @(); $p = 0
    while ($p -lt $seq.Value.Length) {
        $t = Read-Tlv $seq.Value $p
        if ($t.Tag -ne 0x02) { throw 'Expected INTEGER in private key' }
        $items += ,$t.Value; $p = $t.Next
    }
    if ($items.Count -ne 9) { throw 'PKCS#1 must hold 9 INTEGERs' }
    function Norm([byte[]]$v, [int]$size) {
        while ($v.Length -gt 1 -and $v[0] -eq 0) { $v = $v[1..($v.Length - 1)] }
        if ($v.Length -gt $size) { throw 'oversized key component' }
        if ($v.Length -lt $size) { $v = (,([byte]0) * ($size - $v.Length)) + $v }
        return $v
    }
    $rp = New-Object Security.Cryptography.RSAParameters
    $rp.Modulus = Norm $items[1] 256; $rp.Exponent = Norm $items[2] 3
    $rp.D = Norm $items[3] 256; $rp.P = Norm $items[4] 128; $rp.Q = Norm $items[5] 128
    $rp.DP = Norm $items[6] 128; $rp.DQ = Norm $items[7] 128
    $rp.InverseQ = Norm $items[8] 128
    $rsa = New-Object Security.Cryptography.RSACryptoServiceProvider
    $rsa.ImportParameters($rp)
    return $rsa
}
$script:prodRsa = Get-ProdRsa
function Sign-Payload([string]$payload) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($payload)
    [Convert]::ToBase64String($script:prodRsa.SignData($bytes, 'SHA256'))
}
function Write-Manifest([string]$dir, [string]$version, [string]$url, [string]$sha,
        [string]$sig, [bool]$mandatory, [string]$minVer, [string]$status, [string]$notes) {
    $m = [ordered]@{ version = $version; url = $url; sha256 = $sha; signature = $sig }
    if ($mandatory) { $m['mandatory'] = $true }
    if ($minVer) { $m['min_version'] = $minVer }
    if ($status) { $m['status'] = $status }
    if ($notes) { $m['notes'] = $notes }
    [IO.File]::WriteAllText((Join-Path $dir 'manifest.json'), ($m | ConvertTo-Json -Compress))
}

# ---------- localhost TLS cert ----------
# Reuse an existing usable cert when possible: installing into Root raises a
# consent prompt that auto-cancels in headless sessions. A cert is reusable
# when it lives in My WITH its private key, is also trusted in Root, covers
# DNS:localhost with server-auth EKU and is not expired.
function Find-UsableCert {
    $now = Get-Date
    foreach ($c in (Get-ChildItem 'Cert:\CurrentUser\My' |
            Where-Object { $_.Subject -eq 'CN=localhost' -and $_.HasPrivateKey })) {
        if ($c.NotAfter -le $now.AddDays(1)) { continue }
        $rooted = Get-ChildItem 'Cert:\CurrentUser\Root' |
            Where-Object { $_.Thumbprint -eq $c.Thumbprint }
        if (-not $rooted) { continue }
        $san = ($c.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.17' })
        if (-not $san) { continue }
        return $c
    }
    return $null
}
Write-Host '--- localhost TLS cert ---'
$script:certOwned = $false
$cert = Find-UsableCert
if ($cert) {
    Write-Host ("Reusing trusted localhost cert " + $cert.Thumbprint)
} else {
    $cert = New-SelfSignedCertificate -DnsName 'localhost' -CertStoreLocation 'Cert:\CurrentUser\My' `
        -NotAfter (Get-Date).AddDays(2) -KeyUsage DigitalSignature, KeyEncipherment `
        -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.1')
    $script:certOwned = $true
    $store = New-Object Security.Cryptography.X509Certificates.X509Store('Root', 'CurrentUser')
    $store.Open('ReadWrite'); $store.Add($cert); $store.Close()
}
$thumb = $cert.Thumbprint
$pemBody = [Convert]::ToBase64String($cert.RawData, [Base64FormattingOptions]::InsertLineBreaks)
$caPem = Join-Path $e2e 'test-ca.pem'
"-----BEGIN CERTIFICATE-----`r`n$pemBody`r`n-----END CERTIFICATE-----`r`n" |
    Out-File $caPem -Encoding ascii
$env:CURL_CA_BUNDLE = $caPem  # honored by OpenSSL-backed curl; harmless otherwise

function Start-FixtureServer([string]$fixtureDir, [string]$mode) {
    $portFile = Join-Path $e2e ("port_{0}.txt" -f [IO.Path]::GetRandomFileName())
    if (Test-Path $portFile) { Remove-Item $portFile -Force }
    $job = Start-Job -ScriptBlock {
        param($srv, $pf, $th, $fx, $md)
        & powershell -ExecutionPolicy Bypass -File $srv -PortFile $pf `
            -Thumbprint $th -FixtureDir $fx -Mode $md -MaxRequests 60
    } -ArgumentList (Join-Path $root 'tests\tls_server.ps1'), $portFile, $thumb, $fixtureDir, $mode
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline -and -not (Test-Path $portFile)) { Start-Sleep -Milliseconds 100 }
    if (-not (Test-Path $portFile)) { throw 'fixture server did not start' }
    $port = [int](Get-Content $portFile -Raw)
    return @{ Job = $job; Port = $port; PortFile = $portFile }
}
function Stop-FixtureServer($srv) {
    if ($srv -and $srv.Job) {
        Stop-Job $srv.Job -ErrorAction SilentlyContinue | Out-Null
        Remove-Job $srv.Job -Force -ErrorAction SilentlyContinue | Out-Null
    }
    if ($srv -and $srv.PortFile -and (Test-Path $srv.PortFile)) {
        Remove-Item $srv.PortFile -Force -ErrorAction SilentlyContinue
    }
}
function New-ScenarioDir([string]$name) {
    $d = Join-Path $e2e $name
    if (Test-Path $d) { Remove-Item $d -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $d | Out-Null
    return $d
}
# 1MB pseudo-binary: runnable probe + random trailer (valid exe, distinct hash).
function New-Binary([string]$path) {
    Copy-Item $probe $path -Force
    $rnd = New-Object Random
    $trailer = New-Object byte[] (1024 * 1024)
    $rnd.NextBytes($trailer)
    $fs = [IO.File]::Open($path, [IO.FileMode]::Append, [IO.FileAccess]::Write,
        [IO.FileShare]::None)
    try { $fs.Write($trailer, 0, $trailer.Length) } finally { $fs.Close() }
    return ((Get-FileHash -Path $path -Algorithm SHA256).Hash).ToLower()
}

try {
    # ================= S1: happy path 1.0.0 -> 1.1.0 =================
    Write-Host '--- S1: happy path ---'
    $s1 = New-ScenarioDir 's1'
    $sha1 = New-Binary (Join-Path $s1 'app-1.1.0.exe')
    'APP-v1.0.0-installed-dummy' | Out-File (Join-Path $s1 'ASTRA.exe') -Encoding ascii -NoNewline
    $srv = $null
    try {
        $srv = Start-FixtureServer $s1 'normal'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s1 '1.1.0' "$base/app-1.1.0.exe" $sha1 `
            (Sign-Payload "1.1.0`n$sha1`n$base/app-1.1.0.exe") $false '' 'Disponivel' 'E2E fixtures.'
        $out = & $driver full --url "$base/manifest.json" --expect-terminal Ready `
            --expect-version '1.1.0' 2>&1
        $rc = $LASTEXITCODE
        $maxPoll = First-Value $out 'MAX_POLL_MS'
        $samples = First-Value $out 'SAMPLES'
        Ok 'S1 check+download reaches Ready' ($rc -eq 0) ($out -join "`n")
        Ok 'S1 progress sampled' ([int]$samples -gt 0) "samples=$samples"
        Ok 'S1 UI thread never blocked' ([int]$maxPoll -lt 200) "maxPoll=${maxPoll}ms"
        $staged = Join-Path ([IO.Path]::GetTempPath()) 'ASTRA-update\ASTRA-update.exe'
        $stagedHash = ((Get-FileHash -Path $staged -Algorithm SHA256).Hash).ToLower()
        Ok 'S1 staged file matches manifest hash' ($stagedHash -eq $sha1)
        # Real Updater.exe swap + relaunch into the scenario dir.
        $target = Join-Path $s1 'ASTRA.exe'
        $sig1 = Sign-Payload "1.1.0`n$sha1`n$base/app-1.1.0.exe"
        $payloadB64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(
            "1.1.0`n$sha1`n$base/app-1.1.0.exe"))
        $waiter = Start-Process cmd -ArgumentList '/c exit 0' -PassThru
        $p = Start-Process $updater -ArgumentList @(
            '--wait-pid', $waiter.Id, '--input', $staged, '--target', $target,
            '--expect-sha256', $sha1, '--payload', $payloadB64,
            '--signature', $sig1, '--run-after') `
            -WorkingDirectory $s1 -Wait -PassThru
        Ok 'S1 updater exits 0' ($p.ExitCode -eq 0)
        Ok 'S1 target swapped to new bytes' (
            ((Get-FileHash -Path $target -Algorithm SHA256).Hash).ToLower() -eq $sha1)
        Ok 'S1 backup cleaned after verified swap' (-not (Test-Path "$target.bak"))
        Start-Sleep -Seconds 2
        Ok 'S1 restarted marker proves relaunch' (Test-Path (Join-Path $s1 'restarted.txt'))
    } finally { Stop-FixtureServer $srv }

    # ================= S2: interrupted download =================
    Write-Host '--- S2: interrupted download ---'
    $s2 = New-ScenarioDir 's2'
    $sha2 = New-Binary (Join-Path $s2 'app-1.1.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s2 'truncate'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s2 '1.1.0' "$base/app-1.1.0.exe" $sha2 `
            (Sign-Payload "1.1.0`n$sha2`n$base/app-1.1.0.exe") $false '' '' ''
        $out = & $driver full --url "$base/manifest.json" --expect-terminal Error 2>&1
        Ok 'S2 truncated body ends in Error' ($LASTEXITCODE -eq 0) ($out -join "`n")
        Ok 'S2 partial staging deleted' (-not (Test-Path (
            Join-Path ([IO.Path]::GetTempPath()) 'ASTRA-update\ASTRA-update.exe')))
    } finally { Stop-FixtureServer $srv }

    # ================= S9: recovery after failure =================
    # A client that just failed a download must reach Ready on a clean retry
    # (no poisoned worker/manifest state).
    Write-Host '--- S9: recovery after failure ---'
    $s9 = New-ScenarioDir 's9'
    $sha9 = New-Binary (Join-Path $s9 'app-1.1.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s9 'normal'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s9 '1.1.0' "$base/app-1.1.0.exe" $sha9 `
            (Sign-Payload "1.1.0`n$sha9`n$base/app-1.1.0.exe") $false '' '' ''
        $out = & $driver full --url "$base/manifest.json" --expect-terminal Ready `
            --expect-version '1.1.0' 2>&1
        Ok 'S9 clean retry after failure reaches Ready' ($LASTEXITCODE -eq 0) ($out -join "`n")
    } finally { Stop-FixtureServer $srv }

    # ================= S3: tampered manifest =================
    Write-Host '--- S3: tampered manifest ---'
    $s3 = New-ScenarioDir 's3'
    $sha3 = New-Binary (Join-Path $s3 'app-1.1.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s3 'normal'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s3 '1.1.0' "$base/app-1.1.0.exe" $sha3 `
            (Sign-Payload "1.1.0`n$sha3`n$base/app-1.1.0.exe") $false '' '' ''
        [IO.File]::WriteAllText((Join-Path $s3 'manifest.json'),
            ((Get-Content (Join-Path $s3 'manifest.json') -Raw) -replace '"1\.1\.0"', '"1.1.9"'))
        $out = & $driver check --url "$base/manifest.json" --expect-state Error 2>&1
        Ok 'S3 version flip after signing is rejected' ($LASTEXITCODE -eq 0) ($out -join "`n")
    } finally { Stop-FixtureServer $srv }

    # ================= S4: forged signature =================
    Write-Host '--- S4: forged signature ---'
    $s4 = New-ScenarioDir 's4'
    $sha4 = New-Binary (Join-Path $s4 'app-1.1.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s4 'normal'
        $base = "https://localhost:$($srv.Port)"
        $fake = [Convert]::ToBase64String((,([byte]7) * 256))
        Write-Manifest $s4 '1.1.0' "$base/app-1.1.0.exe" $sha4 $fake $false '' '' ''
        $out = & $driver check --url "$base/manifest.json" --expect-state Error 2>&1
        Ok 'S4 forged signature is rejected' ($LASTEXITCODE -eq 0) ($out -join "`n")
    } finally { Stop-FixtureServer $srv }

    # ================= S5: older remote =================
    Write-Host '--- S5: older remote ---'
    $s5 = New-ScenarioDir 's5'
    $sha5 = New-Binary (Join-Path $s5 'app-0.9.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s5 'normal'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s5 '0.9.0' "$base/app-0.9.0.exe" $sha5 `
            (Sign-Payload "0.9.0`n$sha5`n$base/app-0.9.0.exe") $false '' '' ''
        $out = & $driver check --url "$base/manifest.json" --expect-state UpToDate `
            --expect-version '0.9.0' 2>&1
        Ok 'S5 older remote reports UpToDate (no downgrade)' ($LASTEXITCODE -eq 0) ($out -join "`n")
    } finally { Stop-FixtureServer $srv }

    # ================= S6: swap failure keeps old build =================
    Write-Host '--- S6: swap failure ---'
    $s6 = New-ScenarioDir 's6'
    $sha6 = New-Binary (Join-Path $s6 'staged.exe')
    'APP-v1.0.0-installed-dummy' | Out-File (Join-Path $s6 'ASTRA.exe') -Encoding ascii -NoNewline
    $before = (Get-FileHash (Join-Path $s6 'ASTRA.exe') -Algorithm SHA256).Hash
    $payload6 = "9.9.9`n$sha6`nhttps://example.com/x.exe"
    $payloadB64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($payload6))
    $lock = [IO.File]::Open((Join-Path $s6 'ASTRA.exe'),
        [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $waiter = Start-Process cmd -ArgumentList '/c exit 0' -PassThru
        $p = Start-Process $updater -ArgumentList @(
            '--wait-pid', $waiter.Id, '--input', (Join-Path $s6 'staged.exe'),
            '--target', (Join-Path $s6 'ASTRA.exe'),
            '--expect-sha256', $sha6, '--payload', $payloadB64,
            '--signature', (Sign-Payload $payload6)) `
            -Wait -PassThru
        Ok 'S6 locked target fails the install' ($p.ExitCode -ne 0)
    } finally { $lock.Close() }
    Ok 'S6 installed bytes untouched' (
        (Get-FileHash (Join-Path $s6 'ASTRA.exe') -Algorithm SHA256).Hash -eq $before)

    # ================= S7: core lib (install paths + rollback) =================
    Write-Host '--- S7: install core + rollback ---'
    $s7 = New-ScenarioDir 's7'
    'APP-v1.1.0-staged-content' | Out-File (Join-Path $s7 'probe.bin') -Encoding ascii -NoNewline
    $shaB = ((Get-FileHash (Join-Path $s7 'probe.bin') -Algorithm SHA256).Hash).ToLower()
    $payload7 = "9.9.9`n$shaB`nhttps://example.com/x.exe"
    $payloadB64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($payload7))
    $waiter = Start-Process cmd -ArgumentList '/c exit 0' -PassThru
    $out = & $driver corelib --dir $s7 --wait-pid $waiter.Id --sha $shaB `
        --payload-b64 $payloadB64 --sig-b64 (Sign-Payload $payload7) 2>&1
    Ok 'S7 install/tamper/fakesig/rollback core paths' ($LASTEXITCODE -eq 0) ($out -join "`n")

    # ================= S8: mandatory update =================
    Write-Host '--- S8: mandatory update ---'
    $s8 = New-ScenarioDir 's8'
    $sha8 = New-Binary (Join-Path $s8 'app-1.1.0.exe')
    $srv = $null
    try {
        $srv = Start-FixtureServer $s8 'normal'
        $base = "https://localhost:$($srv.Port)"
        Write-Manifest $s8 '1.1.0' "$base/app-1.1.0.exe" $sha8 `
            (Sign-Payload "1.1.0`n$sha8`n$base/app-1.1.0.exe") $true '' '' ''
        $out = & $driver check --url "$base/manifest.json" --expect-state Available `
            --expect-version '1.1.0' --expect-mandatory 1 2>&1
        Ok 'S8 mandatory flag blocks the panel gate' ($LASTEXITCODE -eq 0) ($out -join "`n")
    } finally { Stop-FixtureServer $srv }
} finally {
    Get-Job | Stop-Job -ErrorAction SilentlyContinue | Out-Null
    Get-Job | Remove-Job -Force -ErrorAction SilentlyContinue | Out-Null
    # Cert cleanup is best-effort and only for certs this run created:
    # removing from Root can raise a consent prompt in interactive sessions,
    # so never let it mask test results. Reused certs are left in place.
    if ($script:certOwned) {
        try {
            $store = New-Object Security.Cryptography.X509Certificates.X509Store('Root', 'CurrentUser')
            $store.Open('ReadWrite')
            $store.Remove($cert)
            $store.Close()
        } catch { Write-Host 'WARN: localhost test cert left in CurrentUser\Root (remove manually).' }
        try {
            $store = New-Object Security.Cryptography.X509Certificates.X509Store('My', 'CurrentUser')
            $store.Open('ReadWrite')
            $store.Remove($cert)
            $store.Close()
        } catch { Write-Host 'WARN: localhost test cert left in CurrentUser\My (remove manually).' }
    }
    Remove-Item Env:\CURL_CA_BUNDLE -ErrorAction SilentlyContinue
}

Write-Host '================ E2E SUMMARY ================'
if ($script:failures.Count -eq 0) { Write-Host 'ALL E2E SCENARIOS PASSED'; exit 0 }
else {
    Write-Host ("FAILURES: " + ($script:failures -join ', ')) -ForegroundColor Red
    exit 1
}
