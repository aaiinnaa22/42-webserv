#!/usr/bin/env python3
import cgi
import html

print("Content-Type: text/html\n")
form = cgi.FieldStorage()
name = form.getfirst("name", "Guest")
print("<html><body>")
print("<h1>Hello, {}</h1>".format(html.escape(name)))
print("</body></html>")
