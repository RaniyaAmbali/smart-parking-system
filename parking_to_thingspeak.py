import serial
import requests
import time

arduino = serial.Serial('COM7', 9600, timeout=1)  # change to your port
API_KEY = 'YOUR_THINGSPEAK_WRITE_API_KEY'
URL = 'https://api.thingspeak.com/update'

time.sleep(2)

while True:
    line = arduino.readline().decode('utf-8').strip()
    if line and ',' in line:
        try:
            parts = line.split(',')
            slot1 = int(parts[0])
            slot2 = int(parts[1])
            uid = parts[2]

            r = requests.get(URL, params={
                'api_key': API_KEY,
                'field1': slot1,
                'field2': slot2,
                'field3': uid
            })
            print(f"Sent slot1={slot1}, slot2={slot2}, uid={uid} -> response code {r.status_code}")
        except (ValueError, IndexError):
            pass
    time.sleep(20)
