#!/bin/sh
# ============================================================
# YiLocal v1.0.0 - FACTORY-RESET GATE CLEAR / MEDIA BRING-UP
# ============================================================
#
# SD/RAM-only experiment.
# NO raw MTD writes.
# NO SPI NOR erase/program.
# NO writes to /backup.
#
# Reverse-engineering correction from the full camera dump:
#
#   rmm maps /tmp/mmap.info and its startup state machine uses:
#
#       +0x6dc  start_with_reset
#       +0x678  wifi_connected
#
#   When start_with_reset == 1, rmm stays in the factory-reset /
#   onboarding branch and does NOT reach the normal Wi-Fi VENC/AENC
#   path.  v1.0.0 changed only +0x678, which was insufficient.
#
#   v1.0.0 clears start_with_reset to 0 AND sets wifi_connected to 1.
#   Both writes target /tmp/mmap.info on tmpfs/RAM only.
#
#   rmm may then wait up to ~40 seconds on another internal ready field
#   before checking wifi_connected, so the media probe now waits up to
#   70 seconds instead of 15.
#
# IMPORTANT:
#   We still DO NOT send vendor dispatch command 104 because the vendor
#   propagation path can reach persistent configuration writes.
#
# Ports:
#   TCP 80   YiLocal WebUI
#   TCP 8554 YiLocal RTSP (/main, /sub)
#
# Temporary vendor hardware bridge retained:
#   dispatch + rmm
#
# Yi cloud/P2P/update/recording/tserver userspace is killed.
# ============================================================

BASE=/tmp/sd/yilocal
LOG="$BASE/yilocal.log"
RTSPLOG="$BASE/rtspd.log"
WEBLOG="$BASE/webd.log"
MEDIA_LOG="$BASE/media_enable.log"
BOOTSTATUS="$BASE/BOOT_STATUS.txt"
RINGSTATE="$BASE/MEDIA_RING_STATE.txt"

mkdir -p "$BASE" "$BASE/config"
: > "$LOG"
: > "$RTSPLOG"
: > "$WEBLOG"
: > "$MEDIA_LOG"
echo "UNKNOWN" > "$RINGSTATE"

exec >> "$LOG" 2>&1
set +e

if [ -f /tmp/sd/YILOCAL_DISABLE ]; then
    echo "YiLocal disabled by /tmp/sd/YILOCAL_DISABLE"
    exit 0
fi

chmod +x "$BASE/bin/"* "$BASE/compat/"* "$BASE/scripts/"*.sh 2>/dev/null

echo "============================================================"
echo "===== YiLocal v1.0.0 boot $(date) ====="
echo "============================================================"
echo "debug.sh=$0"
echo "SD/RAM-only factory-reset-state bypass."
echo "WebUI TCP/80; RTSP TCP/8554."
echo "Stock tserver is NOT used."

echo
echo "--- SD configuration ---"
[ -f "$BASE/config/wifi.conf" ] && echo "wifi.conf=FOUND" || echo "wifi.conf=MISSING"
[ -f "$BASE/config/network.conf" ] && { echo "network.conf=FOUND"; cat "$BASE/config/network.conf"; } || echo "network.conf=MISSING"

# ------------------------------------------------------------
# Wait for stock dispatch/rmm and the fshare object.
# File existence does NOT imply encoded data is present.
# ------------------------------------------------------------

echo
echo "Waiting for stock dispatch/rmm and fshare object..."
N=0
BRIDGE_READY=0
while [ "$N" -lt 20 ]; do
    HAVE_RMM=0
    HAVE_DISPATCH=0
    HAVE_RING=0

    ps | grep '[r]mm' >/dev/null 2>&1 && HAVE_RMM=1
    ps | grep '[d]ispatch' >/dev/null 2>&1 && HAVE_DISPATCH=1
    [ -e /dev/shm/fshare_frame_buf ] && HAVE_RING=1

    if [ "$HAVE_RMM" = "1" ] && [ "$HAVE_DISPATCH" = "1" ] && [ "$HAVE_RING" = "1" ]; then
        BRIDGE_READY=1
        echo "Vendor bridge + fshare object ready after ${N}s."
        break
    fi

    N=$((N + 1))
    sleep 1
done

[ "$BRIDGE_READY" = "1" ] || echo "WARNING: vendor bridge was not fully ready after 20 seconds."

# ------------------------------------------------------------
# Remove Yi network/cloud/update/recording userspace.
# ------------------------------------------------------------

echo
echo "Stopping Yi network/cloud/update/recording services..."
for p in \
    watch_process \
    upgrade_firmware \
    cloudAPI \
    cloud \
    p2p_tnp \
    oss \
    rtmp \
    mp4record \
    log_server \
    telnetd \
    tserver
do
    echo "  kill $p"
    killall "$p" 2>/dev/null
done
sleep 1

# ------------------------------------------------------------
# Suppress stock onboarding voice/LED behavior in RAM.
# ------------------------------------------------------------

killall onboarding_suppress.sh 2>/dev/null
if [ -x "$BASE/scripts/onboarding_suppress.sh" ]; then
    "$BASE/scripts/onboarding_suppress.sh" >/dev/null 2>&1 &
    echo "Started Yi onboarding suppression manager"
fi

# ------------------------------------------------------------
# YiLocal WebUI owns port 80.
# ------------------------------------------------------------

killall yilocal_webd 2>/dev/null
if [ -x "$BASE/bin/yilocal_webd" ]; then
    "$BASE/bin/yilocal_webd" > "$WEBLOG" 2>&1 &
    echo "Started YiLocal WebUI on TCP/80"
else
    echo "ERROR: missing yilocal_webd"
fi

# ------------------------------------------------------------
# Join configured Wi-Fi and apply static/DHCP LAN address.
# ------------------------------------------------------------

if [ -x "$BASE/scripts/bootstrap_network.sh" ]; then
    "$BASE/scripts/bootstrap_network.sh"
    NET_RC=$?
    echo "bootstrap_network.sh exit=$NET_RC"
else
    NET_RC=127
    echo "ERROR: missing bootstrap_network.sh"
fi

STATE=$(cat "$BASE/NETWORK_STATE.txt" 2>/dev/null)

echo
echo "--- LOCAL RMM FACTORY-RESET / MEDIA STATE ---"
echo "NETWORK_STATE=$STATE"

MEDIA_ENABLE_RC=99
STATE_GUARD_PID=""

if [ "$STATE" = "CONNECTED" ]; then
    if [ -x "$BASE/bin/yilocal_rmm_local_enable" ]; then
        echo "Clearing rmm factory-reset gate + enabling local Wi-Fi media state..."
        "$BASE/bin/yilocal_rmm_local_enable" > "$MEDIA_LOG" 2>&1
        MEDIA_ENABLE_RC=$?
        echo "yilocal_rmm_local_enable exit=$MEDIA_ENABLE_RC"
        cat "$MEDIA_LOG" 2>/dev/null

        # Keep those two RAM-only state fields pinned while rmm exits its
        # onboarding loop and reaches the normal VENC/AENC path.  This also
        # tells us if dispatch tries to restore the reset state.
        if [ "$MEDIA_ENABLE_RC" = "0" ]; then
            (
                I=0
                while [ "$I" -lt 35 ]; do
                    sleep 2
                    "$BASE/bin/yilocal_rmm_local_enable" >> "$MEDIA_LOG" 2>&1
                    I=$((I + 1))
                done
            ) &
            STATE_GUARD_PID=$!
            echo "Started temporary 70-second RAM-state guard pid=$STATE_GUARD_PID"
        fi
    else
        MEDIA_ENABLE_RC=127
        echo "ERROR: missing yilocal_rmm_local_enable"
    fi
else
    echo "Network is not CONNECTED; not changing rmm media state."
fi

# ------------------------------------------------------------
# Ask stock rmm to retain the fshare media buffer.
# Send once now, then once again after encoded data appears.
# ------------------------------------------------------------

IPC_RC=99
if [ "$MEDIA_ENABLE_RC" = "0" ]; then
    if [ -x "$BASE/compat/yi_ipc_buffer_trigger" ]; then
        "$BASE/compat/yi_ipc_buffer_trigger"
        IPC_RC=$?
        echo "Initial media-ring retain IPC exit=$IPC_RC"
    else
        IPC_RC=127
        echo "WARNING: missing yi_ipc_buffer_trigger"
    fi

    # rmm's factory-reset path can take several seconds to exit, and the
    # normal path contains an up-to-40-second wait before VENC/AENC init.
    # Probe for actual non-zero encoded bytes for up to 70 seconds.
    if [ -x "$BASE/bin/yilocal_ring_probe" ]; then
        echo "Waiting up to 70 seconds for real encoded fshare data..."
        "$BASE/bin/yilocal_ring_probe" >> "$MEDIA_LOG" 2>&1
        RING_RC=$?
        echo "yilocal_ring_probe exit=$RING_RC"

        if [ "$RING_RC" = "0" ]; then
            echo "ACTIVE" > "$RINGSTATE"
            echo "Encoded fshare ring is ACTIVE."

            if [ -x "$BASE/compat/yi_ipc_buffer_trigger" ]; then
                "$BASE/compat/yi_ipc_buffer_trigger"
                echo "Post-activation media-ring retain IPC exit=$?"
            fi
        else
            echo "ALL_ZERO" > "$RINGSTATE"
            echo "ERROR: fshare ring remained all-zero after 70 seconds."
        fi
    else
        RING_RC=127
        echo "UNKNOWN" > "$RINGSTATE"
        echo "ERROR: missing yilocal_ring_probe"
    fi
else
    RING_RC=99
    echo "NOT_ENABLED" > "$RINGSTATE"
fi

# Stop the temporary state guard after the probe window.
if [ -n "$STATE_GUARD_PID" ]; then
    kill "$STATE_GUARD_PID" 2>/dev/null
fi

# ------------------------------------------------------------
# Start YiLocal RTSP.  Even if media activation failed, starting
# the daemon leaves useful RTSP diagnostics available.
# ------------------------------------------------------------

killall yilocal_rtspd 2>/dev/null
if [ -e /dev/shm/fshare_frame_buf ] && [ -x "$BASE/bin/yilocal_rtspd" ]; then
    "$BASE/bin/yilocal_rtspd" > "$RTSPLOG" 2>&1 &
    echo "Started YiLocal RTSP on TCP/8554"
else
    echo "ERROR: RTSP not started: binary or fshare_frame_buf missing"
fi

# ------------------------------------------------------------
# Local managers.
# ------------------------------------------------------------

killall network_manager.sh 2>/dev/null
killall status_loop.sh 2>/dev/null
killall system_manager.sh 2>/dev/null

"$BASE/scripts/network_manager.sh" >> "$BASE/network.log" 2>&1 &
"$BASE/scripts/status_loop.sh" >/dev/null 2>&1 &
"$BASE/scripts/system_manager.sh" >/dev/null 2>&1 &

# ------------------------------------------------------------
# Keep unwanted Yi services dead.
# ------------------------------------------------------------

(
    while true; do
        for p in \
            watch_process \
            upgrade_firmware \
            cloudAPI \
            cloud \
            p2p_tnp \
            oss \
            rtmp \
            mp4record \
            log_server \
            telnetd \
            tserver
        do
            killall "$p" 2>/dev/null
        done
        sleep 2
    done
) >/dev/null 2>&1 &

sleep 2

# ------------------------------------------------------------
# Final status.
# ------------------------------------------------------------

IP=$(ifconfig wlan0 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p' | sed -n '1p')
MAC=$(ifconfig wlan0 2>/dev/null | sed -n 's/.*HWaddr \([^ ]*\).*/\1/p' | sed -n '1p')
SSID=$(/home/base/tools/wpa_cli -i wlan0 status 2>/dev/null | sed -n 's/^ssid=//p' | sed -n '1p')
STATE=$(cat "$BASE/NETWORK_STATE.txt" 2>/dev/null)
MODE=$(cat "$BASE/NETWORK_MODE.txt" 2>/dev/null)
ADDRMODE=$(cat "$BASE/ADDRESS_MODE.txt" 2>/dev/null)
MEDIA_STATE=$(cat "$RINGSTATE" 2>/dev/null)

{
    echo "YiLocal v1.0.0"
    echo "NETWORK_STATE=$STATE"
    echo "NETWORK_MODE=$MODE"
    echo "ADDRESS_MODE=$ADDRMODE"
    echo "IP=$IP"
    echo "MAC=$MAC"
    echo "SSID=$SSID"
    echo "RMM_LOCAL_MEDIA_ENABLE_RC=$MEDIA_ENABLE_RC"
    echo "MEDIA_RING=$MEDIA_STATE"

    if ps | grep '[y]ilocal_webd' >/dev/null 2>&1; then echo "WEBUI=RUNNING"; else echo "WEBUI=FAILED"; fi
    if ps | grep '[y]ilocal_rtspd' >/dev/null 2>&1; then echo "RTSP=RUNNING"; else echo "RTSP=FAILED"; fi
    if ps | grep 'onboarding_supp' | grep -v grep >/dev/null 2>&1; then echo "YI_PROMPT_SUPPRESSION=RUNNING"; else echo "YI_PROMPT_SUPPRESSION=FAILED"; fi
    if ps | grep '[t]server' >/dev/null 2>&1; then echo "STOCK_TSERVER=UNEXPECTEDLY_RUNNING"; else echo "STOCK_TSERVER=REMOVED"; fi

    if [ -n "$IP" ]; then
        echo "WEB=http://$IP/"
        echo "MAIN=rtsp://$IP:8554/main"
        echo "SUB=rtsp://$IP:8554/sub"
    fi
} > "$BOOTSTATUS"

[ -n "$IP" ] && echo "$IP" > "$BASE/CURRENT_IP.txt"
[ -n "$MAC" ] && echo "$MAC" > "$BASE/MAC.txt"

echo
echo "--- FINAL PROCESSES ---"
ps

echo
echo "--- FINAL NETWORK ---"
ifconfig wlan0 2>/dev/null
route -n 2>/dev/null
netstat -an 2>/dev/null

echo
echo "--- MEDIA ENABLE LOG ---"
cat "$MEDIA_LOG" 2>/dev/null

echo
echo "--- BOOT STATUS ---"
cat "$BOOTSTATUS" 2>/dev/null

echo "============================================================"
echo "===== YiLocal v1.0.0 active ================================"
echo "===== WebUI: http://CAMERA_IP/ ==============================="
echo "===== RTSP : rtsp://CAMERA_IP:8554/main ====================="
echo "============================================================"

exit 0
