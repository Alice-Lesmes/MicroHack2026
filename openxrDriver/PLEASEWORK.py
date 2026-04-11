import openvr
import time
import socket

# --- Configuration ---
ESP32_IP = "192.168.8.232" 
PORT = 8080
HAPTIC = False
TRIGGER_THRESHOLD = 0.1 # Change this prob
TIME_TO_POLL = 0.001

# Global variable to hold our persistent connection
client_socket = None

def connect_to_esp32():
    """Establishes or re-establishes the TCP connection to the ESP32."""
    global client_socket
    
    # Clean up existing socket if it exists (useful during reconnects)
    if client_socket is not None:
        try:
            client_socket.close()
        except:
            pass
        
    print(f"Attempting to connect to {ESP32_IP}:{PORT}...")
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Set a shorter timeout (2 seconds instead of 5). 
        # If the ESP32 drops, we don't want the whole VR script freezing for 5 seconds waiting for it.
        client_socket.settimeout(2.0) 
        
        client_socket.connect((ESP32_IP, PORT))
        print("Successfully connected to ESP32!")
        return True
        
    except socket.timeout:
        print("Connection timed out. ESP32 might be off or unreachable.")
        client_socket = None
        return False
    except ConnectionRefusedError:
        print("Connection refused. Is the ESP32 server running?")
        client_socket = None
        return False
    except Exception as e:
        print(f"Connection failed: {e}")
        client_socket = None
        return False

def send_message(message):
    """Sends a message over the persistent socket, auto-reconnecting if it broke."""
    global client_socket
    
    # If we don't have a socket, try to connect first
    if client_socket is None:
        if not connect_to_esp32():
            return # Give up this time, will try again on the next trigger press
            
    data_to_send = message + "\n"
    
    try:
        client_socket.sendall(data_to_send.encode('utf-8'))
        print(f"Sent: {message}")
        
    except (socket.error, BrokenPipeError, ConnectionResetError) as e:
        # This catches what happens if the ESP32 reboots while Python is still running
        print(f"\nConnection lost during send: {e}")
        print("Attempting to reconnect...")
        
        client_socket = None # Wipe the dead socket
        
        # Try to reconnect immediately
        if connect_to_esp32():
            try:
                # Retry sending the trigger pull once after a successful reconnect
                client_socket.sendall(data_to_send.encode('utf-8'))
                print(f"Sent (after reconnect): {message}")
            except Exception as retry_err:
                print(f"Failed to send after reconnect: {retry_err}")

def is_right_hand(event, vr_system, openvr) -> bool:
    if event.eventType == openvr.VREvent_Input_HapticVibration and HAPTIC:
        print("Received Haptic")
        device_id = event.trackedDeviceIndex
        device_class = vr_system.getTrackedDeviceClass(device_id)
        if device_class == openvr.TrackedDeviceClass_Controller:
            print("Is Controller")
            # 5. Ensure it is the RIGHT hand controller before firing
            role = vr_system.getControllerRoleForTrackedDeviceIndex(device_id)
            if role == openvr.TrackedControllerRole_RightHand:
                print("Is Right Hand")
                return True
    return False

# --- OpenVR Setup ---
print("Initializing OpenVR...")
vr_system = openvr.init(openvr.VRApplication_Background)
event = openvr.VREvent_t()

if __name__ == "__main__":
    
    # Connect once right when the script starts
    connect_to_esp32()
    send_message("Hello ESP32, this is Python via persistent Wi-Fi!")
    
    print("\nListening for trigger presses...")

    while True:
        # 1. Call the method to check the queue.
        has_event = vr_system.pollNextEvent(event)
        
        if has_event:
            # 2. Check if the event is a button being pressed down
            if event.eventType == openvr.VREvent_ButtonPress and not HAPTIC:
                # 3. Check if the specific button was the Trigger (Button ID 33)
                if event.data.controller.button == openvr.k_EButton_SteamVR_Trigger:
                    
                    device_id = event.trackedDeviceIndex
                    device_class = vr_system.getTrackedDeviceClass(device_id)
                    
                    # 4. Confirm it came from a controller
                    if device_class == openvr.TrackedDeviceClass_Controller:
                        
                        # 5. Ensure it is the RIGHT hand controller before firing
                        role = vr_system.getControllerRoleForTrackedDeviceIndex(device_id)
                        if role == openvr.TrackedControllerRole_RightHand:
                            right_id = vr_system.getTrackedDeviceIndexForControllerRole(openvr.TrackedControllerRole_RightHand)
                            result, pControllerState = vr_system.getControllerState(right_id)
                            while True:
                                if result:
                                    right_id = vr_system.getTrackedDeviceIndexForControllerRole(openvr.TrackedControllerRole_RightHand)
                                    result, pControllerState = vr_system.getControllerState(right_id)
                                    trigger_value = pControllerState.rAxis[1].x 
                                    if trigger_value > TRIGGER_THRESHOLD:  # Threshold for "held down"
                                        # This will fire EVERY loop iteration while held
                                        print("Sending fire serial...")
                                        send_message("F")
                                    else:
                                        break
                                    time.sleep(TIME_TO_POLL) # Poll very fast so you don't miss queue events
                                else:
                                    break
            if event.eventType == openvr.VREvent_Input_HapticVibration and HAPTIC:
                print("Received Haptic")
                device_id = event.trackedDeviceIndex
                device_class = vr_system.getTrackedDeviceClass(device_id)
                if is_right_hand(event, vr_system, openvr):
                    v_duration = event.data.hapticVibration.fDurationSeconds
                    v_end_time = time.time() + v_duration
                    while time.time() < v_duration:
                        right_id = vr_system.getTrackedDeviceIndexForControllerRole(openvr.TrackedControllerRole_RightHand)
                        result, pControllerState = vr_system.getControllerState(right_id)
                        print(f"Loop entered and result = {bool(result)}")
                        if result:
                            trigger_value = pControllerState.rAxis[1].x 
                            print(f"Trigger value = {trigger_value}")
                            if trigger_value > TRIGGER_THRESHOLD and is_right_hand(event, vr_system, openvr):  # Threshold for "held down"
                                # This will fire EVERY loop iteration while held
                                print("Sending fire serial...")
                                send_message("F")
                                time.sleep(TIME_TO_POLL) # Poll very fast so you don't miss queue events
                            else:
                                break
                        else:
                            break
        time.sleep(TIME_TO_POLL) # Poll very fast so you don't miss queue events
