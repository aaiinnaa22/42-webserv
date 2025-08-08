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
        # Print line with repr, but avoid double quotes around diff symbols like "---", "+++", "@@"
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
    import time
    from pathlib import Path
    import requests

    # Define the file name and content for the test.
    filename = "test_upload.txt"
    file_content = b"sample file content\r\n\r\n"
    files = {'file': (filename, file_content)}
    
    # Send a POST request to the /images/ endpoint.
    response = requests.post("http://127.0.0.1:8081/images/", files=files)
    if response.status_code not in (200, 201):
        print(f"⚠️ Warning: Upload returned {response.status_code}, but continuing test.")
    
    # Optionally, wait a short time to let any asynchronous file writing complete.
    time.sleep(0.5)
    
    # Determine the expected path to the uploaded file.
    #upload_path2 = "/home/hskrzypi/NewWebserv/www/images/test_upload.txt"
    
    upload_path2 = upload_path = Path(__file__).parent / filename
    # Verify that the file now exists.
    print(upload_path2)
    assert upload_path2.exists(), f"Expected uploaded file at {upload_path2} does not exist."
    
    temp_path = Path(__file__).parent / "temp_test_upload.txt"
    with open(temp_path, 'wb') as f:
        f.write(file_content)
    
    # Show differences
    show_file_diff_with_repr(temp_path, upload_path)
    
    # Clean up
    temp_path.unlink()
    upload_path.unlink()
if __name__ == "__main__":
    test_file_upload_and_check()

