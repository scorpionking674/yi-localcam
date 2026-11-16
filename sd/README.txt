YiLocal v1.0.0
==============

1. Copy the contents of this folder to the root of a FAT32 SD card.
2. Set your Wi-Fi: run SET_NETWORK.ps1 from the card, or edit
   yilocal/config/wifi.conf (see wifi.conf.example) and network.conf.
3. Put the card in a YI 1080p Home Camera (H313 / QG2101A), power on.
4. Wait about 60 seconds, then open http://<camera-ip>/

No Wi-Fi configured? The camera starts its own access point
"YiLocal-XXXXXX" (password yilocalsetup) at http://192.168.10.1/

Full docs: the README.md in the source repository.
Remove the card (or create an empty YILOCAL_DISABLE file here)
to return the camera to stock behavior.
