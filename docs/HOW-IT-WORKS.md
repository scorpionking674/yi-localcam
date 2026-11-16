# How YiLocalCam works

Everything here is RAM-only. The SPI flash, the vendor squashfs root, and
the `/backup` jffs2 partition are never written.

## 1. The entry point: vendor SD auto-run

This line of YI firmware (Allwinner H313 / `sun8iw19p1`, vendor build
`12.0.35.8_...`) executes **`/tmp/sd/debug.sh` from the SD card as root**
early in the boot sequence, unconditionally and without signature checks.
That is the entire "hack": `debug.sh` is our launcher, PID 712-ish, UID 0,
right next to the stock services.

## 2. Boot flow (`sd/debug.sh`)

1. Wait up to 20 s for the stock media bridge: `dispatch` + `rmm` running
   and the `/dev/shm/fshare_frame_buf` shared ring present.
2. `killall` the vendor network/cloud stack (`watch_process`,
   `upgrade_firmware`, `cloud`, `cloudAPI`, `p2p_tnp`, `oss`, `rtmp`,
   `mp4record`, `log_server`, `telnetd`, `tserver`) and start a 2 s
   kill-guard loop so `watch_process` can't resurrect it.
3. Suppress stock onboarding: the onboarding AAC prompts are resolved
   through a `dispatch`-created `/tmp/audio` symlink — it is redirected to
   an empty RAM directory, and status LEDs are reset via sysfs + the
   `yilocal_status_led_connected` helper (vendor IPC message).
4. Start the web daemon (`yilocal_webd`, TCP 80).
5. Network bring-up (`bootstrap_network.sh`): join `config/wifi.conf`
   (or leave an existing association), then DHCP or static per
   `config/network.conf`. If the configured network fails, the camera
   falls back to its own **AP** (`YiLocal-XXXXXX`, password `yilocalsetup`,
   web + its own tiny DHCP server on `192.168.10.1`).
6. **The media trick** (`yilocal_rmm_local_enable`): `rmm` maps
   `/tmp/mmap.info` (tmpfs) and its startup state machine reads two
   fields:

   | Offset | Field | Stock meaning | YiLocal action |
   |---|---|---|---|
   | `+0x6dc` | `start_with_reset` | 1 = stay in factory-reset/onboarding loop, never reach the encoder path | write **0** |
   | `+0x678` | `wifi_connected` | set by vendor flow when the vendor stack associates | write **1** |

   Because only the vendor's own cloud flow normally sets these, a camera
   that was factory-reset / never enrolled keeps `start_with_reset = 1`
   and its encoder stays idle. Flipping these two RAM fields makes the
   stock `rmm` take its normal "wifi connected goto venc and aenc" branch,
   so the hardware encoder starts writing into the `fshare` ring.
7. The vendor `dispatch` service is nudged to retain the ring: a POSIX
   message queue poke (`compat/yi_ipc_buffer_trigger`: `mq_open("ipc_dispatch")`
   + `mq_timedsend("IPC trigger sent")`, raw ARM syscalls, no libc).
8. `yilocal_ring_probe` waits up to 70 s for **actual non-zero encoded
   bytes** in the ring (rmm can wait ~40 s internally) and records
   `ACTIVE` / `ALL_ZERO` in `yilocal/MEDIA_RING_STATE.txt`.
9. `yilocal_rtspd` (TCP 8554) `mmap`s `/dev/shm/fshare_frame_buf`, parses
   the H.264/AAC framing the vendor encoder writes, caches the IDR, and
   serves `/main` and `/sub` as RFC 2326 RTSP (TCP interleaved only).
10. Manager loops: `network_manager.sh` (web-initiated Wi-Fi changes with
    rollback), `status_loop.sh` (writes `/tmp/yilocal_status.json` every
    2 s), `system_manager.sh` (handles reboot / RTSP-restart requests
    dropped by the web daemon).

A final `BOOT_STATUS.txt` is written to the card with the whole outcome.

## 3. Why nothing persists

- All state writes: `/tmp/mmap.info` (tmpfs), `/tmp/*`, `/dev/shm/*`.
- All logs/status files: written back onto the SD card (disappears with it).
- Deliberately **not** used: vendor dispatch command 104 (its propagation
  path can reach persistent configuration writes).

Remove the card → next boot is 100 % stock.

## 4. File map

```
sd/
  debug.sh                     vendor auto-run entry point (SD root)
  SET_NETWORK.ps1              Windows Wi-Fi/IP setup helper
  FRIGATE.example.yml          starter NVR config
  yilocal/
    bin/                       ARMv7 static daemons (see ../src)
    compat/yi_ipc_buffer_trigger   mqueue poke for ring retention
    scripts/                   bootstrap, network, onboarding, status, system
    www/                       web UI (served by yilocal_webd)
    config/wifi.conf           your credentials (created by SET_NETWORK.ps1 / UI)
    config/network.conf        DHCP (default) or STATIC
```
