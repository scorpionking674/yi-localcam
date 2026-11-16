#!/bin/sh
BASE=/tmp/sd/yilocal
while true; do
  if [ -f /tmp/yilocal_restart_rtsp.req ]; then
    rm -f /tmp/yilocal_restart_rtsp.req
    killall yilocal_rtspd 2>/dev/null
    sleep 1
    "$BASE/bin/yilocal_rtspd" > "$BASE/rtspd.log" 2>&1 &
  fi
  if [ -f /tmp/yilocal_reboot.req ]; then
    rm -f /tmp/yilocal_reboot.req
    sync
    sleep 1
    reboot
  fi
  sleep 1
done
