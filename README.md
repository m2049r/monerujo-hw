# monerujo-hw
Monerujo Firmware for [The Sekura Monero Hardware Wallet](https://github.com/monero-project/sekura)

## Prerequisites
- This is a [GNU MCU Eclipse](https://gnu-mcu-eclipse.github.io/) project
- [libopencm3](https://github.com/libopencm3/libopencm3) in /opt
- dfu-util to program the device over USB - see the sekura project for more details

## udev Rules
```
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="660", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="f055", ATTR{idProduct}=="c0da", MODE="660", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
```
