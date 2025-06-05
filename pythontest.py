import requests
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


print("***TESTING GET***")
print("GET http://127.0.0.1:1234/index.html")
r = requests.get("http://127.0.0.1:1234/index.html")
assert r.status_code == 200
print(bcolors.OKGREEN + "OK" + bcolors.ENDC)


print("GET http://127.0.0.1:1234/cat.html")
r = requests.get("http://127.0.0.1:1234/cat.html")
assert r.status_code == 404
print(bcolors.OKGREEN + "OK" + bcolors.ENDC)

print("GET http://127.0.0.1:1234/index.htm")
r = requests.get("http://127.0.0.1:1234/index.htm")
print(r)
assert r.status_code == 415
print(bcolors.OKGREEN + "OK" + bcolors.ENDC)

print("DELETE http://127.0.0.1:1234/nodelete.html")
r = requests.get("http://127.0.0.1:1234/nodelete.html")
print(r)
assert r.status_code == 403
print(bcolors.OKGREEN + "OK" + bcolors.ENDC)

"""
print("POST")
post_request("POST test.txt HTTP/1.1\r\n"
    "Content-Length: 40\r\n"
    "\r\n"
    "I am the content of test.txtr\r\n")
sock.sendall
print(r)
assert r.status_code == 200
"""
