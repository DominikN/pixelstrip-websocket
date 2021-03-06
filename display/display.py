#! python3 .

# Control LED strip over websocket example

import asyncio
import websockets
import numpy as np 
import sys
import json

from array import *

NUMPIXELS = 150

def rainbow():
	k = 0
	l = 0
	m = 0
	i = 0
	
	strip = np.array(np.zeros((NUMPIXELS,3)), dtype=np.uint8)
	
	for i in range(NUMPIXELS):
		if (i >= 0) and (i < NUMPIXELS * 1 / 3):
			red = np.uint8(255 * k / (NUMPIXELS / 3))
			green = np.uint8(0)
			blue = np.uint8(255 - (255 * k / (NUMPIXELS / 3)))
			k = k + 1
		if (i >= NUMPIXELS * 1 / 3) and (i < NUMPIXELS * 2 / 3):
			red = np.uint8(255 - (255 * l / (NUMPIXELS / 3) ))
			green = np.uint8(255 * l / (NUMPIXELS / 3))
			blue = np.uint8(0)
			l = l + 1
		if (i >= NUMPIXELS * 2 / 3) and (i < NUMPIXELS * 3 / 3):
			red = np.uint8(0)
			green = np.uint8(255 - (255 * m / (NUMPIXELS / 3) ))
			blue = np.uint8(255 * m / (NUMPIXELS / 3))
			m = m + 1
			
		strip[i][0] = red
		strip[i][1] = green
		strip[i][2] = blue
	
	return strip
	
def oneblink():
	i = 0
	strip = np.array(np.zeros((150,3)), dtype=np.uint8)
	
	for i in range(150):
		strip[i][0] = 1
		strip[i][1] = 1
		strip[i][2] = 1
			
	strip[0][0] = 0
	strip[0][1] = 25
	strip[0][2] = 0
	
	return strip
	
async def mainfunc():
	uri = "ws://pixelstrip:8001"
	async with websockets.connect(uri) as websocket:
		settings = json.dumps({"mode":"sequence", "numpixel":50, "buffer":50, "delay":100})
		# settings = json.dumps({"mode":"auto", "numpixel":150, "buffer":150, "delay":-1})
		print(settings)
		await websocket.send(settings)
		
		a = rainbow()
		# a = oneblink()
		a = a.flatten()
		print(a)
		await asyncio.sleep(0.2)
		while True:
			a = np.roll(a,3)
			
			sys.stdout.write('.')
			sys.stdout.flush()
			
			await asyncio.sleep(0.1)
			await websocket.send(a.tobytes())
			
try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
