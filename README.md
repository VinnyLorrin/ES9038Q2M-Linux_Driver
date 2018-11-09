# ES9038 I2C Linux Driver Install Manual

## Setup compile environment

### Getting Kernel Source
Install "rpi-source" software.  
The "rpi-source" command automaticaly downloads the kernel source that matched running linux kernel version.

https://github.com/notro/rpi-source/wiki

to install:

    sudo wget https://raw.githubusercontent.com/notro/rpi-source/master/rpi-source -O /usr/bin/rpi-source && sudo chmod +x /usr/bin/rpi-source && /usr/bin/rpi-source -q --tag-update
    
### Getting dtc (Device Tree Compiler) package
Install dtc by package system.

    sudo apt-get install device-tree-compiler 

### Install bc command
If bc is not installed, must install bc.

    sudo apt-get install bc

### Getting GCC package
The GCC version must match the gcc that used to build the kernel, the required version can be checked by 

    sudo cat /proc/version	

Install GCC-4.9.x by package system.

    sudo apt-get install gcc-4.9 g++-4.9

Change default GCC

	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 10
	update-alternatives --config gcc

Check GCC version is 4.9.x.

    gcc --version

## Driver Install

### 1. Get the kernel source

Choose place of kernel source files & directory, e.g. '.' current folder.

    rpi-source -d .

if rpi-source gives kernel version mismatch, use 

	sudo apt-get install raspberrypi-kernel-headers


### 2. Build & Install driver & dtb (device tree blob)

    make
    sudo make modules_install
    make dtbs
    sudo make install_dtbo

### 3. Change /boot/config.txt

remove existing dtoverlay
Add following lines.

    dtoverlay=hifibunny3-q2m
    dtparam=i2c_arm=on

### 4. Reboot Raspberry Pi

### 5. Check driver is normaly installed

Use aplay -l command to check audio card is added.

    aplay -l

	
### 6. optional: enable bbr 

	sudo nano /etc/sysctl.conf
	
add following to the end of the file:

	net.core.default_qdisc=fq
	net.ipv4.tcp_congestion_control=bbr
	
In terminal, run:

	sudo sysctl -p
check:

	sysctl net.ipv4.tcp_congestion_control
	
## Note

This driver only works with the ES9038Q2M DAC connected through i2c/i2s.

Supports volume control and DoP, tested on Moode Audio. SoX resampling needs to be set to '32 bit/ *kHz' for proper DoP function.

DAC is setup in MASTER mode with NCO to support both 44.1KHz and 48KHz family with a single crystal. 

Crystal freq is assumed to be 49.152MHz, if other freq are used, ES9038Q2M_NCO_0~ES9038Q2M_NCO_4 values need to be adjusted. 