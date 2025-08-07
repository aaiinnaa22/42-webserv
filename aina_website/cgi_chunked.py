#!/usr/bin/env python3

import socket
import time

HOST = '127.0.0.1'
PORT = 1234
PATH = '/NEW_CGI_chunked.py'  # Make sure this matches your actual CGI script path

# These are the chunks of data we'll send to simulate a real chunked POST body
body_chunks = [
    b"first part of the body\n",
    b"second chunk comes now\n",
    b"last little bit\n"
]

def run():
    with socket.create_connection((HOST, PORT)) as sock:
        # Send request headers with Transfer-Encoding
        headers = (
            f"POST {PATH} HTTP/1.1\r\n"
            f"Host: {HOST}\r\n"
            f"Transfer-Encoding: chunked\r\n"
            f"Content-Type: text/plain\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        )
        sock.sendall(headers.encode())
        print("🚀 Sent headers")

        # Send the body in HTTP chunked format
        total_sent = 0
        for chunk in body_chunks:
            size_line = f"{len(chunk):X}\r\n".encode()  # hex length + CRLF
            sock.sendall(size_line)
            sock.sendall(chunk + b"\r\n")
            total_sent += len(chunk)
            print(f"📤 Sent chunk ({len(chunk)} bytes)")
            time.sleep(0.2)  # optional delay

        # Final chunk (0 length)
        sock.sendall(b"0\r\n\r\n")
        print("✅ Sent terminating chunk")

        # Read response
        response_data = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response_data += chunk

        print("\n🔽 Response:")
        print(response_data.decode(errors="replace"))

if __name__ == "__main__":
    run()
