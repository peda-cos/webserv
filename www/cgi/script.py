import os; 

# // get enviroments variables for CGI execution e log them to the console
print("SCRIPT_NAME: " + os.environ.get('SCRIPT_NAME', ''))
print("REQUEST_METHOD: " + os.environ.get('REQUEST_METHOD', ''))
print("CONTENT_TYPE: " + os.environ.get('CONTENT_TYPE', ''))
print("CONTENT_LENGTH: " + os.environ.get('CONTENT_LENGTH', ''))
print("SERVER_PROTOCOL: " + os.environ.get('SERVER_PROTOCOL', ''))
print("SERVER_SOFTWARE: " + os.environ.get('SERVER_SOFTWARE', ''))
print("GATEWAY_INTERFACE: " + os.environ.get('GATEWAY_INTERFACE', ''))
print("REQUEST_URI: " + os.environ.get('REQUEST_URI', ''))
print("PATH_INFO: " + os.environ.get('PATH_INFO', ''))
print("REMOTE_ADDR: " + os.environ.get('REMOTE_ADDR', ''))

# Get parameter recived from POST request body and log it to the console if the request method is POST
if os.environ.get('REQUEST_METHOD', 'GET') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    post_data = os.read(0, content_length).decode('utf-8')
    print("Received POST data: " + post_data)