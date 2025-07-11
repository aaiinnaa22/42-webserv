#!/usr/bin/env python3

import os
import html
import random
from urllib.parse import parse_qs

# Sample fortunes
fortunes = [
    "You will have a great day!",
    "Something unexpected will happen soon.",
    "A new opportunity is on the horizon.",
    "Happiness begins with facing your fears.",
    "Today is a perfect day to learn something new."
]

print("Content-Type: text/html")
print()  # End of headers

query_string = os.environ.get("QUERY_STRING", "")

# Parse query string into dictionary
params = parse_qs(query_string)

# Extract name if present
name = params.get("name", ["Guest"])[0]
name = html.escape(name)  # Escape for safety

# Pick a random fortune
fortune = random.choice(fortunes)

print("<html><body>")
print(f"<h1>Hello, {name}!</h1>")
print(f"<p>Your fortune for today:</p>")
print(f"<blockquote>{fortune}</blockquote>")
