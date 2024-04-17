# Light Station - Weather station for ambient light and UV
I built this waather station to collect light information (to control blinds) and also drive 4 relays to turn garden lights on/off.

I used a lamp housing for the light sensors and the ESP:
![Case](/light02.jpg)

And the relays are in a separate box a few meters away under the terace:
![Case](/light03.jpg)

## Bill of materials
- SI1145 UV / IR and Visible light sensor: don't remember where I purchased it from, but it is not version: https://circuit.rocks/products/uv-index-ir-visible-si1145-digital-light-sensor . Adafruit sells a different package
- VEML6070: UV A / B and C sensor: https://www.adafruit.com/product/2899. 
- ESP8266 as a Wemos D1 Mini
- Home Sensor board PCB as a base: https://www.pcbway.com/project/shareproject/Home_Sensor_Board.html
- Wires
- 4 Channel relay board (5V version) with like this: https://www.aliexpress.com/item/32649659086.html
- Hi-Link 5V power supply like this: https://www.aliexpress.com/item/1005003052023770.html

## Wiring
Wemos D1 mini | SI114 and VEML6070 | Relay board
---|---|---
GND | GND | GND 
3V3 | VIN |
5V | | VCC
D1 | SCL
D2 | SDA
D5 | | IN1
D6 | | IN2
D7 | | IN3
D0 | | IN4

This is how the sensors and the ESP looked like in the lamp enclosure: 
![Box](/light01.jpg)

I ran a ethernet cable between the relay box and the lamp enclosure (purple wire). I used wires for 5V and GND, and 4 wires for the 4 outputs. The 5V power supply and the relay board in mounted in the junction box with the 4 sockets.

## Video
More details can be found in the full build video here:

[![Weatherstations](https://img.youtube.com/vi/O5KoNkXHxOc/0.jpg)](https://www.youtube.com/watch?v=O5KoNkXHxOc)
