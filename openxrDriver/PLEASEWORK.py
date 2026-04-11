import openvr
import time
import socket
vr_system = openvr.init(openvr.VRApplication_Background)

# Create an empty event object to populate
event = openvr.VREvent_t()

print("Listening for trigger presses...")


ESP32_IP = "192.168.8.232" 
PORT = 8080

def send_message(message):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            print(f"Connecting to {ESP32_IP}:{PORT}...")
            
            # Set a timeout so it doesn't hang forever if the ESP32 is off
            s.settimeout(5.0) 
            s.connect((ESP32_IP, PORT))
            
            # We add a newline '\n' because the ESP32 uses readStringUntil('\n')
            data_to_send = message + "\n"
            
            s.sendall(data_to_send.encode('utf-8'))
            
            print(f"Sent: {message}")
            
    except ConnectionRefusedError:
        print("Connection refused. Is the ESP32 running and on the same network?")
    except socket.timeout:
        print("Connection timed out. Check the IP address and your firewall.")

if __name__ == "__main__":
    send_message("Hello ESP32, this is Python via Wi-Fi!")

while True:
    # 1. Call the method to check the queue.
    has_event = vr_system.pollNextEvent(event)
    
    if has_event:
        # 2. Check if the event is a button being pressed down
        if event.eventType == openvr.VREvent_ButtonPress:
            
            # 3. Check if the specific button was the Trigger (Button ID 33)
            if event.data.controller.button == openvr.k_EButton_SteamVR_Trigger:
                
                device_id = event.trackedDeviceIndex
                device_class = vr_system.getTrackedDeviceClass(device_id)
                
                # 4. Confirm it came from a controller (not a tracker or headset)
                if device_class == openvr.TrackedDeviceClass_Controller:
                    
                    # (Optional) You can check if it's the Left or Right controller
                    # role = vr_system.getControllerRoleForTrackedDeviceIndex(device_id)
                    # if role == openvr.TrackedControllerRole_RightHand:
                    
                    print(f"Trigger pressed on controller {device_id}!")
                    print("Sending UDP packet to ESP32...")
                    send_message("F");
                
    time.sleep(0.001) # Poll very fast so you don't miss queue events
