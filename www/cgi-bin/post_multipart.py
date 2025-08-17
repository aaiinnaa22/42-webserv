import socket

HOST = '127.0.0.1'
PORT = 8081

boundary = '----WebKitFormBoundary7MA4YWxkTrZu0gW'
multipart_data = (
    f"--{boundary}\r\n"
    "Content-Disposition: form-data; name=\"uploaded_file\"; filename=\"hello.txt\"\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "Hello, this is the content of the uploaded file.\n"
    "It can be multiple lines.\n"
    f"--{boundary}--\r\n"
)

content_length = len(multipart_data)

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost:1234\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n"
    f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
    f"Content-Length: {content_length}\r\n"
    "\r\n"
    + multipart_data
)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(request.encode('utf-8'))

    response = b""
    while True:
        part = s.recv(4096)
        if not part:
            break
        response += part

    print(response.decode('utf-8', errors='ignore'))
