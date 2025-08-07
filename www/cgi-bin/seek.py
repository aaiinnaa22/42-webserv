import requests

# Define color codes for terminal output
class bcolors:
    OKGREEN = '\033[92m'
    ENDC = '\033[0m'
    FAIL = '\033[91m'

def test_get_request(url, expected_status):
    try:
        print(f"GET {url}")
        response = requests.get(url)
        assert response.status_code == expected_status, \
            f"Expected {expected_status}, got {response.status_code}"
        print(f"{bcolors.OKGREEN}OK (Status: {response.status_code}){bcolors.ENDC}")
    except requests.exceptions.RequestException as e:
        print(f"{bcolors.FAIL}FAIL (Request failed: {e}){bcolors.ENDC}")
    except AssertionError as e:
        print(f"{bcolors.FAIL}FAIL (Assertion error: {e}){bcolors.ENDC}")
    except:
         print("An exception occurred") 

def test_post_request(url, expected_status):
    try:
        files = {'file': ('filename.txt', b"dummy data\n")}
        print(f"POST {url}")
        response = requests.post(url, files=files)
        assert response.status_code == expected_status, \
            f"Expected {expected_status}, got {response.status_code}"
        print(f"{bcolors.OKGREEN}OK (Status: {response.status_code}){bcolors.ENDC}")
    except requests.exceptions.RequestException as e:
        print(f"{bcolors.FAIL}FAIL (Request failed: {e}){bcolors.ENDC}")
    except AssertionError as e:
        print(f"{bcolors.FAIL}FAIL (Assertion error: {e}){bcolors.ENDC}")
    except:
         print("An exception occurred") 

def test_delete_request(url, expected_status):
    try:
        print(f"POST {url}")
        response = requests.delete(url, files)
        assert response.status_code == expected_status, \
            f"Expected {expected_status}, got {response.status_code}"
        print(f"{bcolors.OKGREEN}OK (Status: {response.status_code}){bcolors.ENDC}")
    except requests.exceptions.RequestException as e:
        print(f"{bcolors.FAIL}FAIL (Request failed: {e}){bcolors.ENDC}")
    except AssertionError as e:
        print(f"{bcolors.FAIL}FAIL (Assertion error: {e}){bcolors.ENDC}")
    except:
         print("An exception occurred") 

print("***TESTING GET***")
test_get_request("http://127.0.0.1:8080/index.html", 200)
test_get_request("http://127.0.0.1:8080/cat.html", 404)
test_get_request("http://127.0.0.1:8080/oldDir/", 200)
test_get_request("http://127.0.0.1:8080/newDir/", 200)

print("***TESTING POST***")
test_post_request("http://127.0.0.1:8080/images/", 200)

print("***TESTING DELETE***")