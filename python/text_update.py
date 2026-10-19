
import json
import time
import serial

PORT = "/dev/cu.usbmodem2101"
BAUD = 115200

data = {
    "cpu_percent": 33,
    "ram_percent": 44,
    "temperature_c": 55.5,
    "uptime_seconds": 66,
    "network_online": True,
    "ip_address": "192.168.2.118",
    "rx_kbps": 125,
    "tx_kbps": 42,
    "ha_online": True,
    "mqtt_online": True,
    "appium_online": False,
    "ha_uptime_seconds": 18000,
    "mqtt_uptime_seconds": 17000,
    "appium_uptime_seconds": 0,
    "power_warning": False,
}

with serial.Serial(PORT, BAUD, timeout=0.2) as ser:
    print(f"Conectado a {PORT}")

    while True:
        message = json.dumps(data) + "\n"
        ser.write(message.encode())
        print("Enviado:", message.strip())

        while ser.in_waiting:
            print("ESP32:", ser.readline().decode(errors="replace").strip())

        time.sleep(0.5)
