
import json
import time
import serial
import glob
import json
import socket
import subprocess
import time
from datetime import datetime

import psutil

# PORT = "/dev/cu.usbmodem2101"
# PORT = "/dev/ttyACM0"
PORT = "/dev/ttyS0"
BAUD = 115200

# dafaultData = {
#     "cpu_percent": 0,
#     "ram_percent": 0,
#     "temperature_c": 0.0,
#     "uptime_seconds": 0,
#     "network_online": False,
#     "ip_address": "-.-.-.-",
#     "rx_kbps": 0,
#     "tx_kbps": 0,
#     "ha_online": False,
#     "mqtt_online": False,
#     "appium_online": False,
#     "ha_uptime_seconds": 0,
#     "mqtt_uptime_seconds": 0,
#     "appium_uptime_seconds": 0,
#     "power_warning": False,
# }

class SystemMetrics:
    def __init__(self):
        self.last_net = psutil.net_io_counters(pernic=True)
        self.last_net_time = time.monotonic()
        self.last_services_check = 0
        self.services_cache = {}
        psutil.cpu_percent(interval=None)

    def _temperature(self):
        for path in [
            "/sys/class/thermal/thermal_zone0/temp",
            *glob.glob("/sys/class/hwmon/hwmon*/temp*_input"),
        ]:
            try:
                with open(path) as f:
                    return round(int(f.read().strip()) / 1000, 1)
            except (OSError, ValueError):
                continue
        return 0.0

    def _network(self):
        try:
            route = subprocess.check_output(
                ["ip", "route", "show", "default"],
                text=True,
                timeout=1,
            ).splitlines()[0]
            interface = route.split("dev ")[1].split()[0]
        except (subprocess.SubprocessError, IndexError):
            interface = ""

        addresses = psutil.net_if_addrs().get(interface, [])
        ip_address = next(
            (a.address for a in addresses if a.family == socket.AF_INET),
            "",
        )

        current = psutil.net_io_counters(pernic=True).get(interface)
        now = time.monotonic()
        elapsed = now - self.last_net_time
        rx_kbps = tx_kbps = 0.0

        if current and interface in self.last_net and elapsed > 0:
            previous = self.last_net[interface]
            rx_kbps = (current.bytes_recv - previous.bytes_recv) / 1024 / elapsed
            tx_kbps = (current.bytes_sent - previous.bytes_sent) / 1024 / elapsed

        self.last_net = psutil.net_io_counters(pernic=True)
        self.last_net_time = now

        return bool(ip_address), ip_address, round(rx_kbps, 1), round(tx_kbps, 1)

    def _container(self, name):
        try:
            output = subprocess.check_output(
                [
                    "docker", "inspect",
                    "--format", "{{.State.Running}}|{{.State.StartedAt}}",
                    name,
                ],
                text=True,
                stderr=subprocess.DEVNULL,
                timeout=2,
            ).strip()

            running, started_at = output.split("|", 1)
            if running != "true":
                return False, 0

            started = datetime.fromisoformat(started_at.replace("Z", "+00:00"))
            uptime = max(
                0,
                int(datetime.now(started.tzinfo).timestamp() - started.timestamp()),
            )
            return True, uptime
        except (subprocess.SubprocessError, ValueError, OSError):
            return False, 0

    def _appium(self):
        try:
            with socket.create_connection(("127.0.0.1", 4723), timeout=0.3):
                pass
        except OSError:
            return False, 0

        for process in psutil.process_iter(["cmdline", "create_time"]):
            try:
                command = " ".join(process.info["cmdline"] or []).lower()
                if "appium" in command:
                    return True, max(
                        0, int(time.time() - process.info["create_time"])
                    )
            except (psutil.Error, TypeError):
                continue

        return True, 0

    def _refresh_services(self):
        now = time.monotonic()
        if now - self.last_services_check < 15:
            return

        self.services_cache["ha_online"], self.services_cache["ha_uptime_seconds"] = (
            self._container("homeassistant")
        )
        self.services_cache["mqtt_online"], self.services_cache["mqtt_uptime_seconds"] = (
            self._container("mosquitto")
        )
        self.services_cache["appium_online"], self.services_cache["appium_uptime_seconds"] = (
            self._appium()
        )
        self.last_services_check = now

    def get_data(self):
        network_online, ip_address, rx_kbps, tx_kbps = self._network()
        self._refresh_services()

        return {
            "cpu_percent": int(psutil.cpu_percent(interval=None)),
            "ram_percent": int(psutil.virtual_memory().percent),
            "temperature_c": self._temperature(),
            "uptime_seconds": int(time.time() - psutil.boot_time()),
            "network_online": network_online,
            "ip_address": ip_address,
            "rx_kbps": rx_kbps,
            "tx_kbps": tx_kbps,
            "ha_online": self.services_cache.get("ha_online", False),
            "mqtt_online": self.services_cache.get("mqtt_online", False),
            "appium_online": self.services_cache.get("appium_online", False),
            "ha_uptime_seconds": self.services_cache.get("ha_uptime_seconds", 0),
            "mqtt_uptime_seconds": self.services_cache.get("mqtt_uptime_seconds", 0),
            "appium_uptime_seconds": self.services_cache.get("appium_uptime_seconds", 0),
            "power_warning": False,
        }

metrics = SystemMetrics()

with serial.Serial(PORT, BAUD, timeout=0.2) as ser:
    print(f"Conectado a {PORT}", flush=True)
    metrics = SystemMetrics()
    data = metrics.get_data()
    last_metrics = time.monotonic()

    while True:
        now = time.monotonic()

        if now - last_metrics >= 2:
            data = metrics.get_data()
            last_metrics = now

        message = json.dumps(data) + "\n"
        try:
            ser.write(message.encode())
        except serial.SerialException as e:
            print(f"Error escribiendo UART: {e}", flush=True)
            raise

        while ser.in_waiting:
            line = ser.readline().decode(errors="replace").strip()
            print("ESP32:", line)

            if line == "CMD:REBOOT":
                print("Comando reboot recibido", flush=True)
                ser.close()
                subprocess.Popen(["/usr/bin/systemctl", "reboot"])
                break
            elif line == "CMD:SHUTDOWN":
                print("Comando powerdown recibido", flush=True)
                ser.close()
                subprocess.Popen(["/usr/bin/systemctl", "poweroff"])
                break

        time.sleep(0.5)