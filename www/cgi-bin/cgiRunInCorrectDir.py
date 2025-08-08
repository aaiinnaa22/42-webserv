#!/usr/bin/env python3

import os 

print("Content-Type: text/plain")

try:
	with open("tempTestCgi.txt", "r") as f: 
		content = f.read()
	print("Status: 200\n")
except FileNotFoundError:
	print("Status: 404\n")