import socket

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to a specific IP address and port
server_address = ('', 12345)  # Use an empty string to bind to all available interfaces
sock.bind(server_address)

# Listen for incoming messages
while True:
    data, address = sock.recvfrom(1024)  # Receive data and the client's address
    #print(f"Received {data.decode().rstrip()} from {address[0]}:{address[1]}")
    print(f"Received {data.decode('utf-8', errors='ignore').rstrip()} from {address[0]}:{address[1]}")

    # Process the received data here

    # Send a response back to the client