import requests
print("***TESTING GET***")
print("GET http://127.0.0.1:1234/index.html")
r = requests.get("http://127.0.0.1:1234/index.html")
print (r)
assert r.status_code == 200

print("GET http://127.0.0.1:1234/cat.html")
r = requests.get("http://127.0.0.1:1234/cat.html")
print(r)
assert r.status_code == 200
