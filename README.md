# YiLocalCam

**Local RTSP + web control for the YI 1080p Home Camera — no cloud account,
no soldering, no reflashing.**

Drop this package on a microSD card and the camera boots into a local-only
mode: the vendor cloud / P2P / auto-update processes are killed and kept
dead, and you get clean RTSP streams plus a small web UI — all from the SD
card, entirely in RAM. Pull the card out and the camera is stock again.

No YI app login. No firmware mod written to flash. Nothing to undo.

---

## Which camera does this work on?

**Tested on exactly this model — everything else is untested:**

| Field | Value |
|---|---|
| Product | **YI 1080p Home Camera** (YI Technology, Allwinner series) |
| Model code | **QG2101A** |
| SoC | Allwinner **H313** (`sun8iw19p1`), ARMv7 |
| Kernel | Linux 4.9.118 (vendor) |
| Sensor | GC2053 (MIPI) |
| Wi-Fi | AltoBeam ATBM-603x (USB, **2.4 GHz only**) |
| U-Boot | 2018.05 (built 2021-11-29) |
| Vendor firmware | `12.0.35.8_202411121417` |

**Do not assume this works on other YI cameras.** YI has shipped cameras on
HiSilicon, SigmaStar, and Ingenic SoCs (Yi Dome, YI G series, YI Action,
older YI Home models) — those have a different SD-card boot behavior, a
different media pipeline, and different state layouts, and are **not**
covered here. Check the model label on the bottom of your camera; if it is
not a QG2101A-class H313 board, treat this as experimental.

If you have a different Allwinner-based YI camera and want a full firmware
replacement instead of an SD-card overlay, look at
[roleoroleo/yi-hack-Allwinner-v2](https://github.com/roleoroleo/yi-hack-Allwinner-v2)
(same platform family; it writes to flash). YiLocalCam deliberately
**never touches flash**.

## What you get

| Service | Address |
|---|---|
| Web UI (status, Wi-Fi setup, logs, reboot) | `http://<camera-ip>/` |
| RTSP main — 1920×1080 H.264 + AAC mic | `rtsp://<camera-ip>:8554/main` |
| RTSP sub — 640×360 H.264 (detection) | `rtsp://<camera-ip>:8554/sub` |

Vendor processes killed every 2 s while the card is in:
`watch_process`, `upgrade_firmware`, `cloud`, `cloudAPI`, `p2p_tnp`, `oss`,
`rtmp`, `mp4record`, `log_server`, `telnetd`, `tserver`.

That means: **no vendor cloud upload, no vendor P2P relay access, no
auto-updates** while running. The camera only talks to your Wi-Fi access
point. Stock onboarding voice prompts and blinking LEDs are suppressed too.

## Quick start

1. Format a microSD card as **FAT32** (≤ 32 GB is the safe choice).
2. Copy **the contents of the [`sd/`](sd/)** folder to the root of the card
   (you should end up with `debug.sh`, `SET_NETWORK.ps1`, and `yilocal/` at
   the card root).
3. Tell it your Wi-Fi — pick one:
   - **Windows:** double-click / run `SET_NETWORK.ps1` from the card root
     (PowerShell 5+, asks SSID + password, optional static IP).
   - **Any OS:** copy `yilocal/config/wifi.conf.example` to
     `yilocal/config/wifi.conf` and fill in SSID + password. The Wi-Fi must
     be **2.4 GHz WPA/WPA2**.
   - **Skip it entirely:** with no `wifi.conf`, the camera opens its own
     access point **`YiLocal-XXXXXX`** (password `yilocalsetup`). Connect to
     it and browse `http://192.168.10.1/`, then set Wi-Fi in the UI.
4. Optionally set a fixed address in `yilocal/config/network.conf`
   (default is DHCP).
5. Insert the card, power the camera on, wait ~60 seconds.
6. Open the web UI. With DHCP, find the address in your router's client
   table, or read `yilocal/CURRENT_IP.txt` (written back to the card after
   boot).

Verify the streams from a PC with ffmpeg/ffprobe:

```
ffprobe -hide_banner -rtsp_transport tcp -show_streams rtsp://<camera-ip>:8554/sub
ffprobe -hide_banner -rtsp_transport tcp -show_streams rtsp://<camera-ip>:8554/main
```

For [Frigigate](https://frigate.video)-style NVR use, see
`sd/FRIGATE.example.yml`.

## Web API

No authentication — **trust your LAN only**. Do not expose these ports
beyond your network.

| Endpoint | Method | Purpose |
|---|---|---|
| `/` | GET | web UI |
| `/api/status` | GET | live status JSON (Wi-Fi, IP, firmware, media state) |
| `/api/log` | GET | full boot log |
| `/api/rtsp-log` | GET | RTSP server log |
| `/api/wifi/scan` | POST | scan nearby networks |
| `/api/wifi/connect` | POST | join a network (`ssid` + `psk` body) |
| `/api/wifi/result` | GET | progress of the last connect attempt |
| `/api/restart-rtsp` | POST | restart the RTSP server |
| `/api/reboot` | POST | reboot the camera |

## How the trick works

Short version: this YI firmware line executes `/tmp/sd/debug.sh` from the
SD card as **root** during boot, with no signature check. YiLocalCam rides
on that: it waits for the stock media bridge (`dispatch` + `rmm`), kills
the vendor network stack, then flips two **RAM-only** fields in rmm's
`/tmp/mmap.info` state (`start_with_reset` → 0, `wifi_connected` → 1) so
the vendor encoder path starts feeding the shared frame ring, which the
included `yilocal_rtspd` republishes as RTSP. No MTD writes, no writes to
`/backup`, no persistent changes at all.
Details: [`docs/HOW-IT-WORKS.md`](docs/HOW-IT-WORKS.md).

## Uninstall

Pull the SD card (or create an empty file named `YILOCAL_DISABLE` at the
card root) and reboot. The camera boots fully stock — there is nothing
persistent to remove.

## Building from source

Prebuilt ARMv7 static binaries ship in `sd/yilocal/bin/`. Sources for the
web daemon, RTSP server, media helpers, and the assembly syscall helpers
are in [`src/`](src/):

```
make            # expects an armv7 cross toolchain (see src/Makefile)
```

`zig cc` works out of the box; classic `arm-linux-gnueabi-gcc -static`
also works.

## Troubleshooting

- **No streams / black ring:** check `yilocal/BOOT_STATUS.txt` on the card —
  `MEDIA_RING=ACTIVE` is required. `ALL_ZERO` means the vendor encoder never
  started feeding; check `yilocal/media_enable.log` and `yilocal/yilocal.log`.
- **Wrong address:** `yilocal/CURRENT_IP.txt` and `yilocal/NETWORK_DIAG.txt`
  on the card show what actually happened.
- **It associated with the wrong network:** the vendor image's saved Wi-Fi
  config (`/backup/wpa_supplicant.conf`) is left alone but *not* used;
  only `wifi.conf` (or the AP flow / web UI) is honored.
- **Camera reachable but UI broken:** `POST /api/reboot`, or reseat the card.

## Safety notes

- All writes go to RAM (`/tmp`, `/dev/shm`) or the SD card. The SPI flash
  and the vendor `rootfs`/`backup` partitions are never written.
- The camera stays on your Wi-Fi and speaks to nobody else (vendor relay
  processes are dead). But remember: **RTSP and the web UI have no auth**.
- This is an unofficial community hack. You use it at your own risk; a
  misbehaving SD card can only ever put the camera back to stock on the
  next boot, but always keep a backup of your card.

## License

MIT — see [LICENSE](LICENSE). The vendor binaries it talks to (`dispatch`,
`rmm`, `wpa_supplicant`, …) remain property of their owners and are **not**
included in this repository.
