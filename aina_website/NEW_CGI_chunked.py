#!/usr/bin/env python3

import os
import sys

# CGI response headers
print("Content-Type: text/plain")
print()

# Request method
method = os.environ.get('REQUEST_METHOD', '')
print(f"Method: {method}")

# Content type and length
content_type = os.environ.get('CONTENT_TYPE', '')
content_length = os.environ.get('CONTENT_LENGTH', '')

print(f"Content-Type: {content_type}")
print(f"Content-Length: {content_length}")

# Read the body
try:
    length = int(content_length)
except (ValueError, TypeError):
    length = 0

body = sys.stdin.read(length) if length > 0 else ''
print("Body:")
print(body)
