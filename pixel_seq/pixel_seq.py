#! python3 .
# Control LED strip over websocket example

import asyncio
import websockets
import numpy as np 
import sys
import argparse
import json

import matplotlib.pyplot as plt

from array import *

NUMPIXELS = 50

def cmap_tab():
	cmap = plt.get_cmap(args.theme)
	
	colors_i = np.linspace(0, 1., NUMPIXELS/2)
	
	# convert float to uint8
	tab = np.uint8(255 * cmap(colors_i))
	
	# delete white channel
	tab = np.delete(tab, np.arange(3, tab.size, 4))
	
	# create mirrored pixels
	tab2 = np.reshape(tab,(-1,3))
	tab2 = np.flip(tab2,0)
	tab2 = tab2.flatten()
	
	# connect original and mirrored pixel table
	tab = np.concatenate((tab, tab2), axis=0)
	
	# delete NUMPIXEL+1'st pixel
	# tab = np.delete(tab, [0,1,2], 0)
	
	return tab

async def mainfunc():
	uri = "ws://pixelstrip:8001"
	async with websockets.connect(uri) as websocket:
		settings = json.dumps({"mode":"sequence", "numpixel":NUMPIXELS, "buffer":NUMPIXELS, "delay":int(1000*args.delay)})
		print(settings)
		await websocket.send(settings)
		
		a = cmap_tab()
	
		print("pixel table:")
		print(a)
		print("programming sequence: ")
		
		for x in range(NUMPIXELS):	
			await websocket.send(a.tobytes())
			await asyncio.sleep(0.05)
			
			a = np.roll(a,3)
			
			sys.stdout.write('.')
			sys.stdout.flush()
		
		print("done")
		
			
parser = argparse.ArgumentParser()

parser.add_argument("theme", nargs='?', default="autumn", help="Theme to be displayed on a pixel strip. Available themes are listed here: https://matplotlib.org/3.1.0/tutorials/colors/colormaps.html . Default = autumn")

parser.add_argument("delay", nargs='?', default=0.05, type=float, help="delay between frames in seconds. Default = 0.05")

args = parser.parse_args()

print("theme: ", args.theme)
print("delay: ", args.delay, "[s]")

try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
