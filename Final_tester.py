#!/usr/bin/env pytest

import requests
import sys
from pathlib import Path
import http.client
import socket

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