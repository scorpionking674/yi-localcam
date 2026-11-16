#!/bin/sh
BASE=/tmp/sd/yilocal
OUT=/tmp/yilocal_status.json
WPACLI=/home/base/tools/wpa_cli
jesc() { echo "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'; }
proc_running() { P="$1"; ps | grep "$P" | grep -v grep >/dev/null 2>&1; }
while true; do
  W=$("$WPACLI" -i wlan0 status 2>/dev/null)
  SSID=$(echo "$W" | sed -n 's/^ssid=//p' | sed -n '1p')
  STATE=$(echo "$W" | sed -n 's/^wpa_state=//p' | sed -n '1p')
  BSSID=$(echo "$W" | sed -n 's/^bssid=//p' | sed -n '1p')
  IP=$(ifconfig wlan0 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p' | sed -n '1p')
  MAC=$(ifconfig wlan0 2>/dev/null | sed -n 's/.*HWaddr \([^ ]*\).*/\1/p' | sed -n '1p')
  SIG=$(iwconfig wlan0 2>/dev/null | sed -n 's/.*Signal level[=:]\([^ ]*\).*/\1/p' | sed -n '1p'); [ -z "$SIG" ] && SIG="unknown"
  UP=$(cut -d. -f1 /proc/uptime 2>/dev/null); [ -z "$UP" ] && UP=0
  ADDRMODE=$(cat "$BASE/ADDRESS_MODE.txt" 2>/dev/null); [ -z "$ADDRMODE" ] && ADDRMODE="unknown"
  GW=$(route -n 2>/dev/null | awk '$1=="0.0.0.0" {print $2; exit}')
  if proc_running '[y]ilocal_rtspd'; then RTSP=true; else RTSP=false; fi
  if proc_running '[y]ilocal_webd'; then WEB=true; else WEB=false; fi
  if proc_running '[r]mm'; then RMM=true; else RMM=false; fi
  if proc_running '[d]ispatch'; then DISPATCH=true; else DISPATCH=false; fi
  if ps | grep 'onboarding_supp' | grep -v grep >/dev/null 2>&1; then SUPPRESS=true; else SUPPRESS=false; fi
  RINGSTATE=$(cat "$BASE/MEDIA_RING_STATE.txt" 2>/dev/null)
  if [ "$RINGSTATE" = "ACTIVE" ]; then FSHARE=true; else FSHARE=false; fi
  YI=0
  for p in cloud p2p_tnp oss rtmp mp4record watch_process cloudAPI upgrade_firmware log_server telnetd; do ps | grep "$p" | grep -v grep >/dev/null 2>&1 && YI=$((YI + 1)); done
  VER=$(cat /home/app/.appver 2>/dev/null); [ -z "$VER" ] && VER=$(cat /home/homever 2>/dev/null)
  TEMP=/tmp/yilocal_status.$$
  echo "{\"ok\":true,\"name\":\"YiLocal\",\"version\":\"v1.0.0\",\"vendor_firmware\":\"$(jesc "$VER")\",\"wifi_state\":\"$(jesc "$STATE")\",\"ssid\":\"$(jesc "$SSID")\",\"bssid\":\"$(jesc "$BSSID")\",\"ip\":\"$(jesc "$IP")\",\"mac\":\"$(jesc "$MAC")\",\"signal\":\"$(jesc "$SIG")\",\"address_mode\":\"$(jesc "$ADDRMODE")\",\"gateway\":\"$(jesc "$GW")\",\"uptime_s\":$UP,\"web\":$WEB,\"rtsp\":$RTSP,\"rmm\":$RMM,\"dispatch\":$DISPATCH,\"onboarding_suppression\":$SUPPRESS,\"fshare\":$FSHARE,\"yi_network_processes\":$YI,\"main\":\"1920x1080 H.264 + AAC 16k mono\",\"sub\":\"640x360 H.264\"}" > "$TEMP"
  mv "$TEMP" "$OUT"
  [ -n "$IP" ] && echo "$IP" > "$BASE/CURRENT_IP.txt"
  sleep 2
done
