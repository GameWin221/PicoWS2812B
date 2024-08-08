# PicoWS2812B

![glow](/photos/glow.jpg)

16x16 WS2812B matrix display controlled via Wi-Fi TCP socket with a Raspberry Pi Pico W. This project is my passion project but it also puts emphasis on performance and code readabilty in order to serve as a simple example for people learning. 

# Important Features
- Both 8 bits per-channel mode and 4 bits per-channel mode for lowering network usage and decreasing delay.
- TCP used instead of HTTP for additional performance.
- Seamless handling of client<->server connect / disconnect.
- Readable code that can easily be used for learning purposes.
- Images, Image sequences and GIFs can be stored in flash memory and played back offline with no interruptions.

# Controller
Any controller that uses **TCP sockets** and conforms to the **Packet Format** and the **Data Protocol** sections below will be compatible with the display. 

Take a look at the [working example of a compatible controller](https://github.com/GameWin221/pico-ws2812b-controller)

## Data Protocol
The display receives packets through a TCP socket open on port 4242 by deafault. (It can be changed)

1. Controller sends a valid packet (look at **Packet Format**) to the Pico W
2. Controller waits for a response from the Pico W
3. If the response reads exactly `"ACK"`, then the whole packet was sent and received succesfully.

After reading the correct response, the next packet can be sent immediately.

## Packet Format
Take a look at the [packet.hpp](src/packet.hpp) file to see all packet types. Mind the [default C/C++ struct alignment](https://en.wikipedia.org/wiki/Data_structure_alignment#Typical_alignment_of_C_structs_on_x86). 

The first byte (`uint8_t`) of a received packet is always the `data_type` field. Each packet type has its own unique data type.

RGB data is expected to be in the [row-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order) order with (x:0, y:0) being the lower-left corner:
```
...       , ...       , ...
(x:0; y:2), (x:1; y:2), ...
(x:0; y:1), (x:1, y:1), ...
(x:0; y:0), (x:1, y:0), ...
```

8bpp (Full) RGB data layout:
```
data[0] , data[1] , data[2] , data[...]
RRRRRRRR, GGGGGGGG, BBBBBBBB, ...
```

4bpp (Half) RGB data layout:
```
data[0] , data[1] , data[2] , data[...]
RRRRGGGG, BBBBRRRR, GGGGBBBB, ...
```


## Working Example
The working example of a compatible controller can be found [here in this repository](https://github.com/GameWin221/pico-ws2812b-controller)

# Build
## Prerequisites
- Native GNU C++ compiler [(for example MinGW)](https://winlibs.com/) that is added to PATH
- [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and the `PICO_TOOLCHAIN_PATH` environmental variable.
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) and the `PICO_SDK_PATH` environmental variable.

## Command
Once the prerequisites are satisfied, run the following command:
```
cmake -G "MinGW Makefiles" .. 
make (ming32-make on Windows)
```

And copy the built .uf2 file to your Pico's Mass Storage

# Power Consumption
**Please double-check if your power supply can safely provide enough current at 5V. Note that not every WS2812B draws the same amount of current.**

Current draw was measured only at 0%, 12.5% and 25% brightness with the whole display set to RGB(255, 255, 255). Higher values were calculated by extrapolatating the measurements.

![current](/photos/current.png)

# Parts Required
- [16x16 WS2812B RGB LED Matrix](https://botland.store/led-strings-chains-matrices/6183-elastic-matrix-16x16-256-led-rgb-ws2812b-5904422374945.html)
- [Raspberry Pi Pico W](https://botland.store/raspberry-pi-pico-modules-and-kits/21574-raspberry-pi-pico-w-rp2040-arm-cortex-m0-cyw43439-wifi-5056561803173.html)
- [Short Breadboard](https://botland.store/breadoards/19942-breadboard-justpi-400-holes-5904422328627.html)
- 3 Male-Male Jumper Wires (or any other way to connect WS2812B to the Pico)
- [DC 5,5x2,1mm socket with quick connection](https://botland.store/quick-couplings/1804-dc-55x21mm-socket-with-quick-connection-5904422373238.html)
- A piece of white paper or thin plastic that works well as a light diffuser
- 3D printed frame and grid (both .stl files are in the `3dprints` directory)

## Assembly
- Put the WS2812B in between the printed `Frame.stl` and the printed `Grid.stl` so that it fits nicely
- Glue `Frame.stl` and `Grid.stl` together
- Glue the light diffuser to `Grid.stl` so that it fully covers the LEDs
- Stick the breadboard to the matrix's backside - inside `Frame.stl`
- Insert the DC socket into the dedicated hole and **correctly connect** the power cables of the LED matrix.
- Put the Raspberry Pi Pico W into the breadboard and **correctly connect** the jumper wires.

# Disclaimer
**I am not responsible for any physical damage caused by running this code. Use adequate brightness levels and double-check if your power supply can safely provide enough current in order to avoid any unwanted damage. Some WS2812B LED matrices may require more current than others. The TCP server accepts every incoming connection and every packet with no verification so it may potentially pose a threat to your local network's overall security.**