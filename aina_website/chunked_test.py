import socket
import time

HOST = '127.0.0.1'
PORT = 1234
PATH = '/BB_test.txt'

# Generate ~512KB body
body = b"B" * (512 * 1024)
chunk_size = 4096  # 4KB chunks

def run():
    with socket.create_connection((HOST, PORT)) as sock:
        # Send the headers first
        headers = (
            f"POST {PATH} HTTP/1.1\r\n"
            f"Host: {HOST}\r\n"
            f"Content-Length: {len(body)}\r\n"
            f"Content-Type: text/plain\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        )
        sock.sendall(headers.encode())

        # Now send the body in chunks
        total_sent = 0
        for i in range(0, len(body), chunk_size):
            chunk = body[i:i + chunk_size]
            sock.sendall(chunk)
            total_sent += len(chunk)
            print(f"🚀 Sent {len(chunk)} bytes, total sent: {total_sent / 1024:.1f} KB")
            time.sleep(0.05)  # Simulate delay between chunks (50ms)

        # Read the response
        response_data = b""
        total = 0
        while True:
            chunk = sock.recv(8192)
            if not chunk:
                break
            total += len(chunk)
            response_data += chunk
            print(f"📦 Received {len(chunk)} bytes, total: {total / 1024:.1f} KB")

        print(f"\n✅ Final total received: {total / 1024:.1f} KB")
        print("\n🔽 Response:")
        print(response_data.decode(errors="replace"))

if __name__ == "__main__":
    run()