#!/usr/bin/env python3

import os

print("Content-Type: text/plain\n")

path_info = os.environ.get("PATH_INFO", "")

print(path_info, end="")