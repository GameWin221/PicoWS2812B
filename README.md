# PicoWS2812B

![glow](/photos/glow.jpg)

16x16 WS2812B matrix display controlled via Wi-Fi TCP socket with a Raspberry Pi Pico W. This project is my passion project but it also puts emphasis on performance and code readabilty in order to serve as a simple example for people learning. 

# Important Features
- Both 8 bits per-channel mode and 4 bits per-channel mode for lowering network usage and decreasing delay.
- TCP used instead of HTTP for additional performance.
- Seamless handling of client<->server connect / disconnect.
- Readable code that can easily be used for learning purposes.

# Controller
Any controller that uses **TCP sockets** and conforms to the **Packet Format** and the **Data Protocol** sections below will be compatible with the display. 

Take a look at the [working example of a compatible controller](https://github.com/GameWin221/pico-ws2812b-controller)

## Data Protocol
The controller must first connect to the Raspberry Pi Pico **via a TCP socket on port 4242**.

1. Controller sends a valid packet (look at **Packet Format**) to the Pico W
2. Controller waits for a response from the Pico W
3. If the response reads exactly `"ACK"`, then the packet was sent succesfully

After reading the correct response, the next packet can be sent immediately. The whole process typically takes about only 10ms depending on the network hardware.

## Packet Format
```cpp
struct Packet {
    uint8_t data_type;
    uint8_t data[];
};
```

The `data_type` field can be either 0x01 (DATA_TYPE_FULL) or 0x02 (DATA_TYPE_HALF) and the display will handle both types automatically. Any other value will be considered invalid.

DATA_TYPE_FULL packet expects to carry **exactly** 16\*16\*3 (768) bytes of 8-bit per channel RGB data.

DATA_TYPE_HALF packet expects to carry **exactly** 16\*16\*3/2 (384) bytes of tightly packed 4-bit per channel RGB data. This mode saves data in order to lower the network usage and decrease the delay.

RGB data is expected to be in the [row-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order) order with (x:0, y:0) being the lower-left corner:
```
...       , ...       , ...
(x:0; y:2), (x:1; y:2), ...
(x:0; y:1), (x:1, y:1), ...
(x:0; y:0), (x:1, y:0), ...
```
## Working Example
The working example of a compatible controller can be found [here in this repository](https://github.com/GameWin221/pico-ws2812b-controller)

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