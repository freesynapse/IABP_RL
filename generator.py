#!/usr/bin/python3

""" 
Writes fake data to a socket where we later can 
read the data to try real-time RL implementation
with PyTorch in python.
"""

import time
import sys
import os

#----------------------------------------------------------------------------------------
def generate():

	master, slave = os.openpty()
	slave = os.ttyname(slave)
	print(slave)

	while True:
		pass



#----------------------------------------------------------------------------------------
if __name__ == '__main__':
	generate()


