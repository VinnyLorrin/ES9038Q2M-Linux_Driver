KERNEL_SRC = /lib/modules/$(shell uname -r)/source
BUILD_DIR := $(shell pwd)
DTC_DIR = /lib/modules/$(shell uname -r)/build/scripts/dtc/
VERBOSE = 0

OBJS    = hifibunny3-codec.o hifibunny3-q2m.o

obj-m := $(OBJS)

all:
	make -C $(KERNEL_SRC) SUBDIRS=$(BUILD_DIR) KBUILD_VERBOSE=$(VERBOSE) modules

clean:
	make -C $(KERNEL_SRC) SUBDIRS=$(BUILD_DIR) clean
	rm -f hifibunny3-q2m-overlay.dtb
	rm -f hifibunny3-q2m.dtbo

dtbs:
	$(DTC_DIR)/dtc -@ -I dts -O dtb -o hifibunny3-q2m-overlay.dtb hifibunny3-q2m-overlay.dts
	$(DTC_DIR)/dtc -@ -H epapr -I dts -O dtb -o hifibunny3-q2m.dtbo hifibunny3-q2m-overlay.dts

modules_install:
	cp hifibunny3-codec.ko /lib/modules/$(shell uname -r)/kernel/sound/soc/codecs/
	cp hifibunny3-q2m.ko /lib/modules/$(shell uname -r)/kernel/sound/soc/bcm/
	depmod -a

modules_remove:
	rm /lib/modules/$(shell uname -r)/kernel/sound/soc/codecs/hifibunny3-codec.ko
	rm /lib/modules/$(shell uname -r)/kernel/sound/soc/bcm/hifibunny3-q2m.ko
	depmod -a

install:
	modprobe hifibunny3-codec
	modprobe hifibunny3-q2m

remove:
	modprobe -r hifibunny3-q2m
	modprobe -r hifibunny3-codec

# Kernel 4.1.y
install_dtb:
	cp hifibunny3-q2m-overlay.dtb /boot/overlays/

# Kernel 4.4.y
install_dtbo:
	cp hifibunny3-q2m.dtbo /boot/overlays/

remove_dtb:
	rm -f /boot/overlays/hifibunny3-q2m-overlay.dtb
	rm -f /boot/overlays/hifibunny3-q2m.dtbo
