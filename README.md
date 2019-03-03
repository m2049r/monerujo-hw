# monerujo-hw
Monerujo Firmware for [The Kastelo Hardware Wallet](https://github.com/monero-project/kastelo/)

## Prerequisites
- This is a [GNU MCU Eclipse](https://gnu-mcu-eclipse.github.io/) project, however the
  command line GNU Make(1) is supported as well.
- sudo apt-get install gcc-arm-none-eabi
- [libopencm3](https://github.com/libopencm3/libopencm3/), either in /opt (don't forget to
  ```make``` there as well) or a local submodule
- dfu-util to program the device over USB - see the Kastelo project for more details

## udev Rules
```
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="660", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="f055", ATTR{idProduct}=="c0da", MODE="660", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
```

## Programming
```dfu-util -a 0 -s 0x08000000:leave -D monerujo-(con|nbl).bin```

## Running
If the screen displays correctly but button switches seem to do nothing, it's likely that the device has been programmed with blocking I/O. To test this, connect to the device with a serial terminal. For Linux, use ```screen /dev/ttyACM0``` (maybe ACM1 - check the /dev directory). For Windows I suggest TeraTerm. Terminal output will show the generated keys as well as other debug info.
