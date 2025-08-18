#!/usr/bin/env pytest
import requests
import time

import difflib
from pathlib import Path

def show_file_diff_with_repr(path1, path2):
    """Compares the contents of two text files line by line, prints their differences. 
    Function wraps non-header lines with repr() so that invisible characters 
    (such as spaces, tabs, or newline differences) are clearly visible.
    """
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
    assert not diff_lines, f"Files {path1} and {path2} differ (see diff above)"

def test_file_upload_and_check():
    """
    Test to check if a file upload POST request works,
    the file is saved in the 'home/images' directory
    and there are no differences between the POST and GET file content.
    
    Expected routing for POST: /images/, eg: ./home/images
    """

    fname = "test_upload.txt"
    fcontent = b"sample file content\r\n\r\n"
    files = {'file': (fname, fcontent)}
    upload_url = "http://127.0.0.1:8081/images/"
    print(f"Uploading to: {upload_url}")
    
    response = requests.post(upload_url, files=files)
    print(f"Upload response: {response.status_code}")
    
    assert response.status_code in (200, 201), f"Upload failed with {response.status_code}"
    if response.status_code == 404:
        print("❌ 404 Not Found – check your server routing for POST to /images/")
    
    time.sleep(0.5)

    upload_path = Path(__file__).parent.parent / "images" / fname
   
    print(f"Path checked for uploaded file: {upload_path}")
    assert upload_path.exists(), f"Upload at {upload_path} does not exist."

    temp_path = Path(__file__).parent / "temp_test_upload.txt"
    with open(temp_path, 'wb') as f:
        f.write(fcontent)
    
    show_file_diff_with_repr(temp_path, upload_path)

    temp_path.unlink()
    upload_path.unlink()

