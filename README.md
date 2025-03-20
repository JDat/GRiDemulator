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

* This is early stage. Lot of things not working.
* So far I am working only on 1101 emulator because need to finish peripheral hardware emulation.
* For now ROM is running up to stage where it tries to boot from GPIB or bubble memory. This is normal in current stage.
* Lot of work to do.
* Don't complain about functionality in this early stage.
* Bubble booting sometimes working

Sorry, Windows support and PC XT hardware removed from original code.

### Compiling from source under Linux
`sudo apt-get install libsdl2-dev libpcap-dev cmake`

`make`

### Running under Linux
Make shure you have boot ROMs in ROM directory

`./gridemu -debug detail`

`./gridemu -debug detail 2> test.log`
