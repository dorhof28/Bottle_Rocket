#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

// settings
int center1 = 90;      // servo 1 straight
int center2 = 90;      // servo 2 straight
int throwAmt = 20;     // fin angle from center
bool flip1 = false;    // flip if fin goes the wrong way
bool flip2 = true;
float leanGain = 0.03;   // gyro: push per degree of lean
float rateGain = 0.008;  // gyro: push per degree/sec
float spinGain = 0.01;   // gyro: push against spin
float gyroSign = 1;      // -1 if the fins follow the lean instead of fighting it

Servo servo1;
Servo servo2;
BLEUart bleuart;

int gyroAddr = 0x6B;
float biasX = 0, biasZ = 0;
float lean = 0;
long lastRead = 0;

int rollCmd = 0;
int pitchCmd = 0;

// 0 = phone arrows, 1 = gyro, 2 = zigzag, 3 = push button
int mode = 0;
int pressCount = 0;
int zigStep = 0;
long zigTime = 0;

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
void writeReg(int reg, int val) {
  Wire.beginTransmission(gyroAddr);
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

void readGyro(float &x, float &z) {
  Wire.beginTransmission(gyroAddr);
  Wire.write(0xA8);
  Wire.endTransmission(false);
  Wire.requestFrom(gyroAddr, 6);
  int xl = Wire.read();
  int xh = Wire.read();
  Wire.read();
  Wire.read();
  int zl = Wire.read();
  int zh = Wire.read();
  int rawX = xh * 256 + xl;
  int rawZ = zh * 256 + zl;
  if (rawX > 32767) rawX = rawX - 65536;
  if (rawZ > 32767) rawZ = rawZ - 65536;
  x = rawX * 0.0175;
  z = rawZ * 0.0175;
}

void startGyro() {
  Wire.begin();
  if (!findGyro()) {
    while (1) delay(100);
  }
  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
  delay(100);

  // average readings while it sits still
  float sumX = 0, sumZ = 0, x, z;
  for (int i = 0; i < 200; i++) {
    readGyro(x, z);
    sumX += x;
    sumZ += z;
    delay(5);
  }
  biasX = sumX / 200;
  biasZ = sumZ / 200;
}

// fins fight the lean
void runGyro() {
  long now = millis();
  if (now - lastRead < 10) return;
  float dt = (now - lastRead) / 1000.0;
  lastRead = now;

  float rx, rz;
  readGyro(rx, rz);
  rx -= biasX;
  rz -= biasZ;
  lean += rx * dt;

  float push = (lean * leanGain + rx * rateGain) * gyroSign;
  float spin = rz * spinGain * gyroSign;
  setFins(-spin, -push);
}

// zigzag, swaps sides every 0.4 sec
void runZigzag() {
  if (millis() - zigTime < 400) return;
  zigTime = millis();
  float side = (zigStep % 2 == 0) ? 1 : -1;
  setFins(0, side);
  zigStep++;
}

// push button: left, center, right
void pushStep() {
  float spots[3] = {-1, 0, 1};
  setFins(0, spots[pressCount % 3]);
  pressCount++;
}

void changeMode(int newMode) {
  mode = newMode;
  rollCmd = pitchCmd = 0;
  lean = 0;
  lastRead = millis();
  zigStep = 0;
  pressCount = 0;
  setFins(0, 0);
}

// same button again turns the mode off
void toggleMode(int m, String name) {
  if (mode == m) {
    changeMode(0);
    bleuart.print(name);
    bleuart.println(" off");
  } else {
    changeMode(m);
    bleuart.print(name);
    bleuart.println(" on");
  }
}

// 1 gyro, 2 zigzag, 3 push button, 4 stop everything, arrows are 5-8
void handleButton(int b, bool down) {
  if (!down) {
    if (b == '7' && rollCmd == -1) rollCmd = 0;
    if (b == '8' && rollCmd == 1) rollCmd = 0;
    if (b == '6' && pitchCmd == -1) pitchCmd = 0;
    if (b == '5' && pitchCmd == 1) pitchCmd = 0;
    if (mode == 0) setFins(rollCmd, pitchCmd);
    return;
  }

  switch (b) {
    case '1': toggleMode(1, "gyro"); break;
    case '2': toggleMode(2, "zigzag"); break;
    case '3':
      if (mode != 3) changeMode(3);
      pushStep();
      break;
    case '4': changeMode(0); break;
    case '5': pitchCmd = 1; break;
    case '6': pitchCmd = -1; break;
    case '7': rollCmd = -1; break;
    case '8': rollCmd = 1; break;
  }
  if (mode == 0) setFins(rollCmd, pitchCmd);
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
  servo1.attach(A3);
  servo2.attach(A2);
  setFins(0, 0);
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
    handleButton(p[2], p[3] == '1');
  }

  if (mode == 1) runGyro();
  if (mode == 2) runZigzag();
}
