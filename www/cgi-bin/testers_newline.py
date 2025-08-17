#!/usr/bin/env pytest
import requests
import time

import difflib
from pathlib import Path

def show_file_diff_with_repr(path1, path2):
    with open(path1, 'r') as f1, open(path2, 'r') as f2:
        lines1 = f1.readlines()
        lines2 = f2.readlines()

    diff = difflib.unified_diff(lines1, lines2, fromfile=str(path1), tofile=str(path2))
    diff_lines = list(diff)
    
    if not diff_lines:
        print("Files are identical.")
        return
    
    print("Differences found (shown with repr to highlight exact chars):")
    for line in diff_lines:
        if line.startswith(("---", "+++", "@@")):
            print(line, end='')
        else:
            print(repr(line))

def test_file_upload_and_check():
    """
    Test that uploading a file through a POST request is successful and 
    that the file is correctly saved in the 'home/images' directory.
    
    Assumes the server routes POST requests to /images/ to the directory:
      ./home/images
    """

    filename = "test_upload.txt"
    file_content = b"sample file content\r\n\r\n"
    files = {'file': (filename, file_content)}
    upload_url = "http://127.0.0.1:8081/images/"
    print(f"Uploading to: {upload_url}")
    
    response = requests.post(upload_url, files=files)
    print(f"Upload response: {response.status_code}")
    
    if response.status_code == 404:
        print("❌ 404 Not Found – check your server routing for POST to /images/")
    
    time.sleep(0.5)

    upload_path = Path(__file__).parent.parent / "images" / filename
   
    print(f"Checking for uploaded file at: {upload_path}")
    assert upload_path.exists(), f"Expected uploaded file at {upload_path} does not exist."

    temp_path = Path(__file__).parent / "temp_test_upload.txt"
    with open(temp_path, 'wb') as f:
        f.write(file_content)
    
    show_file_diff_with_repr(temp_path, upload_path)

    temp_path.unlink()
    upload_path.unlink()

