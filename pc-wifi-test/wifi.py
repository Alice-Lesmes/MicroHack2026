import socket

# Replace this with the IP address printed in your ESP32 Serial Monitor!
ESP32_IP = "192.168.8.232" 
PORT = 8080

def send_message(message):
    try:
        # 1. Create a TCP/IP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            print(f"Connecting to {ESP32_IP}:{PORT}...")
            
            # Set a timeout so it doesn't hang forever if the ESP32 is off
            s.settimeout(5.0) 
            s.connect((ESP32_IP, PORT))
            
            # 2. Prepare the data
            # We add a newline '\n' because the ESP32 uses readStringUntil('\n')
            data_to_send = message + "\n"
            
            # 3. Send the data (encoded to bytes)
            s.sendall(data_to_send.encode('utf-8'))
            
            print(f"Sent: {message}")
            
    except ConnectionRefusedError:
        print("Connection refused. Is the ESP32 running and on the same network?")
    except socket.timeout:
        print("Connection timed out. Check the IP address and your firewall.")

if __name__ == "__main__":
    send_message("Hello ESP32, this is Python via Wi-Fi!")
