import time
import socket

# 1. ADD SPACE_KEY TO YOUR IMPORTS
from keypress import PressKey, ReleaseKey, W_KEY, R_KEY, S_KEY, A_KEY, D_KEY, F10_KEY
from threading import Thread

# Is moving consts
STOPPED = 0
MOVING = 1

# Walk speed - for now we always run.
WALK_SPEED = 0
RUN_SPEED = 1

# Direction of the movement - for now, only forward.
MOV_CENTER = 0
MOV_BACKWARD = 2
MOV_FORWARD = 1

# If we are moving sideways - not used for now.
CENTER = 0
LEFT = 1
RIGHT = 2

# Trigger consts
TRIGGER_OFF = 0
TRIGGER_ON = 1

def check_direction(direction):
    if direction == MOV_FORWARD:
        PressKey(W_KEY)
        PressKey(R_KEY)
    elif direction == MOV_BACKWARD:
        PressKey(S_KEY)
        PressKey(R_KEY)
    else:
        ReleaseKey(W_KEY)
        ReleaseKey(S_KEY)
        ReleaseKey(R_KEY)

def check_tilt(tilt):
    if tilt != CENTER:
        if tilt == LEFT:
            PressKey(A_KEY)
        elif tilt == RIGHT:
            PressKey(D_KEY)
    else:
        ReleaseKey(A_KEY)
        ReleaseKey(D_KEY)

def should_run(speed):
    if speed == RUN_SPEED:
        PressKey(R_KEY)
    else:
        ReleaseKey(R_KEY)

def stop_movement():
    ReleaseKey(A_KEY)
    ReleaseKey(D_KEY)
    ReleaseKey(W_KEY)
    ReleaseKey(S_KEY)

# 2. CREATE A NEW FUNCTION TO HANDLE THE TRIGGER
def check_trigger(trigger_state):
    if trigger_state == TRIGGER_ON:
        PressKey(F10_KEY)
        #print("TRIGGER ON")
    else:
        ReleaseKey(F10_KEY)
        #print("TRIGGER OFF")

if __name__ == "__main__":
    print('Creating socket...')
    s = socket.socket()
    port = 8080

    s.bind(('0.0.0.0', port))
    s.listen(0)
    print(f"Socket is listening in {port}")

    while True:
        print('waiting for data...')
        c, add = s.accept()
        while True:
            content = c.recv(32)
            content = content.decode('Ascii')
            
            # Ensure we received enough data to prevent index errors
            if len(content) >= 5:
                moving = int(content[0])
                speed = int(content[1])
                direction = int(content[2])
                tilt = int(content[3])
                
                # 3. PARSE THE 5TH CHARACTER FOR THE TRIGGER
                trigger = int(content[4]) 
                
                print(content)

                if moving == MOVING:
                    should_run(speed)
                    check_tilt(tilt)
                    check_direction(direction)
                else:
                    stop_movement()
                    
                # 4. CHECK TRIGGER STATE (Independent of movement)
                check_trigger(trigger)

            c.close()
            break
