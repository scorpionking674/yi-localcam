# YiLocalCam - ARMv7 static builds for the YI 1080p Home Camera (H313)
#
# Works with any of:
#   zig cc            (default here; no separate toolchain download)
#   arm-linux-gnueabi-gcc / arm-linux-gnueabihf-gcc (with -static)
#
# Override the compiler:  make CC=arm-linux-gnueabi-gcc
# Output is installed into ../sd/yilocal/bin (and compat) so the SD tree
# is ready to copy straight onto a card.

CC      ?= zig cc -target arm-linux-gnueabi-musl
CFLAGS  ?= -static -Os -s
ASFLAGS ?= -static

OUT     := ../sd/yilocal/bin
COMPAT  := ../sd/yilocal/compat
BINS    := $(OUT)/yilocal_webd $(OUT)/yilocal_rtspd $(OUT)/yilocal_dhcpd \
           $(OUT)/yilocal_ring_probe $(OUT)/yilocal_rmm_local_enable \
           $(OUT)/yilocal_status_led_connected $(COMPAT)/yi_ipc_buffer_trigger

all: $(BINS)

$(OUT)/yilocal_%: yilocal_%.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/yilocal_status_led_connected: yilocal_status_led_connected.S | $(OUT)
	$(CC) $(ASFLAGS) -o $@ $<

$(COMPAT)/yi_ipc_buffer_trigger: yi_ipc_buffer_trigger.S | $(COMPAT)
	$(CC) $(ASFLAGS) -o $@ $<

$(OUT) $(COMPAT):
	mkdir -p $@

clean:
	rm -f $(BINS)

.PHONY: all clean
