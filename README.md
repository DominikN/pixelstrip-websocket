# pixelstrip-websocket

**LED pixel strip controlled over internet available websocket API based on ESP32**

Send LED pixel strip state as a table (number of pixels * 3 RGB channels) over the internet available websocket. Thanks to that you can integrate LED pixel strips with other systems in very easy way. Sample application could be:

a) make light FFT efect to a music played on your laptop
b) smart TV backlight
c) connecting ledstrip with a movement sensor in your lobby
d) make a cool light decoration for your christmas tree that interacts with environment (eg. if someones enters room)

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
- in a field **Additional Board Manager URLs** add this link: `https://files.husarion.com/arduino/package_esp32_index.json`
- open `Tools -> Board: ... -> Boards Manager ...`
- search for `esp32-husarnet by Husarion`
- click Install button

**2. Select ESP32 dev board:**

- open `Tools -> Board`
- select **_ESP32 Dev Module_** under "ESP32 Arduino (Husarnet)" section


**3. Install arduinoWebSockets library (Husarnet fork):**

- download https://github.com/husarnet/arduinoWebSockets as a ZIP file (this is Husarnet compatible fork of arduinoWebSockets by Links2004 (Markus) )
- open `Sketch -> Include Library -> Add .ZIP Library ... `
- choose `arduinoWebSockets-master.zip` file that you just downloaded and click open button

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
- modify line 37 with your Husarnet `join code` (get on https://app.husarnet.com)
- modify lines 19 - 26 to add your Wi-Fi network credentials
- upload project to your ESP32 board.

**C. PYTHON SCRIPT: **

**1. Add your laptop/server/RaspberryPi to the same VPN network as ESP32 :**

type in the Linux terminal:

- `$ curl https://install.husarnet.com/install.sh | sudo bash` to install Husarnet.
- `$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop` replace XXX...X with your own `join code`. 

To find your join code:
a) register at https://app.husarnet.com
b) create a new network or choose existing one
c) in the chosen network click **Add element** button and go to "join code" tab

At this stage your ESP32 and your laptop are in the same VLAN network.

**2. Run sample Python 3 script on you Linux computer to display a sample rainbow theme on LED pixel strip**

- make sure Python 3 is installed on your machine
- install `asyncio` Python 3 library by typing the Linux terminal:
```py -m pip install asyncio```
- install `websockets` Python 3 library by typing the Linux terminal:
```py -m pip install websockets```
- install `numpy` Python 3 library by typing the Linux terminal:
```py -m pip install numpy```
- and finally run the script - in the project folder type in the terminal:
```py display```

After a while you should see a rainbow theme displayed on the LED pixel strip.
