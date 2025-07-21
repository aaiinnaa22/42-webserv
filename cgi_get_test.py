import socket

HOST = '127.0.0.1'
PORT = 1234

request = (
    "GET /test.py HTTP/1.1\r\n"
    "Host: dfdf\r\n"
    "\r\n"
)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(request.encode('utf-8'))
    response = s.recv(4096)

    print(response.decode('utf-8'))
