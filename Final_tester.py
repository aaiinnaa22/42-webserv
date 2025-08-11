#!/usr/bin/env pytest

import requests
import sys
from pathlib import Path
import http.client
import socket
import time

cgi_bin_path = Path(__file__).parent / "www" / "cgi-bin"
print(f"Adding to sys.path: {cgi_bin_path}")
sys.path.append(str(cgi_bin_path))

from testers_newline import *

HOST = "127.0.0.1"
PORT = 8081
BASE_URL = f"http://{HOST}:{PORT}"

def test_get_root():
    """Test that GET /index.html returns 200."""
    response = requests.get(f"{BASE_URL}/index.html")
    assert response.status_code == 200

def test_get_root_bad():
    """Test that GET /indexx.html returns 200."""
    response = requests.get(f"{BASE_URL}/indexx.html")
    assert response.status_code == 404

def test_number_php_script():
    response = requests.get(f"{BASE_URL}/cgi-bin/number.php?number=42")
    assert response.status_code == 200
    assert response.text == "You entered the correct number: 42!"

def test_fortune_python_script():
    response = requests.get(f"{BASE_URL}/cgi-bin/aina_test.py?name=Leo")
    assert response.status_code == 200

def test_upload_file():
    test_file_upload_and_check()

def test_head_root():
    response = requests.head(f"{BASE_URL}/index.html")
    assert response.status_code == 405

def test_post_without_content_length():
    conn = http.client.HTTPConnection(f"{HOST}", 8081)
    conn.putrequest("POST", "/test.txt")
    conn.putheader("Accept", "text/plain")
    conn.endheaders()

    response = conn.getresponse()
    status_code = response.status
    conn.close()

    assert status_code == 411, f"Expected 411 Length Required, got {status_code}"

def test_bad_request():
    conn = http.client.HTTPConnection(f"{HOST}", PORT)
    conn.putrequest("GET", "bla")
    conn.putheader("Accept", "text/plain")
    conn.endheaders()

    response = conn.getresponse()
    status_code = response.status
    conn.close()

    assert status_code == 400, f"Expected 400 Bad Request, got {status_code}"

def test_missing_host_header():
    with socket.create_connection((HOST, PORT)) as sock:
        request = b"GET / HTTP/1.1\r\n\r\n"
        sock.sendall(request)

        response = sock.recv(1024).decode("utf-8", errors="ignore")

        status_line = response.splitlines()[0]
        assert "400" in status_line, f"Expected 400 Bad Request, got: {status_line}"

def test_infine_cgi():
	response = requests.get(f"{BASE_URL}/cgi-bin/infinite_cgi_test.py")
	assert response.status_code == 500

def test_spam():
    for i in range(42):
        response = requests.get(f"{BASE_URL}/index.html")
        assert response.status_code == 200
        if i % 3 == 0:
            test_missing_host_header()
        if i % 5 == 0:
            test_fortune_python_script()
        response = requests.get(f"{BASE_URL}/images/Leo.jpg")
        assert response.status_code == 200

def test_slow_chunks():
    conn = http.client.HTTPConnection(f"{HOST}", PORT)
    conn.putrequest("POST", "/test.txt")
    conn.putheader("Content-Type", "text/plain")
    conn.putheader("Transfer-Encoding", "chunked")
    conn.endheaders()
    data = b"A"
    for i in range(10):
        conn.send(f"{len(data):X}\r\n".encode("utf-8"))
        conn.send(data + b"\r\n")
        time.sleep(1)
    conn.send(b"0\r\n\r\n")
    
    response = conn.getresponse()
    assert response.status == 201

def test_delete():
    response = requests.delete(f"{BASE_URL}/test.txt")
    assert response.status_code == 204

def test_broken_cgi():
    response = requests.get(f"{BASE_URL}/cgi-bin/broken.py")
    assert response.status_code == 403

def test_redirection():
    conn = http.client.HTTPConnection(f"{HOST}", PORT)
    conn.putrequest("GET", "/imagesREDIR/Leo.jpg")
    conn.endheaders()

    response = conn.getresponse()
    assert response.headers.get("Location", "") == "/images/Leo.jpg"
    assert response.status == 307

def test_bad_redirection():
    conn = http.client.HTTPConnection(f"{HOST}", PORT)
    conn.putrequest("GET", "/uploadREDIR/Leo.jpg")
    conn.endheaders()

    response = conn.getresponse()
    assert response.status == 404

