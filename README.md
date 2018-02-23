# monerujo-hw
Monerujo Firmware for [The Sekura Monero Hardware Wallet](https://github.com/monero-project/sekura)

## Prerequisites
- This is a [GNU MCU Eclipse](https://gnu-mcu-eclipse.github.io/) project
- sudo apt-get install gcc-arm-none-eabi
- [libopencm3](https://github.com/libopencm3/libopencm3) in /opt (don't forget to ```make``` there as well)
- dfu-util to program the device over USB - see the sekura project for more details

## udev Rules
```
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="660", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="c0da", MODE="660", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
```

## Programming
```dfu-util -a 0 -s 0x08000000:leave -D monerujo-hw.bin```

## Running
As the USB I/O is blocking you **MUST** connect to the device with a serial terminal. For Linux, use ```screen /dev/ttyACM0``` (maybe ACM1 - check the /dev directory). For Windows I suggest TeraTerm. Terminal output will show the generated keys as well as other debug info.
