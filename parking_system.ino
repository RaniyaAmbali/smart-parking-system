// Smart Parking Availability System
// Reads occupancy for two parking slots using a single ultrasonic sensor
// mounted on a servo, logs RFID card taps, shows live status on an LCD,
// and sends everything out over serial for a Python script to pick up.

#include <SPI.h>              // needed for the RFID reader (uses SPI communication)
#include <MFRC522.h>          // RFID reader library
#include <Servo.h>            // controls the servo motor
#include <Wire.h>             // needed for I2C communication (used by the LCD)
#include <LiquidCrystal_I2C.h> // LCD library (I2C version, only needs 2 data pins)

// --- Pin setup ---
// Named constants instead of raw numbers, so the wiring is easy to read/change later
#define SS_PIN 10       // RFID SDA/SS pin
#define RST_PIN 9       // RFID reset pin
#define TRIG_PIN 6      // ultrasonic sensor trigger pin
#define ECHO_PIN 7      // ultrasonic sensor echo pin
#define SERVO_PIN 5     // servo signal pin

// Distance (in cm) below which a slot is considered occupied.
// This was found by testing, not calculated - it depends on the sensor's
// mounting height and how far the sensor sits from each slot.
#define OCCUPIED_THRESHOLD 10

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x27 is this LCD's I2C address 

// Sends a short ultrasonic pulse out and times how long the echo takes to return.
// Distance = (time * speed of sound) / 2, the /2 because the sound travels there AND back.
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// Moves the servo to a given angle, then detaches it.
//
// Why detach? Originally the servo stayed attached permanently, which meant it kept
// sending a constant signal pulse even while sitting still. That constant pulse was
// interfering with the LCD's I2C data line and corrupting the display output.
// Attaching only right before a move, and detaching right after, means the servo
// only sends a signal when it's actually supposed to be moving - this fixed the
// LCD corruption without needing any wiring changes.
void moveServoTo(int angle) {
  myServo.attach(SERVO_PIN);
  myServo.write(angle);
  delay(500);   // give the servo time to physically reach the angle before reading anything
  myServo.detach();
  delay(100);   
}

void setup() {
  Serial.begin(9600);   
  SPI.begin();
  rfid.PCD_Init();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Parking System");
  delay(1500);
  lcd.clear();
}

void loop() {
  // --- Check Slot 1 ---
  // Only one ultrasonic sensor is available, so instead of two fixed sensors,
  // this one sensor is mounted on the servo and physically swivels to look at
  // each slot in turn. 90 degrees was found (by testing) to
  // point at Slot 1.
  moveServoTo(90);
  long dist1 = readDistance();
  bool slot1Occupied = (dist1 > 0 && dist1 < OCCUPIED_THRESHOLD);
  delay(200);

  moveServoTo(135);  // return to a center/neutral position between readings

  // --- Check Slot 2 ---
  // 150 degrees points at Slot 2 - again, found by testing the physical setup.
  moveServoTo(150);
  long dist2 = readDistance();
  bool slot2Occupied = (dist2 > 0 && dist2 < OCCUPIED_THRESHOLD);
  delay(200);

  moveServoTo(135);  // back to center again before the next cycle

  // --- Update the LCD with the latest slot status ---
  lcd.setCursor(0, 0);
  lcd.print("S1:");
  lcd.print(slot1Occupied ? "FULL " : "FREE ");
  lcd.print("S2:");
  lcd.print(slot2Occupied ? "FULL " : "FREE ");

  // --- Check for an RFID card tap ---
  String uidStr = "NONE";
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Build the card's unique ID as a hex string, byte by byte
    uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidStr += String(rfid.uid.uidByte[i], HEX);
    }
    lcd.setCursor(0, 1);
    lcd.print("Card scanned!   ");
    rfid.PICC_HaltA();  // stop reading this card until it's removed and tapped again
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Tap card to log ");
  }

  // --- Send everything to the Python script over serial ---
  // Format: slot1state,slot2state,cardUID
  // Slot states are sent as 1 (occupied) or 0 (free) since that's easiest
  // for both Python and ThingSpeak to work with directly.
  Serial.print(slot1Occupied ? 1 : 0);
  Serial.print(",");
  Serial.print(slot2Occupied ? 1 : 0);
  Serial.print(",");
  Serial.println(uidStr);

  delay(2000);  
}
