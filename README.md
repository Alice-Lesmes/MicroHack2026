# Gun Stock Recoil
An interface to allow for a [ESP-whatever] to create recoil simulating haptic feedback in a gun-stock adapter.

# One time setup
- for wi-fi based connectivity, flash ./esp32-vr/vr-esp-receive/vr-esp-receive.ino onto an ESP32, then run [***WHERE WIFI CODE IS (test is ./pc-wifi-test/wifi.py)***]
- for bluetooth based connectivity, flash ./esp32-vr/bt-vr-esp-receive/bt-receiver/bt-receiver.ino onto ESP32, then run [*** WHERE BLUETOOTH CODE IS (test at ./pc-wifi-test/bluetooth.py)***]

# How to Run
- Run SteamVR (this means ALVR setup if you need it)
- Run OpenVR background processes (python scripts)
NOTE: To run the python scripts make sure you have `python -m pip install pyopenxr`
  Also make sure to have a python virtual environment as well.

Contributors:
- Alice-Lesmes
- Ryan-Wang
- [***ADD YOURSELF and ask to be made Contributors***]
