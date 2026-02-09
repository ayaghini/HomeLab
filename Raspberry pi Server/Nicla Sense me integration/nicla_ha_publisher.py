#!/usr/bin/env python3
import json
import time
from datetime import datetime

import serial
from paho.mqtt import client as mqtt_client

try:
    import config as cfg
except ImportError:
    raise SystemExit(
        "Missing config.py. Copy config.example.py to config.py and edit your credentials."
    )

SERIAL_PORT = cfg.SERIAL_PORT
SERIAL_BAUD = cfg.SERIAL_BAUD

MQTT_ENABLED = cfg.MQTT_ENABLED
MQTT_BROKER = cfg.MQTT_BROKER
MQTT_PORT = cfg.MQTT_PORT
MQTT_USER = cfg.MQTT_USER
MQTT_PASS = cfg.MQTT_PASS

CLIENT_ID = cfg.CLIENT_ID
DISCOVERY_PREFIX = cfg.DISCOVERY_PREFIX
BASE_TOPIC = cfg.BASE_TOPIC

MQTT_RECONNECT_EVERY_SEC = 5


def utc_now_iso() -> str:
    return datetime.utcnow().isoformat()


def open_serial() -> serial.Serial:
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
    ser.reset_input_buffer()
    return ser


def build_mqtt_client() -> mqtt_client.Client:
    c = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION1, CLIENT_ID)
    c.username_pw_set(MQTT_USER, MQTT_PASS)

    def on_connect(_client, _userdata, _flags, rc):
        if rc == 0:
            print("MQTT connected")
        else:
            print(f"MQTT connect failed (rc={rc})")

    def on_disconnect(_client, _userdata, rc):
        print(f"MQTT disconnected (rc={rc})")

    c.on_connect = on_connect
    c.on_disconnect = on_disconnect
    return c


def mqtt_is_connected(c) -> bool:
    return bool(getattr(c, "is_connected", lambda: False)())


def try_connect_mqtt(c) -> bool:
    try:
        c.connect(MQTT_BROKER, MQTT_PORT, 60)
        c.loop_start()
        return True
    except Exception as e:
        print(f"MQTT not available: {e}")
        return False


def publish_discovery(c):
    """
    Publish Home Assistant MQTT discovery configs (retained).
    Creates entities automatically in HA.
    """
    device = {
        "identifiers": [CLIENT_ID],
        "name": CLIENT_ID,
        "manufacturer": "Custom",
        "model": "RPI Touch Nicla Serial Bridge",
    }

    def pub_sensor(obj_id, name, state_topic, unit=None, device_class=None):
        payload = {
            "name": name,
            "state_topic": state_topic,
            "unique_id": f"{CLIENT_ID}_{obj_id}",
            "device": device,
        }
        if unit is not None:
            payload["unit_of_measurement"] = unit
        if device_class is not None:
            payload["device_class"] = device_class

        config_topic = f"{DISCOVERY_PREFIX}/sensor/{CLIENT_ID}/{obj_id}/config"
        c.publish(config_topic, json.dumps(payload), retain=True)

    # --- ENV sensors ---
    pub_sensor("temperature", "Temperature", f"{BASE_TOPIC}/temperature", "°C", "temperature")
    pub_sensor("humidity",    "Humidity",    f"{BASE_TOPIC}/humidity",    "%",  "humidity")
    pub_sensor("pressure",    "Pressure",    f"{BASE_TOPIC}/pressure",    "hPa", "pressure")
    pub_sensor("iaq",         "IAQ",         f"{BASE_TOPIC}/iaq",         None, None)
    pub_sensor("co2_eq",      "CO2eq",       f"{BASE_TOPIC}/co2_eq",      "ppm", "carbon_dioxide")
    pub_sensor("gas",         "Gas",         f"{BASE_TOPIC}/gas",         None, None)
    pub_sensor("accuracy",    "Accuracy",    f"{BASE_TOPIC}/accuracy",    None, None)

    # --- ACC sensors (generic; unit depends on what Nicla sends: m/s^2 or g) ---
    pub_sensor("acc_xAve", "Acc X Ave", f"{BASE_TOPIC}/acc_xAve", None, None)
    pub_sensor("acc_yAve", "Acc Y Ave", f"{BASE_TOPIC}/acc_yAve", None, None)
    pub_sensor("acc_zAve", "Acc Z Ave", f"{BASE_TOPIC}/acc_zAve", None, None)
    pub_sensor("acc_xMin", "Acc X Min", f"{BASE_TOPIC}/acc_xMin", None, None)
    pub_sensor("acc_yMin", "Acc Y Min", f"{BASE_TOPIC}/acc_yMin", None, None)
    pub_sensor("acc_zMin", "Acc Z Min", f"{BASE_TOPIC}/acc_zMin", None, None)
    pub_sensor("acc_xMax", "Acc X Max", f"{BASE_TOPIC}/acc_xMax", None, None)
    pub_sensor("acc_yMax", "Acc Y Max", f"{BASE_TOPIC}/acc_yMax", None, None)
    pub_sensor("acc_zMax", "Acc Z Max", f"{BASE_TOPIC}/acc_zMax", None, None)
    pub_sensor("acc_counts", "Acc Counts", f"{BASE_TOPIC}/acc_counts", None, None)

    print("Published HA discovery configs (retained)")


def parse_serial_line(line: str):
    """
    Input examples:
      acc,xAve,yAve,zAve,xMin,yMin,zMin,xMax,yMax,zMax,counts
      env,tmp,gas,pressure,iaq,co2_eq,humid,accuracy
    Returns dict of topics->values (flat), or None if unrecognized.
    """
    parts = [p.strip() for p in line.split(",")]
    if not parts:
        return None

    kind = parts[0].lower()

    if kind == "env" and len(parts) >= 8:
        return {
            f"{BASE_TOPIC}/temperature": float(parts[1]),
            f"{BASE_TOPIC}/gas":         float(parts[2]),
            f"{BASE_TOPIC}/pressure":    float(parts[3]),
            f"{BASE_TOPIC}/iaq":         float(parts[4]),
            f"{BASE_TOPIC}/co2_eq":      float(parts[5]),
            f"{BASE_TOPIC}/humidity":    float(parts[6]),
            f"{BASE_TOPIC}/accuracy":    float(parts[7]),
        }

    if kind == "acc" and len(parts) >= 11:
        return {
            f"{BASE_TOPIC}/acc_xAve":   float(parts[1]),
            f"{BASE_TOPIC}/acc_yAve":   float(parts[2]),
            f"{BASE_TOPIC}/acc_zAve":   float(parts[3]),
            f"{BASE_TOPIC}/acc_xMin":   float(parts[4]),
            f"{BASE_TOPIC}/acc_yMin":   float(parts[5]),
            f"{BASE_TOPIC}/acc_zMin":   float(parts[6]),
            f"{BASE_TOPIC}/acc_xMax":   float(parts[7]),
            f"{BASE_TOPIC}/acc_yMax":   float(parts[8]),
            f"{BASE_TOPIC}/acc_zMax":   float(parts[9]),
            f"{BASE_TOPIC}/acc_counts": float(parts[10]),
        }

    return None


if __name__ == "__main__":
    ser = open_serial()
    print(f"Reading serial on {SERIAL_PORT} @ {SERIAL_BAUD}")

    c = None
    last_mqtt_attempt = 0.0
    discovery_sent = False

    if MQTT_ENABLED:
        c = build_mqtt_client()
        try_connect_mqtt(c)
        last_mqtt_attempt = time.time()

    while True:
        # Read serial (always)
        try:
            raw = ser.readline()
            if raw:
                line = raw.decode(errors="ignore").strip()
                if line:
                    topics = parse_serial_line(line)

                    # Always show data
                    print(f"{utc_now_iso()}  {line}")
                    if topics:
                        print("Parsed:", topics)

                    # MQTT publish if available
                    if MQTT_ENABLED and c and mqtt_is_connected(c):
                        if not discovery_sent:
                            publish_discovery(c)
                            discovery_sent = True

                        if topics:
                            for t, v in topics.items():
                                c.publish(t, str(v), retain=False)

        except serial.SerialException as e:
            print(f"Serial error: {e}. Reopening in 2s...")
            time.sleep(2)
            try:
                ser.close()
            except Exception:
                pass
            ser = open_serial()

        except Exception as e:
            print(f"Error: {e}")

        # MQTT reconnect (non-blocking)
        if MQTT_ENABLED and c:
            now = time.time()
            if not mqtt_is_connected(c) and (now - last_mqtt_attempt) >= MQTT_RECONNECT_EVERY_SEC:
                last_mqtt_attempt = now
                try:
                    c.reconnect()
                except Exception:
                    pass
