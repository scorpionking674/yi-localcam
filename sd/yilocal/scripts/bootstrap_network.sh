#!/bin/sh
# YiLocal v1.0.0 network bootstrap
# SD/RAM only. No SPI NOR writes and no writes to /backup.
# Supports explicit DHCP or STATIC LAN configuration from SD.

BASE=/tmp/sd/yilocal
LOG="$BASE/network.log"
WPA=/backup/tools/wpa_supplicant
WPACLI=/home/base/tools/wpa_cli
ACTIVE=/tmp/yilocal_active_wpa.conf
DHCP_HOOK="$BASE/scripts/yilocal_udhcpc.sh"
SDCONF="$BASE/config/wifi.conf"
NETCONF="$BASE/config/network.conf"

mkdir -p "$BASE/config"

log() { echo "$*" >> "$LOG"; }
get_ip() { ifconfig wlan0 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p' | sed -n '1p'; }
get_mac() { ifconfig wlan0 2>/dev/null | sed -n 's/.*HWaddr \([^ ]*\).*/\1/p' | sed -n '1p'; }
get_ssid() { "$WPACLI" -i wlan0 status 2>/dev/null | sed -n 's/^ssid=//p' | sed -n '1p'; }
associated() { "$WPACLI" -i wlan0 status 2>/dev/null | grep -q '^wpa_state=COMPLETED$'; }

read_net() {
    KEY="$1"
    sed -n "s/^[[:space:]]*$KEY[[:space:]]*=[[:space:]]*//p" "$NETCONF" 2>/dev/null | sed -n '1p'
}

valid_ipv4() {
    echo "$1" | grep -Eq '^[0-9]{1,3}(\.[0-9]{1,3}){3}$'
}

wait_wlan() {
    N=0
    while [ "$N" -lt 20 ]; do
        ifconfig wlan0 >/dev/null 2>&1 && return 0
        N=$((N + 1))
        sleep 1
    done
    return 1
}

wait_assoc() {
    N=0
    while [ "$N" -lt 35 ]; do
        STATUS=$("$WPACLI" -i wlan0 status 2>/dev/null)
        STATE=$(echo "$STATUS" | sed -n 's/^wpa_state=//p' | sed -n '1p')
        SSID=$(echo "$STATUS" | sed -n 's/^ssid=//p' | sed -n '1p')
        log "association[$N] state=$STATE ssid=$SSID"
        echo "$STATUS" | grep -q '^wpa_state=COMPLETED$' && return 0
        N=$((N + 1))
        sleep 1
    done
    return 1
}

wait_ip() {
    N=0
    while [ "$N" -lt 35 ]; do
        IP=$(get_ip)
        if [ -n "$IP" ] && [ "$IP" != "0.0.0.0" ]; then return 0; fi
        N=$((N + 1))
        sleep 1
    done
    return 1
}

valid_sd_config() {
    [ -f "$SDCONF" ] || return 1
    grep -q '^[[:space:]]*network=' "$SDCONF" 2>/dev/null || return 1
    grep -q '^[[:space:]]*ssid=' "$SDCONF" 2>/dev/null || return 1
    grep -q '^[[:space:]]*psk=' "$SDCONF" 2>/dev/null || return 1
    return 0
}

start_wpa() {
    CONF="$1"
    killall yilocal_dhcpd 2>/dev/null
    killall udhcpc 2>/dev/null
    killall wpa_supplicant 2>/dev/null
    sleep 1
    ifconfig wlan0 down 2>/dev/null
    sleep 1
    ifconfig wlan0 up 2>/dev/null
    sleep 1
    mkdir -p /var/run/wpa_supplicant
    log "Starting wpa_supplicant using $CONF"
    "$WPA" -B -i wlan0 -c "$CONF" >> "$LOG" 2>&1
    RC=$?
    log "wpa_supplicant exit=$RC"
    return "$RC"
}

clear_ipv4() {
    killall udhcpc 2>/dev/null
    while route del default 2>/dev/null; do :; done
    ifconfig wlan0 0.0.0.0 2>/dev/null
}

start_dhcp() {
    clear_ipv4
    rm -f /tmp/yilocal_ip /tmp/yilocal_gateway
    log "Starting YiLocal DHCP client"
    udhcpc -i wlan0 -b -s "$DHCP_HOOK" >> "$LOG" 2>&1
}

apply_static() {
    STATIC_IP=$(read_net IP)
    NETMASK=$(read_net NETMASK)
    GATEWAY=$(read_net GATEWAY)
    DNS=$(read_net DNS)
    [ -z "$NETMASK" ] && NETMASK=255.255.255.0

    if ! valid_ipv4 "$STATIC_IP" || ! valid_ipv4 "$NETMASK" || ! valid_ipv4 "$GATEWAY"; then
        log "ERROR: invalid static network.conf values"
        return 1
    fi

    clear_ipv4
    log "Applying static IPv4: IP=$STATIC_IP NETMASK=$NETMASK GATEWAY=$GATEWAY"
    ifconfig wlan0 "$STATIC_IP" netmask "$NETMASK" up >> "$LOG" 2>&1 || return 1
    route add default gw "$GATEWAY" wlan0 >> "$LOG" 2>&1 || return 1

    echo "$STATIC_IP" > /tmp/yilocal_ip
    echo "$GATEWAY" > /tmp/yilocal_gateway
    if [ -n "$DNS" ]; then
        echo "nameserver $DNS" > /tmp/yilocal_resolv.conf
    fi

    ping -c 1 -W 2 "$GATEWAY" >/dev/null 2>&1
    if [ $? -eq 0 ]; then log "Static gateway responds: $GATEWAY"; else log "WARNING: static gateway did not answer ICMP; leaving static config active"; fi
    return 0
}

apply_lan_config() {
    NETMODE=$(read_net MODE)
    [ -z "$NETMODE" ] && NETMODE=DHCP
    case "$NETMODE" in
        STATIC|static)
            if apply_static; then
                echo "CONNECTED" > "$BASE/NETWORK_STATE.txt"
                echo "STATIC" > "$BASE/ADDRESS_MODE.txt"
                log "Static address active: $(get_ip)"
                return 0
            fi
            return 1
            ;;
        *)
            start_dhcp
            if wait_ip; then
                echo "CONNECTED" > "$BASE/NETWORK_STATE.txt"
                echo "DHCP" > "$BASE/ADDRESS_MODE.txt"
                log "DHCP address: $(get_ip)"
                return 0
            fi
            return 1
            ;;
    esac
}

start_provision_ap() {
    MAC=$(get_mac)
    SUFFIX=$(echo "$MAC" | awk -F: '{print $4 $5 $6}')
    [ -z "$SUFFIX" ] && SUFFIX="SETUP"
    APSSID="YiLocal-$SUFFIX"
    APCONF=/tmp/yilocal_ap.conf

    cat > "$APCONF" <<EOC
ctrl_interface=/var/run/wpa_supplicant
ap_scan=2
network={
    ssid="$APSSID"
    mode=2
    frequency=2437
    key_mgmt=WPA-PSK
    proto=RSN
    pairwise=CCMP
    psk="yilocalsetup"
}
EOC

    log "Trying provisioning AP: $APSSID"
    killall yilocal_dhcpd 2>/dev/null
    killall udhcpc 2>/dev/null
    killall wpa_supplicant 2>/dev/null
    sleep 1
    ifconfig wlan0 down 2>/dev/null
    sleep 1
    ifconfig wlan0 up 2>/dev/null
    sleep 1
    mkdir -p /var/run/wpa_supplicant
    "$WPA" -B -i wlan0 -c "$APCONF" >> "$LOG" 2>&1
    AP_RC=$?
    log "AP wpa_supplicant exit=$AP_RC"
    sleep 4
    ifconfig wlan0 192.168.10.1 netmask 255.255.255.0 up 2>/dev/null
    while route del default 2>/dev/null; do :; done
    cp "$APCONF" "$ACTIVE" 2>/dev/null
    echo "$APSSID" > "$BASE/SETUP_AP_SSID.txt"
    echo "yilocalsetup" > "$BASE/SETUP_AP_PASSWORD.txt"
    echo "192.168.10.1" > "$BASE/CURRENT_IP.txt"
    echo "AP" > "$BASE/ADDRESS_MODE.txt"
    "$BASE/bin/yilocal_dhcpd" >> "$BASE/dhcpd.log" 2>&1 &
    log "Provisioning AP attempted: SSID=$APSSID WebUI=http://192.168.10.1/"
}

: > "$LOG"
log "===== YiLocal v1.0.0 network bootstrap $(date) ====="
log "Explicit SD Wi-Fi + static/DHCP LAN mode. Vendor PSK migration disabled."
[ -w /proc/sys/net/ipv4/icmp_echo_ignore_all ] && echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_all

if ! wait_wlan; then
    log "ERROR: wlan0 never appeared."
    echo "NO_WLAN" > "$BASE/NETWORK_STATE.txt"
    exit 1
fi

{
    echo "--- initial wpa status ---"
    "$WPACLI" -i wlan0 status 2>/dev/null
    echo "--- initial ifconfig ---"
    ifconfig wlan0 2>/dev/null
    echo "--- initial route ---"
    route -n 2>/dev/null
} >> "$LOG"

MODE=""
if valid_sd_config; then
    MODE="SD_CONFIG"
    cp "$SDCONF" "$ACTIVE"
    log "Using explicit YiLocal SD-card Wi-Fi configuration."
    start_wpa "$ACTIVE"
    if wait_assoc; then
        log "Association complete: $(get_ssid)"
        if ! apply_lan_config; then
            log "ERROR: association succeeded but LAN addressing failed."
            echo "ADDRESS_FAILED" > "$BASE/NETWORK_STATE.txt"
            start_provision_ap
            MODE="AP_FALLBACK"
        fi
    else
        log "ERROR: explicit SD Wi-Fi configuration did not associate."
        echo "ASSOCIATION_FAILED" > "$BASE/NETWORK_STATE.txt"
        start_provision_ap
        MODE="AP_FALLBACK"
    fi
elif associated && [ -n "$(get_ip)" ]; then
    MODE="EXISTING_ASSOCIATION"
    log "Camera already had a working association; leaving it in place."
    echo "CONNECTED" > "$BASE/NETWORK_STATE.txt"
else
    MODE="AP_ONLY"
    log "No YiLocal wifi.conf and no working association."
    echo "NO_SD_WIFI_CONFIG" > "$BASE/NETWORK_STATE.txt"
    start_provision_ap
fi

IP=$(get_ip)
SSID=$(get_ssid)
MAC=$(get_mac)
[ -n "$IP" ] && echo "$IP" > "$BASE/CURRENT_IP.txt"
[ -n "$MAC" ] && echo "$MAC" > "$BASE/MAC.txt"
echo "$SSID" > "$BASE/CURRENT_SSID.txt"
echo "$MODE" > "$BASE/NETWORK_MODE.txt"

{
    echo "===== YiLocal v1.0.0 network result ====="
    date
    echo "MODE=$MODE"
    echo "ADDRESS_MODE=$(cat "$BASE/ADDRESS_MODE.txt" 2>/dev/null)"
    echo "SSID=$SSID"
    echo "IP=$IP"
    echo "MAC=$MAC"
    echo "STATE=$(cat "$BASE/NETWORK_STATE.txt" 2>/dev/null)"
    echo "--- configured network.conf (no Wi-Fi password here) ---"
    cat "$NETCONF" 2>/dev/null
    echo "--- wpa status ---"
    "$WPACLI" -i wlan0 status 2>/dev/null | grep -E 'bssid=|freq=|ssid=|mode=|pairwise_cipher=|group_cipher=|key_mgmt=|wpa_state=|ip_address=|address='
    echo "--- ifconfig ---"
    ifconfig wlan0 2>/dev/null
    echo "--- route ---"
    route -n 2>/dev/null
    echo "--- SD wifi.conf present ---"
    [ -f "$SDCONF" ] && echo YES || echo NO
    echo "NOTE: Wi-Fi password is intentionally not printed in this diagnostic."
} > "$BASE/NETWORK_DIAG.txt" 2>&1

exit 0
