# rpiTouchNicla (Nicla Hat on Raspberry Pi)

This project reads sensor data from a **Nicla HAT** connected to the Raspberry Pi’s **UART** and (optionally) publishes it to **Home Assistant** via the **Mosquitto MQTT broker** using Home Assistant’s **MQTT Discovery**.

- Serial device on the Pi: `/dev/serial0` (symlink to `/dev/ttyS0`)
- Script: `nicla.py`
- MQTT Discovery prefix (Home Assistant default): `homeassistant`

---

## 1) Hardware / Serial Setup (Raspberry Pi)

### Enable Serial Interface
1. Open Raspberry Pi Configuration:
   - `sudo raspi-config`
2. Go to:
   - **Interface Options** → **Serial Port**
3. Recommended settings:
   - **Login shell over serial?** → **No**
   - **Enable serial hardware?** → **Yes**

Reboot:
```bash
sudo reboot
```

### Confirm Serial Device
After reboot:
```bash
ls -l /dev/serial0
```

You should see something like:
- `/dev/serial0 -> ttyS0`

---

## 2) Home Assistant (Mosquitto MQTT Broker)

Install Mosquitto broker add-on:
- Home Assistant → **Settings** → **Add-ons** → **Mosquitto broker**

Create or confirm MQTT credentials (example):
- Broker IP: ``
- Port: `1883`
- Username: ``
- Password: ``

> Note: Use your own broker IP/credentials.

---

## 3) Python Environment (venv)

Create a virtual environment:
```bash
python3 -m venv ~/nicla_env
```

Activate it:
```bash
source ~/nicla_env/bin/activate
```

Install dependencies:
```bash
pip install pyserial paho-mqtt python-dotenv
```

Deactivate when done:
```bash
deactivate
```

---

## 4) Keep MQTT Credentials Out of Git (Recommended)

### Option A: Use a `.env` file (recommended)
Create a `.env` file next to `nicla.py` (example: `~/Desktop/.env`):

```env
MQTT_ENABLED=true
MQTT_BROKER=
MQTT_PORT=1883
MQTT_USER=
MQTT_PASS=
CLIENT_ID=rpiTouchNicla
DISCOVERY_PREFIX=homeassistant
BASE_TOPIC=rpiTouchNicla
SERIAL_PORT=/dev/serial0
SERIAL_BAUD=115200
```

Add `.env` to `.gitignore`:
```gitignore
.env
__pycache__/
*.pyc
.venv/
venv/
```

Also commit a safe template for others:
- `.env.example` (same keys, placeholder values)

---

## 5) Run Manually (for testing)

From wherever your script lives (example Desktop):
```bash
source ~/nicla_env/bin/activate
python3 ~/Desktop/nicla.py
```

The script will:
- Always read + parse serial data and print it
- Publish to MQTT only if MQTT is reachable
- Publish MQTT Discovery messages (retained) when MQTT is connected so Home Assistant auto-creates entities

---

## 6) Run Automatically on Boot (systemd)

### Create a service
Create:
```bash
sudo nano /etc/systemd/system/nicla.service
```

Paste (update `User=` and paths if your username/location differs):
```ini
[Unit]
Description=Nicla Serial to MQTT bridge (rpiTouchNicla)
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/Desktop
ExecStart=/home/pi/nicla_env/bin/python /home/pi/Desktop/nicla.py
Restart=always
RestartSec=3
Environment=PYTHONUNBUFFERED=1
EnvironmentFile=-/home/pi/Desktop/.env

[Install]
WantedBy=multi-user.target
```

Enable + start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable --now nicla.service
```

Check status/logs:
```bash
systemctl status nicla.service
journalctl -u nicla.service -f
```

---

## 7) Troubleshooting

### Serial permission denied
Add your user to `dialout`:
```bash
sudo usermod -aG dialout $USER
```
Then reboot:
```bash
sudo reboot
```

### UART is busy (serial console still enabled)
Disable the serial getty:
```bash
sudo systemctl disable --now serial-getty@ttyS0.service
```

### Garbled serial output
Almost always a baud mismatch. Confirm the Nicla is set to the same baud rate as `SERIAL_BAUD` (default shown: `115200`).

### MQTT entities not showing in Home Assistant
- Confirm MQTT integration is configured in HA
- Verify the script is publishing discovery configs:
  - Discovery topics look like:
    - `homeassistant/sensor/rpiTouchNicla/temperature/config`
- Check logs:
  ```bash
  journalctl -u nicla.service -f
  ```

---

## Notes
- Acceleration works fine in Home Assistant; it shows up as separate MQTT sensors (e.g., `acc_xAve`, `acc_yAve`, etc.).
- If you want proper units for acceleration (`m/s²` or `g`), set the `unit_of_measurement` in the discovery payload accordingly.
