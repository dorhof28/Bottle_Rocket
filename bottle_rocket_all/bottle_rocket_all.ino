#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

// pins
const int servo1Pin = A6;   // RX pad
const int servo2Pin = A7;   // TX pad

// fin settings
int center1 = 72;      // servo 1 straight
int center2 = 45;      // servo 2 straight
int throwAmt = 20;     // fin angle from center
bool flip1 = false;    // flip if a fin goes the wrong way
bool flip2 = true;

// gyro settings
int tiltLimit = 5;       // degrees of lean before the fins move
int gyroMove = 15;       // how far the fins move when it leans
float noise = 1;         // turning slower than this (degrees/sec) is ignored
int gyroDirection = 1;   // -1 if the fins go the same way as the lean

// zigzag settings
long startDelay = 3000;       // 3 seconds after pressing the button
long stepTime = 30000;        // 30 seconds between moves
float zigSides[4] = {-1, 1, -1, 0};   // left, right, left, center

// modes
const int MANUAL = 0;
const int GYRO = 1;
const int ZIGZAG = 2;
int mode = MANUAL;

Servo servo1;
Servo servo2;
BLEUart bleuart;

// gyro
int gyroAddr = 0x6B;
bool gyroOk = false;
float bias = 0;
float lean = 0;
long lastRead = 0;

// zigzag state
long zigStart = 0;
int zigStep = -1;

// push button state
int pressCount = 0;

float limit(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// move both fins
void setFins(float roll, float pitch) {
  float a = limit(pitch + roll, -1, 1);
  float b = limit(pitch - roll, -1, 1);
  if (flip1) a = -a;
  if (flip2) b = -b;
  int angle1 = center1 + a * throwAmt;
  int angle2 = center2 + b * throwAmt;
  servo1.write(limit(angle1, 0, 180));
  servo2.write(limit(angle2, 0, 180));
}

// gyro stuff
void writeReg(int addr, int reg, int val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool findGyro() {
  int addrs[2] = {0x6B, 0x6A};
  for (int i = 0; i < 2; i++) {
    gyroAddr = addrs[i];
    Wire.beginTransmission(gyroAddr);
    Wire.write(0x0F);
    Wire.endTransmission(false);
    Wire.requestFrom(gyroAddr, 1);
    if (Wire.available() && Wire.read() == 0xD7) return true;
  }
  return false;
}

// turning speed around the x axis in degrees per second
float readGyro() {
  Wire.beginTransmission(gyroAddr);
  Wire.write(0xA8);
  Wire.endTransmission(false);
  Wire.requestFrom(gyroAddr, 2);
  int low = Wire.read();
  int high = Wire.read();
  int raw = high * 256 + low;
  if (raw > 32767) raw = raw - 65536;
  return raw * 0.0175;
}

// average a bunch of readings while it sits still, that is the gyro's zero
void calibrateGyro() {
  float total = 0;
  for (int i = 0; i < 200; i++) {
    total += readGyro();
    delay(5);
  }
  bias = total / 200;
}

void startGyro() {
  gyroOk = findGyro();
  if (!gyroOk) return;
  writeReg(gyroAddr, 0x20, 0x0F);
  writeReg(gyroAddr, 0x23, 0x10);
  delay(100);
  calibrateGyro();
}

// keeps track of the lean, fins go the other way once it leans enough
void runGyro() {
  if (!gyroOk) return;
  long now = millis();
  if (now - lastRead < 10) return;
  float dt = (now - lastRead) / 1000.0;
  lastRead = now;

  float rate = readGyro() - bias;
  if (abs(rate) > noise) {
    lean = lean + rate * dt;
  }

  float amount = (float)gyroMove / throwAmt * gyroDirection;
  if (lean > tiltLimit) {
    setFins(0, -amount);
  } else if (lean < -tiltLimit) {
    setFins(0, amount);
  } else {
    setFins(0, 0);
  }
}

// zigzag: starts 3 sec after the button, then left, right, left, center, 30 sec apart
void runZigzag() {
  long since = millis() - zigStart;
  if (since < 0) return;

  int step = since / stepTime;
  if (step > 3) step = 3;
  if (step == zigStep) return;

  zigStep = step;
  setFins(0, zigSides[step]);
  bleuart.print("zigzag move ");
  bleuart.println(step + 1);
}

// push button: left, center, right
void pushStep() {
  float spots[3] = {-1, 0, 1};
  setFins(0, spots[pressCount % 3]);
  pressCount++;
}

void changeMode(int newMode) {
  mode = newMode;
  lean = 0;
  lastRead = millis();
  zigStep = -1;
  pressCount = 0;
  setFins(0, 0);
}

// pressing the same mode button again goes back to manual
void toggleMode(int newMode, String name) {
  if (mode == newMode) {
    changeMode(MANUAL);
    bleuart.print(name);
    bleuart.println(" off");
    return;
  }
  if (newMode == GYRO) {
    if (!gyroOk) {
      bleuart.println("gyro not found");
      return;
    }
    bleuart.println("hold it still...");
    calibrateGyro();
  }
  changeMode(newMode);
  bleuart.print(name);
  bleuart.println(" on");
  if (newMode == ZIGZAG) {
    zigStart = millis() + startDelay;
    bleuart.println("starts in 3 seconds");
  }
}

// arrows move the fins by hand
void handleArrow(int b) {
  if (mode != MANUAL) return;

  pressCount = 0;
  if (b == '7') setFins(0, -1);
  if (b == '8') setFins(0, 1);
  if (b == '5' || b == '6') setFins(0, 0);
}

// 1 gyro, 2 zigzag, 3 manual, 4 push button, arrows 5 up, 6 down, 7 left, 8 right
void handleButton(int b) {
  if (b == '1') toggleMode(GYRO, "gyro");
  if (b == '2') toggleMode(ZIGZAG, "zigzag");
  if (b == '3') {
    changeMode(MANUAL);
    bleuart.println("manual");
  }
  if (b == '4') {
    if (mode != MANUAL) changeMode(MANUAL);
    pushStep();
  }
  if (b >= '5' && b <= '8') handleArrow(b);
}

void startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void setup() {
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  setFins(0, 0);

  Wire.begin();
  startGyro();

  Bluefruit.begin();
  Bluefruit.setName("BottleRocket");
  bleuart.begin();
  startAdvertising();
}

void loop() {
  // read button packets from the app
  while (bleuart.available() >= 5) {
    if (bleuart.peek() != '!') {
      bleuart.read();
      continue;
    }
    int p[5];
    for (int i = 0; i < 5; i++) p[i] = bleuart.read();
    if (p[1] != 'B') continue;
    int sum = 0;
    for (int i = 0; i < 4; i++) sum = sum + p[i];
    if (255 - (sum % 256) != p[4]) continue;
    if (p[3] == '1') handleButton(p[2]);
  }

  if (mode == GYRO) runGyro();
  if (mode == ZIGZAG) runZigzag();
}
