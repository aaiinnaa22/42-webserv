#!/usr/bin/env pytest

import requests
import sys
from pathlib import Path


cgi_bin_path = Path(__file__).parent / "www" / "cgi-bin"
print(f"Adding to sys.path: {cgi_bin_path}")
sys.path.append(str(cgi_bin_path))

from testers_newline import *


def test_get_root():
    """Test that GET /index.html returns 200."""
    response = requests.get("http://127.0.0.1:8081/index.html")
    assert response.status_code == 200

def test_get_root_bad():
    """Test that GET /indexx.html returns 200."""
    response = requests.get("http://127.0.0.1:8081/indexx.html")
    assert response.status_code == 404

def test_number_php_script():
    url = "http://127.0.0.1:8081/cgi-bin/number.php?number=42"
    response = requests.get(url)

    assert response.status_code == 200

def test_fortune_python_script():
    url = "http://127.0.0.1:8081/cgi-bin/aina_test.py?name=Leo"
    response = requests.get(url)

    assert response.status_code == 200

#Aina

def test_post_php_cgi():
	url = "http://127.0.0.1:8081/cgi-bin/cgi-bin/post_test.php"
	post_body = "cgi-php is amazing wow"

	response = requests.post(url, data=post_body)

	assert response.text == post_body

	

#Aina end
def test_upload_file():
    test_file_upload_and_check()

