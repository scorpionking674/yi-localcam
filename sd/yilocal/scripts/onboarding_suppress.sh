#!/bin/sh
# YiLocal v1.0.0 - suppress Yi factory-reset onboarding state in RAM/runtime.
# Does NOT modify /home, /backup, or SPI NOR.
BASE=/tmp/sd/yilocal
LOG="$BASE/onboarding.log"
SILENT=/tmp/yilocal_silent_audio
LED_HELPER="$BASE/bin/yilocal_status_led_connected"
mkdir -p "$SILENT"
: > "$LOG"
log() { echo "$*" >> "$LOG"; }
log "===== YiLocal onboarding suppression $(date) ====="

suppress_audio_link() {
    # dispatch creates /tmp/audio -> /home/app/audio_file/<language> and later
    # resolves reset/wait/connect prompt AAC files through it. Redirect only
    # that volatile link to an empty RAM directory. Microphone capture is untouched.
    if [ -L /tmp/audio ] || [ -e /tmp/audio ]; then rm -rf /tmp/audio 2>/dev/null; fi
    ln -s "$SILENT" /tmp/audio 2>/dev/null
    if [ -L /tmp/audio ]; then log "Yi prompt path redirected to $SILENT"; else log "WARNING: could not redirect /tmp/audio"; fi
}

set_connected_led_state() {
    [ -x "$LED_HELPER" ] || return 0
    [ "$(cat "$BASE/NETWORK_STATE.txt" 2>/dev/null)" = "CONNECTED" ] || return 0
    "$LED_HELPER" >> "$LOG" 2>&1
    RC=$?
    [ "$RC" -eq 0 ] && log "Requested vendor status state: yellow OFF, blue ON" || log "WARNING: dispatch LED helper exit=$RC"
}

suppress_safe_sysfs_leds() {
    [ -d /sys/class/leds ] || return 0
    for L in /sys/class/leds/*; do
        [ -d "$L" ] || continue
        N=${L##*/}
        echo "$N" | grep -Eiq 'yellow|orange|status|alarm' || continue
        echo "$N" | grep -Eiq 'ir|white|illumin' && continue
        [ -w "$L/trigger" ] && echo none > "$L/trigger" 2>/dev/null
        [ -w "$L/brightness" ] && echo 0 > "$L/brightness" 2>/dev/null
        log "Best-effort sysfs onboarding LED off: $N"
    done
}

suppress_audio_link
suppress_safe_sysfs_leds

# dispatch can re-create the prompt symlink or reassert factory-reset LEDs.
# Keep the YiLocal connected state asserted after our Wi-Fi is actually up.
COUNT=0
while true; do
    TARGET=$(readlink /tmp/audio 2>/dev/null)
    [ "$TARGET" = "$SILENT" ] || suppress_audio_link
    suppress_safe_sysfs_leds
    COUNT=$((COUNT + 1))
    if [ "$COUNT" -ge 3 ]; then
        COUNT=0
        set_connected_led_state
    fi
    sleep 2
done
