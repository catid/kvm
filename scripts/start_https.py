from http.server import HTTPServer, BaseHTTPRequestHandler
import ssl

httpd = HTTPServer(('', 80), BaseHTTPRequestHandler)

httpd.socket = ssl.wrap_socket (httpd.socket, 
        keyfile="/home/pi/kvm/scripts/key.pem", 
        certfile='/home/pi/kvm/scripts/cert.pem', server_side=True)

httpd.serve_forever()
