import socket
import sys
import argparse

# Create ArgumentParser() object
parser = argparse.ArgumentParser()

# Add argument
parser.add_argument('--method', required=True, help='path to dataset')
args = parser.parse_args()
data = args.method
print data

HOSTIP = "127.0.0.1"
PORT = 8113
client = socket.socket()
client.connect((HOSTIP,PORT))
client.send(data)
#data_recv = client.recv(1024)
#print(data_recv.decode())
client.close()



