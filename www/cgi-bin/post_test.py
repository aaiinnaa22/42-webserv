#!/usr/bin/env python3

import os
import sys


print("Content-Type: text/plain")
print()

request_method = os.environ.get("REQUEST_METHOD", "")
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

if request_method == "POST":
	post_data = sys.stdin.read(content_length)
	print(post_data, end="")
