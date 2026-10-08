<#
.SYNOPSIS
  Minimal TLS fixture server for the update E2E (test infrastructure only).
  Serves files from -FixtureDir by basename over https://localhost:<port>/.
  -Mode normal   : full bodies.
  -Mode truncate : .exe bodies are cut in half (simulates interrupted download).
#>
param(
    [Parameter(Mandatory)] [string]$PortFile,
    [Parameter(Mandatory)] [string]$Thumbprint,
    [Parameter(Mandatory)] [string]$FixtureDir,
    [string]$Mode = 'normal',
    [int]$MaxRequests = 60
)
$ErrorActionPreference = 'Stop'
$cert = Get-ChildItem "Cert:\CurrentUser\My\$Thumbprint" -ErrorAction Stop
$listener = New-Object Net.Sockets.TcpListener([Net.IPAddress]::Loopback, 0)
$listener.Start()
$port = ($listener.LocalEndpoint).Port
Set-Content -Path $PortFile -Value "$port" -NoNewline
$count = 0
try {
    while ($count -lt $MaxRequests) {
        $client = $listener.AcceptTcpClient()
        try {
            $client.ReceiveTimeout = 10000
            $ssl = New-Object Net.Security.SslStream($client.GetStream(), $false)
            $ssl.AuthenticateAsServer($cert, $false,
                [System.Security.Authentication.SslProtocols]::Tls12, $false)
            $reader = New-Object IO.StreamReader($ssl)
            $req = $reader.ReadLine()
            if ([string]::IsNullOrEmpty($req)) { continue }
            $line = ''
            while ($line -ne $null -and $line -ne '') { $line = $reader.ReadLine() }
            $path = ($req -split ' ')[1]
            $base = [IO.Path]::GetFileName($path)
            $file = Join-Path $FixtureDir $base
            if (($base -match '\.\.') -or -not (Test-Path $file -PathType Leaf)) {
                $h = 'HTTP/1.1 404 Not Found' + "`r`n" + 'Content-Length: 0' + "`r`n" +
                     'Connection: close' + "`r`n`r`n"
                $hb = [Text.Encoding]::ASCII.GetBytes($h)
                $ssl.Write($hb, 0, $hb.Length)
                continue
            }
            $bytes = [IO.File]::ReadAllBytes($file)
            $ctype = 'application/octet-stream'
            if ($base -like '*.json') { $ctype = 'application/json' }
            $h = 'HTTP/1.1 200 OK' + "`r`n" + "Content-Type: $ctype" + "`r`n" +
                 "Content-Length: $($bytes.Length)" + "`r`n" + 'Connection: close' +
                 "`r`n`r`n"
            $hb = [Text.Encoding]::ASCII.GetBytes($h)
            $ssl.Write($hb, 0, $hb.Length)
            if ($Mode -eq 'truncate' -and $base -notlike '*.json') {
                $half = [Math]::Max(1024, [Math]::Floor($bytes.Length / 2))
                $ssl.Write($bytes, 0, $half)
                # Abrupt close: no TLS close_notify -> client sees a truncated body.
            } else {
                $ssl.Write($bytes, 0, $bytes.Length)
            }
        } catch { } finally {
            try { $client.Close() } catch { }
            $count++
        }
    }
} finally { $listener.Stop() }
