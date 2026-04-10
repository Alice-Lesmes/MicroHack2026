import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "ESP32S3_BLE_UART"
# We write to the RX characteristic (from the ESP32's perspective)
NUS_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

async def send_ble_uart_message(message):
    print(f"Scanning for {DEVICE_NAME}...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)

    if not device:
        print("Device not found. Make sure the ESP32-S3 is running.")
        return

    print(f"Found it! Connecting to {device.address}...")

    async with BleakClient(device.address) as client:
        if client.is_connected:
            print("Connected to BLE UART!")
            
            # Encode string to bytes
            data = message.encode('utf-8')
            
            # Send the data
            await client.write_gatt_char(NUS_RX_UUID, data)
            print(f"Sent: {message}")

if __name__ == "__main__":
    asyncio.run(send_ble_uart_message("Hello ESP32-S3, this is a BLE UART stream!"))
