#!/bin/sh
# YiLocal v1.0.0 WebUI Wi-Fi manager.
# Wi-Fi changes keep the address mode in config/network.conf.
BASE=/tmp/sd/yilocal
WPA=/backup/tools/wpa_supplicant
WPACLI=/home/base/tools/wpa_cli
ACTIVE=/tmp/yilocal_active_wpa.conf
DHCP_HOOK="$BASE/scripts/yilocal_udhcpc.sh"
LOG="$BASE/network.log"
RESULT=/tmp/yilocal_wifi_result.json
SCAN=/tmp/yilocal_wifi_scan.txt
NETCONF="$BASE/config/network.conf"

mkdir -p "$BASE/config"
json_result() { STATE="$1"; MSG="$2"; echo "{\"state\":\"$STATE\",\"message\":\"$MSG\"}" > "$RESULT"; }
get_ip() { ifconfig wlan0 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p' | sed -n '1p'; }
read_net() { KEY="$1"; sed -n "s/^[[:space:]]*$KEY[[:space:]]*=[[:space:]]*//p" "$NETCONF" 2>/dev/null | sed -n '1p'; }
wait_assoc() { N=0; while [ "$N" -lt 25 ]; do S=$("$WPACLI" -i wlan0 status 2>/dev/null); echo "$S" | grep -q '^wpa_state=COMPLETED$' && return 0; N=$((N+1)); sleep 1; done; return 1; }
wait_ip() { N=0; while [ "$N" -lt 25 ]; do IP=$(get_ip); [ -n "$IP" ] && [ "$IP" != "0.0.0.0" ] && { echo "$IP"; return 0; }; N=$((N+1)); sleep 1; done; return 1; }
start_wpa_file() { CONF="$1"; killall yilocal_dhcpd 2>/dev/null; killall udhcpc 2>/dev/null; killall wpa_supplicant 2>/dev/null; sleep 1; ifconfig wlan0 up 2>/dev/null; mkdir -p /var/run/wpa_supplicant; "$WPA" -B -i wlan0 -c "$CONF" >> "$LOG" 2>&1; }
clear_ipv4() { killall udhcpc 2>/dev/null; while route del default 2>/dev/null; do :; done; ifconfig wlan0 0.0.0.0 2>/dev/null; }
start_dhcp() { clear_ipv4; rm -f /tmp/yilocal_ip; udhcpc -i wlan0 -b -s "$DHCP_HOOK" >> "$LOG" 2>&1; }
apply_static() { IP=$(read_net IP); MASK=$(read_net NETMASK); GW=$(read_net GATEWAY); [ -z "$MASK" ] && MASK=255.255.255.0; [ -n "$IP" ] && [ -n "$GW" ] || return 1; clear_ipv4; ifconfig wlan0 "$IP" netmask "$MASK" up >> "$LOG" 2>&1 || return 1; route add default gw "$GW" wlan0 >> "$LOG" 2>&1 || return 1; echo "$IP" > "$BASE/CURRENT_IP.txt"; return 0; }
apply_addressing() { M=$(read_net MODE); if [ "$M" = "STATIC" ] || [ "$M" = "static" ]; then apply_static; return $?; fi; start_dhcp; IP=$(wait_ip); [ -n "$IP" ]; }
restore_previous() { [ -f /tmp/yilocal_wpa.rollback ] || return; cp /tmp/yilocal_wpa.rollback "$ACTIVE"; start_wpa_file "$ACTIVE"; if grep -q 'mode=2' "$ACTIVE" 2>/dev/null; then sleep 2; ifconfig wlan0 192.168.10.1 netmask 255.255.255.0 up; "$BASE/bin/yilocal_dhcpd" >> "$BASE/dhcpd.log" 2>&1 & else wait_assoc && apply_addressing; fi; }
enable_local_media() {
  [ -x "$BASE/bin/yilocal_rmm_local_enable" ] || return 1
  "$BASE/bin/yilocal_rmm_local_enable" >> "$BASE/media_enable.log" 2>&1 || return 1
  sleep 2
  [ -x "$BASE/compat/yi_ipc_buffer_trigger" ] && "$BASE/compat/yi_ipc_buffer_trigger" >> "$BASE/media_enable.log" 2>&1
  if [ -x "$BASE/bin/yilocal_ring_probe" ]; then
    "$BASE/bin/yilocal_ring_probe" >> "$BASE/media_enable.log" 2>&1
    if [ $? -eq 0 ]; then echo ACTIVE > "$BASE/MEDIA_RING_STATE.txt"; else echo ALL_ZERO > "$BASE/MEDIA_RING_STATE.txt"; fi
  fi
}
apply_request() {
  SSID=$(cat /tmp/yilocal_ssid.req 2>/dev/null); PSK=$(cat /tmp/yilocal_psk.req 2>/dev/null)
  rm -f /tmp/yilocal_ssid.req /tmp/yilocal_psk.req /tmp/yilocal_wifi_connect.req
  [ -n "$SSID" ] || { json_result error "SSID was empty"; return; }
  [ -n "$PSK" ] || { json_result error "Password was empty"; return; }
  json_result connecting "Associating with requested Wi-Fi network"
  cp "$ACTIVE" /tmp/yilocal_wpa.rollback 2>/dev/null
  cat > /tmp/yilocal_wpa.new <<EOC
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
network={
    ssid="$SSID"
    scan_ssid=1
    proto=WPA RSN
    key_mgmt=WPA-PSK
    pairwise=CCMP TKIP
    group=CCMP TKIP
    psk="$PSK"
}
EOC
  start_wpa_file /tmp/yilocal_wpa.new
  if ! wait_assoc; then json_result error "Wi-Fi association failed; restoring previous network"; restore_previous; return; fi
  if ! apply_addressing; then json_result error "Associated, but LAN addressing failed; restoring previous network"; restore_previous; return; fi
  IP=$(get_ip)
  cp /tmp/yilocal_wpa.new "$ACTIVE"
  cp /tmp/yilocal_wpa.new "$BASE/config/wifi.conf"
  [ -n "$IP" ] && echo "$IP" > "$BASE/CURRENT_IP.txt"
  enable_local_media
  json_result connected "Connected successfully; Wi-Fi saved on SD and existing static/DHCP mode retained"
}
scan_wifi() { rm -f /tmp/yilocal_wifi_scan.req; "$WPACLI" -i wlan0 scan >/dev/null 2>&1; sleep 3; "$WPACLI" -i wlan0 scan_results > "$SCAN" 2>/dev/null; }
json_result idle "Network manager ready"
while true; do [ -f /tmp/yilocal_wifi_scan.req ] && scan_wifi; [ -f /tmp/yilocal_wifi_connect.req ] && apply_request; sleep 1; done
