import socket

UDP_IP = "10.0.0.69"
UDP_PORT = 8000
MESSAGE = "Hello, World!"

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
while True:
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
