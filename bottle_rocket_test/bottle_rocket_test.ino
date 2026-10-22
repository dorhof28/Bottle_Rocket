#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

// open the serial monitor (115200) and type a letter then enter
// servo 1 is on A3, servo 2 is on A2

Servo servo1;
Servo servo2;
BLEUart bleuart;
int gyroAddr = 0x6B;
bool gyroOk = false;

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

void startGyro() {
  Wire.begin();
  int id = checkAddress(0x6B);
  if (id == 0) id = checkAddress(0x6A);

  if (id == 0) {
    Serial.println("gyro FAIL: nothing answered at 0x6B or 0x6A");
    Serial.println("check VIN, GND, SCL, SDA wires");
    return;
  }
  if (id != 0xD7) {
    Serial.print("gyro FAIL: wrong chip id, got 0x");
    Serial.println(id, HEX);
    return;
  }
  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
  gyroOk = true;
  Serial.print("gyro PASS: found at 0x");
  Serial.println(gyroAddr, HEX);
}

// prints gyro numbers until you type something
void showGyro() {
  if (!gyroOk) {
    Serial.println("gyro was not found, nothing to read");
    return;
  }
  Serial.println("tilt it, type anything to stop");
  while (!Serial.available()) {
    Serial.print("x ");
    Serial.print(readAxis(0xA8));
    Serial.print("  y ");
    Serial.print(readAxis(0xAA));
    Serial.print("  z ");
    Serial.println(readAxis(0xAC));
    delay(300);
  }
  while (Serial.available()) Serial.read();
}

void moveServo(Servo &s, String name) {
  Serial.print("moving ");
  Serial.println(name);
  s.write(90);
  delay(500);
  s.write(60);
  delay(700);
  s.write(120);
  delay(700);
  s.write(90);
  delay(500);
  Serial.println("done, did it move?");
}

void startBluetooth() {
  Bluefruit.begin();
  Bluefruit.setName("BottleRocket");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
  Serial.println("bluetooth PASS: started, look for BottleRocket in the app");
}

void bluetoothStatus() {
  if (Bluefruit.connected()) {
    Serial.println("phone is connected, press buttons in the Control Pad");
  } else {
    Serial.println("no phone connected yet");
  }
}

// prints any buttons coming from the phone
void checkPhone() {
  while (bleuart.available() >= 5) {
    if (bleuart.peek() != '!') {
      bleuart.read();
      continue;
    }
    int p[5];
    for (int i = 0; i < 5; i++) p[i] = bleuart.read();
    if (p[1] == 'B' && p[3] == '1') {
      Serial.print("phone button ");
      Serial.println((char)p[2]);
    }
  }
}

void showHelp() {
  Serial.println("1 = move servo 1");
  Serial.println("2 = move servo 2");
  Serial.println("3 = move both");
  Serial.println("g = gyro numbers");
  Serial.println("b = bluetooth status");
  Serial.println("h = this list");
}

void setup() {
  Serial.begin(115200);
  int wait = 0;
  while (!Serial && wait < 500) {
    delay(20);
    wait++;
  }
  Serial.println("bottle rocket test");
  servo1.attach(A3);
  servo2.attach(A2);
  servo1.write(90);
  servo2.write(90);
  startGyro();
  startBluetooth();
  showHelp();
}

void loop() {
  checkPhone();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') moveServo(servo1, "servo 1 (A3)");
    if (c == '2') moveServo(servo2, "servo 2 (A2)");
    if (c == '3') {
      moveServo(servo1, "servo 1 (A3)");
      moveServo(servo2, "servo 2 (A2)");
    }
    if (c == 'g') showGyro();
    if (c == 'b') bluetoothStatus();
    if (c == 'h') showHelp();
  }
}
