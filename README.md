# Smart Parking Availability System
<img width="1080" height="1440" alt="full system" src="https://github.com/user-attachments/assets/32562aee-cd32-484f-8799-2ea495478764" />



This is a small prototype I built over the summer using the Arduino IoT starter kit provided by my university. It's a two-slot parking system that checks occupancy with a swiveling ultrasonic sensor, logs card access with an RFID reader, shows live status on an LCD screen and sends everything to a ThingSpeak dashboard so it can be checked remotely too.

 This is my first real hands-on embedded systems project, built and debugged on my own.

The full code lives in [`parking_system.ino`](./parking_system.ino) and [`parking_to_thingspeak.py`](./parking_to_thingspeak.py) in this repo. This README explains what it's doing and why, rather than repeating the whole thing here.

## Why I built this

Circling a parking lot looking for a spot is a small daily annoyance, but it adds up — wasted fuel, wasted time, more traffic. I wanted to see if I could build something that actually solves a small piece of that, using just the components in a basic starter kit.

## What you'll need

<img width="734" height="398" alt="block diagram" src="https://github.com/user-attachments/assets/68fc6b2a-23b1-4e92-9c0a-dbe0c3fc6369" />


- Arduino Uno
- HC-SR04 ultrasonic sensor
- SG90 servo motor
- MFRC522 RFID reader + card
- 16x2 I2C LCD
- Something to mount it on (I used a foam board, roughly 20cm x 30cm)

## Wiring

| Component | Arduino pin |
|---|---|
| RFID SDA | 10 |
| RFID SCK | 13 |
| RFID MOSI | 11 |
| RFID MISO | 12 |
| RFID RST | 9 |
| RFID 3.3V | 3.3V |
| Servo signal | 5 |
| Ultrasonic Trig | 6 |
| Ultrasonic Echo | 7 |
| LCD SDA | A4 |
| LCD SCL | A5 |
| Servo / Ultrasonic / LCD VCC | 5V |
| Everything's GND | GND |

One thing worth flagging — the RFID module runs on 3.3V, not 5V. Be careful with it or else it could damage your RFID module.
Libraries needed (installable through the Arduino Library Manager): `MFRC522`, `Servo`, `Wire`, `LiquidCrystal_I2C`.

## How the Arduino code works

**Measuring distance.** 
The `readDistance()` function sends a short pulse out of the ultrasonic sensor's Trig pin, then measures how long it takes for the echo to come back on the Echo pin. Sound travels at a known, fairly constant speed, so that time delay converts directly into a distance in centimeters. I only had one ultrasonic sensor, so this same function gets reused for both slots — the trick is in how the sensor gets pointed at each one.

**Moving the sensor between slots.** 
Instead of buying a second sensor, I mounted the one I had on a servo motor and had it physically swivel between two angles — one aimed at each slot. That's what `moveServoTo()` does. It's slightly more involved than a plain `servo.write()` though: I originally had the servo attached the whole time, but that caused a separate bug (explained below), so this function now attaches the servo right before moving, waits for it to get there, then detaches it again. Only powering it when it actually needs to move turned out to matter a lot.

**The main loop.** Each cycle, the code:
1. Swivels to slot 1's angle, reads the distance, and decides "occupied" if something's closer than a set threshold
2. Returns to a center position
3. Swivels to slot 2's angle, does the same reading
4. Returns to center again
5. Updates the LCD with both slot statuses
6. Checks whether an RFID card is present, and if so, reads its unique ID
7. Prints one line over serial with everything: slot 1 state, slot 2 state, and the card ID (or "NONE" if nothing was tapped)

That serial line is the handoff point to the Python side.

**A quick note on the values:**
on the ThingSpeak dashboard (and in the serial output), slot status is sent as a plain number rather than text — `1` means the slot is occupied/full, and `0` means it's free. This is just because ThingSpeak fields work best with numeric data; the LCD screen still shows it in plain English (FREE / FULL) for anyone checking on-site.

## How the Python script works

The script opens a connection to the Arduino over USB using `pyserial`, and just sits in a loop reading whatever line comes in. Each line looks something like `1,0,53abab2c` — slot 1 occupied, slot 2 free, and a card UID. The script splits that on the commas, and uses the `requests` library to send those three values to ThingSpeak's API as a simple web request, one per field.

The 20-second wait between each send isn't arbitrary — ThingSpeak's free tier won't accept updates faster than every 15 seconds, so I gave it a small buffer to avoid getting rejected.

## Problems I ran into

This was genuinely the hardest part of the project, and probably the most valuable.

**Getting the servo angles right.** 
I tried calculating them with basic trigonometry first, based on measured distances, but once everything was actually mounted, the numbers didn't match reality. I ended up just testing angles by hand — writing different values, watching where the sensor pointed and adjusting until it lined up with each slot.

**COM port conflicts.**
Arduino IDE and my Python script can't both use the same USB port at once. I kept getting "Access is denied" errors until I got in the habit of always closing one before opening the other.

**The LCD kept showing garbled characters.** 
This one took a while to track down. It only happened once the servo, RFID, and LCD were all running together — the LCD worked fine on its own. Turned out the servo was sending a constant signal even while sitting still and that was interfering with the LCD's data line. Fixed it by only "attaching" the servo right before it needs to move, and detaching it right after, which is why `moveServoTo()` looks the way it does above.

**RFID taps kept getting missed.** 
I was only checking for a card once per full sensing cycle, so if you tapped at the wrong moment, nothing happened. Fixed it by checking for a card several times throughout the cycle instead of just once.

## Limitations

Being honest about what this doesn't do yet:

- The cloud dashboard updates with a ~15-20 second delay, because of ThingSpeak's free tier limit
- It's built for two slots specifically — scaling it up would need a different approach
- The ultrasonic readings can shift a bit depending on the angle/surface of whatever it's detecting
- The RFID reader currently just logs who tapped in, it doesn't actually control a gate or lock
- Everything runs on one Arduino, so there's no backup if something fails

## What I'd add next to make it more efficient  

- Swap the laptop + Python bridge for an ESP32, so it doesn't need a computer connected to work
- Add an actual gate/lock so RFID access means something physically
- Get it working for more than two slots
---

