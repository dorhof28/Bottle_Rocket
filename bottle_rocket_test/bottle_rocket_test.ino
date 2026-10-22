#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

// open the serial monitor (115200) to see the results
// servo 1 is on A3, servo 2 is on A2

Servo servo1;
Servo servo2;
BLEUart bleuart;
int gyroAddr = 0x6B;

void writeReg(int reg, int val) {
  Wire.beginTransmission(gyroAddr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// returns the who am i value, 0 if nothing answered
int checkAddress(int addr) {
  gyroAddr = addr;
  Wire.beginTransmission(gyroAddr);
  Wire.write(0x0F);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom(gyroAddr, 1);
  if (!Wire.available()) return 0;
  return Wire.read();
}

float readAxis(int lowReg) {
  Wire.beginTransmission(gyroAddr);
  Wire.write(lowReg);
  Wire.endTransmission(false);
  Wire.requestFrom(gyroAddr, 2);
  int low = Wire.read();
  int high = Wire.read();
  int raw = high * 256 + low;
  if (raw > 32767) raw = raw - 65536;
  return raw * 0.0175;
}

void testGyro() {
  Serial.println("--- gyro ---");
  Wire.begin();
  int id = checkAddress(0x6B);
  if (id == 0) id = checkAddress(0x6A);

  if (id == 0) {
    Serial.println("FAIL: nothing answered at 0x6B or 0x6A");
    Serial.println("check VIN, GND, SCL, SDA wires");
    return;
  }
  Serial.print("something answered at 0x");
  Serial.println(gyroAddr, HEX);
  if (id != 0xD7) {
    Serial.print("FAIL: wrong chip id, got 0x");
    Serial.println(id, HEX);
    return;
  }
  Serial.println("PASS: L3GD20H found");

  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
  delay(100);

  Serial.println("tilt and spin it for 5 seconds, numbers should change");
  float biggest = 0;
  for (int i = 0; i < 25; i++) {
    float x = readAxis(0xA8);
    float y = readAxis(0xAA);
    float z = readAxis(0xAC);
    Serial.print("x ");
    Serial.print(x);
    Serial.print("  y ");
    Serial.print(y);
    Serial.print("  z ");
    Serial.println(z);
    if (abs(x) > biggest) biggest = abs(x);
    if (abs(y) > biggest) biggest = abs(y);
    if (abs(z) > biggest) biggest = abs(z);
    delay(200);
  }
  if (biggest > 20) {
    Serial.println("PASS: gyro readings move");
  } else {
    Serial.println("WARNING: readings barely changed, did you move it?");
  }
}

void sweepServo(Servo &s, String name) {
  Serial.print("watch ");
  Serial.println(name);
  s.write(90);
  delay(500);
  s.write(60);
  delay(700);
  s.write(120);
  delay(700);
  s.write(90);
  delay(500);
}

void testServos() {
  Serial.println("--- servos ---");
  servo1.attach(A3);
  servo2.attach(A2);
  sweepServo(servo1, "servo 1 (A3)");
  sweepServo(servo2, "servo 2 (A2)");
  Serial.println("servo that didn't move: check its power, ground and signal wire");
  Serial.println("if the board reset when they moved, they need a battery pack");
}

void testBluetooth() {
  Serial.println("--- bluetooth ---");
  Bluefruit.begin();
  Bluefruit.setName("BottleRocket");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.start(0);
  Serial.println("PASS: bluetooth started");
  Serial.println("connect with Bluefruit LE Connect now (30 seconds)");

  int waited = 0;
  while (!Bluefruit.connected() && waited < 300) {
    delay(100);
    waited++;
  }
  if (!Bluefruit.connected()) {
    Serial.println("FAIL: nothing connected");
    return;
  }
  Serial.println("PASS: phone connected");
  Serial.println("open Controller > Control Pad and press buttons (20 seconds)");

  int presses = 0;
  int counter = 0;
  while (Bluefruit.connected() && counter < 200) {
    if (bleuart.available() >= 5) {
      if (bleuart.peek() != '!') {
        bleuart.read();
      } else {
        int p[5];
        for (int i = 0; i < 5; i++) p[i] = bleuart.read();
        if (p[1] == 'B' && p[3] == '1') {
          Serial.print("got button ");
          Serial.println((char)p[2]);
          presses++;
        }
      }
    }
    delay(100);
    counter++;
  }
  if (presses > 0) {
    Serial.println("PASS: buttons come through");
  } else {
    Serial.println("FAIL: connected but no button presses seen");
  }
}

void setup() {
  Serial.begin(115200);
  int wait = 0;
  while (!Serial && wait < 500) {
    delay(20);
    wait++;
  }
  Serial.println("bottle rocket test");
  delay(500);

  testGyro();
  testServos();
  testBluetooth();
  Serial.println("--- done ---");
}

void loop() {
}
