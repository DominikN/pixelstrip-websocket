# pixelstrip-websocket

**LED pixel strip controlled over internet available websocket API based on ESP32**

Send LED pixel strip state as a table (number of pixels * 3 RGB channels) over the internet available websocket. Thanks to that you can integrate LED pixel strips with other systems in very easy way. Sample application could be:

* make light FFT efect to a music played on your laptop
* smart TV backlight
* connecting ledstrip with a movement sensor in your lobby
* make a cool light decoration for your christmas tree that interacts with environment (eg. if someones enters room)

Running the project is straightforward. Just follow these steps:

**A. HARDWARE:**

Interface between ESP32 and LED pixel strip is as follows:
```
ESP32 <-> LED pixel strip

P12   <-> Din
GND   <-> GND
```
Remember to provide appropriate power supply both for your ESP32 and LED pixel strip.

**B. SOFTWARE:**

To run the project, open Arduino IDE and follow these steps:

**1. Install Husarnet package for ESP32:**

- open `File -> Preferences`
- in a field **Additional Board Manager URLs** add this link: `https://github.com/husarnet/arduino-esp32/releases/download/1.0.4-1/package_esp32_index.json`
- open `Tools -> Board: ... -> Boards Manager ...`
- search for `esp32-husarnet by Husarion`
- click Install button

Please note that we include here a modified fork (mainly IPv6 support related changes) of official Arduino core for ESP32 - https://github.com/husarnet/arduino-esp32 . If you had installed the original version before, it is recommended to remove all others Arduino cores for ESP32 that you had in your system.

**2. Select ESP32 dev board:**

- open `Tools -> Board`
- select **_ESP32 Dev Module_** under "ESP32 Arduino (Husarnet)" section


**3. Install arduinoWebSockets library (Husarnet fork):**

- download https://github.com/husarnet/arduinoWebSockets as a ZIP file (this is Husarnet compatible fork of arduinoWebSockets by Links2004 (Markus) )
- open `Sketch -> Include Library -> Add .ZIP Library ... `
- choose `arduinoWebSockets-master.zip` file that you just downloaded and click open button

Modify this library a little bit:
- open **WebSockets.h** file
- modify lines:
```
#define WEBSOCKETS_MAX_DATA_SIZE  (30*1024) // (15*1024) CHANGE!
...
#define WEBSOCKETS_TCP_TIMEOUT    (20000) // (2000) CHANGE!
```

**4. Install NeoPixelBus library:**

- open `Tools -> Manage Libraries`
- search for `NeoPixelBus` by Makuna, version 2.3.5
- click "Install" button

**5. Install PID library:**

- open `Tools -> Manage Libraries`
- search for `PID` by Brett Beauregard
- click "Install" button

**6. Program ESP32 board:**

- Open **ESP32-MPU9250-web-view.ino** project
- modify line 40 with your Husarnet `join code` (get on https://app.husarnet.com)
- modify lines 21 - 29 to add your Wi-Fi network credentials
- upload project to your ESP32 board.

**C. PYTHON SCRIPT:**

**1. Add your laptop/server/RaspberryPi to the same VPN network as ESP32 :**

type in the Linux terminal:

- `$ curl https://install.husarnet.com/install.sh | sudo bash` to install Husarnet.
- `$ systemctl restart husarnet` to restart Husarnet.
- `$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop` to connect your laptop to the Husarnet network. Rreplace XXX...X with your own `join code`. 

To find your join code:
* register at https://app.husarnet.com
* create a new network or choose existing one
* in the chosen network click **Add element** button and go to "join code" tab

At this stage your ESP32 and your laptop are in the same VLAN network.

**2. Run sample Python 3 script on you Linux computer to display a sample rainbow theme on LED pixel strip**

- make sure Python 3 is installed on your machine
- install `asyncio` Python 3 library by typing the Linux terminal:
```py -m pip install asyncio```
- install `websockets` Python 3 library by typing the Linux terminal:
```py -m pip install websockets```
- install `numpy` Python 3 library by typing the Linux terminal:
```py -m pip install numpy```
- install `matplotlib` Python 3 library by typing the Linux terminal:
```py -m pip install matplotlib```
- and finally run the script - in the project folder type in the terminal:
```py display```

After a while you should see a rainbow theme displayed on the LED pixel strip.

TROUBLESHOOTING

If ESP32 doesn't work and you see similar error:

I (1559) wifi: wifi firmware version: 224c254
I (1559) wifi: config NVS flash: enabled
I (1563) wifi: config nano formating: disabled
W (1570) wifi: wifi_nvs_set fail, index=3 ret=4357

It may mean that you stored to much data in NVS memory. To deal with that install esptool - https://github.com/espressif/esptool .

py -m pip install esptool
cd C:\Users\usr1\AppData\Local\Programs\Python\Python38-32\Lib\site-packages
py esptool.py --chip esp32 --port com3 erase_flash

To increase a NVS partition, edit C:\Users\domin\OneDrive\Dokumenty\ArduinoData\packages\esp32-husarnet\hardware\esp32\1.0.5\tools\partitions\default.csv

file. For more information visit that site: https://github.com/espressif/arduino-esp32/issues/703 .

Change  esp32-husarnet\hardware\esp32\1.0.5\tools\partitions\default.csv partiton table to:

```
# Name,   Type, SubType, Offset,  Size, Flags
spiffs,      data, spiffs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x140000,
app1,     app,  ota_1,   0x150000,0x140000,
eeprom,   data, 0x99,    0x290000,0x1000,
nvs,   data, nvs,  0x291000,0x16F000,
```

