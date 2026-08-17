#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9
#define TRIG_PIN 6
#define ECHO_PIN 7
#define SERVO_PIN 5

#define OCCUPIED_THRESHOLD 10

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void moveServoTo(int angle) {
  myServo.attach(SERVO_PIN);
  myServo.write(angle);
  delay(500);
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
  moveServoTo(90);
  long dist1 = readDistance();
  bool slot1Occupied = (dist1 > 0 && dist1 < OCCUPIED_THRESHOLD);
  delay(200);

  moveServoTo(135);

  moveServoTo(150);
  long dist2 = readDistance();
  bool slot2Occupied = (dist2 > 0 && dist2 < OCCUPIED_THRESHOLD);
  delay(200);

  moveServoTo(135);

  lcd.setCursor(0, 0);
  lcd.print("S1:");
  lcd.print(slot1Occupied ? "FULL " : "FREE ");
  lcd.print("S2:");
  lcd.print(slot2Occupied ? "FULL " : "FREE ");

  String uidStr = "NONE";
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidStr += String(rfid.uid.uidByte[i], HEX);
    }
    lcd.setCursor(0, 1);
    lcd.print("Card scanned!   ");
    rfid.PICC_HaltA();
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Tap card to log ");
  }

  Serial.print(slot1Occupied ? 1 : 0);
  Serial.print(",");
  Serial.print(slot2Occupied ? 1 : 0);
  Serial.print(",");
  Serial.println(uidStr);

  delay(2000);
}
