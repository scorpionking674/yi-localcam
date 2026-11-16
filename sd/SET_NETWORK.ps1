# YiLocal network setup helper for Windows PowerShell 5+
# Run from the ROOT of the SD card before putting the card in the camera.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$configDir = Join-Path $root 'yilocal\config'
$wifiFile = Join-Path $configDir 'wifi.conf'
$netFile = Join-Path $configDir 'network.conf'

Write-Host ''
Write-Host 'YiLocal network setup'
Write-Host ''

$ssid = Read-Host 'Wi-Fi SSID (must be 2.4 GHz)'
if ([string]::IsNullOrWhiteSpace($ssid)) { throw 'SSID cannot be empty.' }
$secure = Read-Host 'Wi-Fi password' -AsSecureString
$bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
try { $psk = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr) }
finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) }
if ($psk.Length -lt 8 -or $psk.Length -gt 64) { throw 'WPA/WPA2 password must be 8-63 characters, or exactly 64 hexadecimal characters.' }

Write-Host 'Static IPv4 address, or press Enter for DHCP.'
$ip = Read-Host 'Static IP [Enter = DHCP]'
$ipMode = 'DHCP'
if (-not [string]::IsNullOrWhiteSpace($ip)) {
    $ipMode = 'STATIC'
    $mask = Read-Host 'Netmask [255.255.255.0]'
    if ([string]::IsNullOrWhiteSpace($mask)) { $mask = '255.255.255.0' }
    $gw = Read-Host 'Gateway'
    if ([string]::IsNullOrWhiteSpace($gw)) { throw 'Gateway is required for static mode.' }
    $dns = Read-Host 'DNS [$gw]'
    if ([string]::IsNullOrWhiteSpace($dns)) { $dns = $gw }
} else {
    $mask = '255.255.255.0'; $gw = ''; $dns = ''
}

function Test-IPv4([string]$s) {
    $a = $null
    if (-not [System.Net.IPAddress]::TryParse($s,[ref]$a)) { return $false }
    return $a.AddressFamily -eq [System.Net.Sockets.AddressFamily]::InterNetwork
}
$toCheck = @($mask)
if ($ipMode -eq 'STATIC') { $toCheck = @($ip,$gw,$dns) + $toCheck }
foreach ($x in $toCheck) { if (-not (Test-IPv4 $x)) { throw "Invalid IPv4 value: $x" } }

function Escape-WpaQuoted([string]$s) { return $s.Replace('\','\\').Replace('"','\"') }
$ssidEsc = Escape-WpaQuoted $ssid
if ($psk -match '^[0-9A-Fa-f]{64}$') { $pskLine = "    psk=$psk" }
else { $pskEsc = Escape-WpaQuoted $psk; $pskLine = "    psk=`"$pskEsc`"" }

New-Item -ItemType Directory -Force -Path $configDir | Out-Null
$wifi = @"
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
network={
    ssid="$ssidEsc"
    scan_ssid=1
    proto=WPA RSN
    key_mgmt=WPA-PSK
    pairwise=CCMP TKIP
    group=CCMP TKIP
$pskLine
}
"@
if ($ipMode -eq 'STATIC') {
    $net = @"
MODE=STATIC
IP=$ip
NETMASK=$mask
GATEWAY=$gw
DNS=$dns
"@
} else {
    $net = "MODE=DHCP"
}
[IO.File]::WriteAllText($wifiFile,$wifi,[Text.Encoding]::ASCII)
[IO.File]::WriteAllText($netFile,$net,[Text.Encoding]::ASCII)

# Rewrite the included Frigate starter config to the chosen static IP.
$frigate = Join-Path $root 'FRIGATE.example.yml'
if (Test-Path $frigate) {
    $f = [IO.File]::ReadAllText($frigate)
    $repl = if ($ipMode -eq 'STATIC') { $ip } else { '<CAM_IP>' }
    $f = [regex]::Replace($f,'rtsp://(?:<CAM_IP>|(?:\d{1,3}\.){3}\d{1,3}):8554/','rtsp://' + $repl + ':8554/')
    [IO.File]::WriteAllText($frigate,$f,[Text.Encoding]::ASCII)
}

Write-Host ''
Write-Host "Saved Wi-Fi config: $wifiFile"
Write-Host "Saved LAN config:   $netFile"
Write-Host "SSID:               $ssid"
Write-Host "Address mode:       $ipMode"
if ($ipMode -eq 'STATIC') { Write-Host "Static IP:          $ip" }
Write-Host 'Password was NOT printed.'
Write-Host ''
if ($ipMode -eq 'STATIC') {
    Write-Warning "Make sure $ip is not already used by another device."
    Write-Host "After boot:  http://$ip/"
    Write-Host "Main RTSP:   rtsp://$ip`:8554/main"
    Write-Host "Sub RTSP:    rtsp://$ip`:8554/sub"
} else {
    Write-Host 'After boot, find the camera address in your router DHCP table,'
    Write-Host 'or read yilocal/CURRENT_IP.txt written back to the SD card.'
}
