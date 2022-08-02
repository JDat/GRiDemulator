GRid Compass and Compass II emulator.
Models:

* 1100
* 1101 (have ROM)
* 1109
* 1121
* 1128-VP
* 1129
* 1131
* 1139 (have ROM)

Based on XTulator project by mikechambers84.

https://github.com/mikechambers84/XTulator

Some ideas come from MAME emulator's GRiD implementation by usernameak.

And big thanks to usernameak personally for his research and consulting

http://deltacxx.insomnia247.nl/gridcompass/

* This is early stage.
* Added CMake system from https://github.com/user890104/XTulator
* For now ROM is running up to stage where it tries to boot from GPIB or bubble memory. This is normal in current stage.
* Lot of work to do.
* Don't complain about functionality in this early stage.


Sorry, Windows support and PC XT hardware removed from original code.

### Compiling from source under Linux
`sudo apt-get install libsdl2-dev libpcap-dev cmake`

`mkdir build && cd build && cmake .. && make`

### Running under Linux
Make shure you have boot ROMs in ROM directory

`./gridemu -machine 1101`

`./gridemu -machine 1139`

`./gridemu -machine 1101 -debug detail`

`./gridemu -machine 1101 -debug detail 2> test.log`


Original readme from XTulator author:

### XTulator - A portable, open source (GPLv2) 80186 PC emulator

### About

XTulator is an x86 PC emulator that is designed to run software that was written for Intel processors up to the 80186. It's able to run MS-DOS, FreeDOS, Windows 3.0, and many games of the era. It supports graphics up to the EGA/VGA standard, and emulates the Sound Blaster 2.0 as well as Adlib/OPL2 (using Nuked OPL). It also emulates peripherals like the standard Microsoft-compatible serial mouse and a serial modem, which can simulate phone line connections via TCP. An NE2000 Ethernet adapter is also emulated using pcap.

This is actually a rewrite of an emulator I wrote many years ago. It was poorly implemented, even though it worked fairly well. It had many hacks and a poor architecture, but most old 80186 software could still run under it. I've just never quite been happy with it, so I am writing this new emulator to be more sanely coded, and more accurate. I'm aiming to create a much more modular architecture this time around and avoid the design flaws which turned the old code into a mess.

### Re-write goals

- More sane architecture in general
- Minimize (preferably eliminate) use of shared globals
- Implement a sort of generic "virtual ISA card" interface
- Keep CPU and other modules as independent of each other as possible
- Improve accuracy of chipset and components emulation
- Maintain host platform-independence
- Implement proper hard drive and floppy disk controller interfaces, rather than "cheating" with HLE like before

### Current status

WARNING: This software is still currently in the early stages, and not really ready for general use. If you enjoy testing experimental new software, this is for you!

Checkmarks below mean that feature is implemented enough to boot and run things with the "generic_xt" machine definition. See comments below for details.

- [x] CPU - Complete
- [x] Implement support for multiple machine defnitions.
- [x] Intel 8253 timer (Re-implemented, but needs some work to be fully accurate)
- [x] Intel 8259 interrupt controller (Working, also needs some more attention. This may be the cause of some of the BIOS issues.)
- [x] Re-implement proper system timing
- [x] Re-implement proper video rendering
- [x] Keyboard input
- [x] RTC
