import serial
import requests
import time

# Open the serial connection to the Arduino.
# COM7 is specific to my machine

arduino = serial.Serial('COM7', 9600, timeout=1)

API_KEY = 'YOUR_API_KEY_HERE'  # from your ThingSpeak channel's API Keys tab - not sharing my real key publicly
URL = 'https://api.thingspeak.com/update'

time.sleep(2)  #serial connection settle before reading anything

while True:
    # Read one line sent by the Arduino
    line = arduino.readline().decode('utf-8').strip()

    if line and ',' in line:
        try:
            # Split into slot1 status, slot2 status, and the card UID
            parts = line.split(',')
            slot1 = int(parts[0])
            slot2 = int(parts[1])
            uid = parts[2]

            # Send all three values to ThingSpeak in a single request
            r = requests.get(URL, params={
                'api_key': API_KEY,
                'field1': slot1,
                'field2': slot2,
                'field3': uid
            })

            print(f"Sent slot1={slot1}, slot2={slot2}, uid={uid} -> response code {r.status_code}")

        except (ValueError, IndexError):
            # Ignore incomplete/corrupted lines instead of crashing
            pass

    # ThingSpeak's free tier requires at least 15 seconds between updates.
    # Waiting 20 gives a small safety buffer so requests don't get rejected.
    time.sleep(20)
