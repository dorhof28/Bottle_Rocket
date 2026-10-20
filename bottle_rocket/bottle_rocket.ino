#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

int CENTER_1 = 90;        // servo 1 angle when fin is straight (trim so fin looks flush)
int CENTER_2 = 90;        // servo 2 angle when fin is straight
int THROW = 20;           // degrees of fin deflection from center
bool INVERT_1 = false;    // flip a servo if its fin tilts the wrong way
bool INVERT_2 = true;     // fins face opposite ways, so servo 2 is usually mirrored.
                          // CALIBRATE: press UP - both fins' trailing edges must
                          // move toward the same side. If not, flip this.
bool LINK_LOST_CENTER = true;   // center fins if the phone disconnects

// gyro settings (L3GD20H on the SDA / SCL pads)
float LEAN_GAIN = 0.03;   // fin push per degree of lean
float RATE_GAIN = 0.008;  // fin push per degree/sec
float SPIN_GAIN = 0.01;   // fin push against spinning
float STAB_SIGN = 1;      // set to -1 if the fins move the wrong way when you tilt it

// Servo signal wires: A1 and A2 pads. Power servos from a separate 4xAA pack
// (common ground with the board), NOT the board's 3.3V pad.
Servo servo1;
Servo servo2;
BLEUart bleuart;

int rollCmd = 0;
int pitchCmd = 0;
bool stabilize = false;

float leanAngle = 0;
float biasX = 0, biasZ = 0;
unsigned long lastRead = 0;

int GYRO_ADDR = 0x6B;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(GYRO_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool findGyro() {
  int tries[2] = {0x6B, 0x6A};
  for (int i = 0; i < 2; i++) {
    GYRO_ADDR = tries[i];
    Wire.beginTransmission(GYRO_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission(false);
    Wire.requestFrom(GYRO_ADDR, 1);
    if (Wire.available() && Wire.read() == 0xD7) return true;
  }
  return false;
}

float clampF(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

void setFins(float roll, float pitch) {
  float a = clampF(pitch + roll, -1, 1);
  float b = clampF(pitch - roll, -1, 1);
  if (INVERT_1) a = -a;
  if (INVERT_2) b = -b;
  servo1.write((int)clampF(CENTER_1 + a * THROW, 0, 180));
  servo2.write((int)clampF(CENTER_2 + b * THROW, 0, 180));
}

void readGyro(float &x, float &z) {
  Wire.beginTransmission(GYRO_ADDR);
  Wire.write(0xA8);
  Wire.endTransmission(false);
  Wire.requestFrom(GYRO_ADDR, 6);
  int xl = Wire.read();
  int xh = Wire.read();
  Wire.read();
  Wire.read();
  int zl = Wire.read();
  int zh = Wire.read();
  int16_t gx = (xh << 8) | xl;
  int16_t gz = (zh << 8) | zl;
  x = gx * 0.0175;
  z = gz * 0.0175;
}

void gyroSetup() {
  Wire.begin();
  if (!findGyro()) {
    while (1) delay(100);
  }
  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
  delay(100);

  // average a bunch of readings while it sits still
  float sx = 0, sz = 0, x, z;
  for (int i = 0; i < 200; i++) {
    readGyro(x, z);
    sx += x;
    sz += z;
    delay(5);
  }
  biasX = sx / 200;
  biasZ = sz / 200;
}

void updateStabilize() {
  unsigned long now = millis();
  if (now - lastRead < 10) return;
  float dt = (now - lastRead) / 1000.0;
  lastRead = now;

  float rx, rz;
  readGyro(rx, rz);
  rx -= biasX;
  rz -= biasZ;
  leanAngle += rx * dt;

  float lean = (leanAngle * LEAN_GAIN + rx * RATE_GAIN) * STAB_SIGN;
  float spin = rz * SPIN_GAIN * STAB_SIGN;
  setFins(rollCmd - spin, pitchCmd - lean);
}

void handleButton(char b, bool down) {
  switch (b) {
    case '7': rollCmd = down ? -1 : (rollCmd == -1 ? 0 : rollCmd); break;
    case '8': rollCmd = down ? 1 : (rollCmd == 1 ? 0 : rollCmd); break;
    case '6': pitchCmd = down ? -1 : (pitchCmd == -1 ? 0 : pitchCmd); break;
    case '5': pitchCmd = down ? 1 : (pitchCmd == 1 ? 0 : pitchCmd); break;
    case '1': if (down) rollCmd = pitchCmd = 0; break;
    case '2':
      if (down) {
        stabilize = !stabilize;
        leanAngle = 0;
        lastRead = millis();
      }
      break;
    case '3': if (down) THROW = (THROW >= 30) ? 10 : THROW + 10; break;
    case '4': if (down) leanAngle = 0; break;
  }
  if (!stabilize) setFins(rollCmd, pitchCmd);
}

void startAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason) {
  rollCmd = pitchCmd = 0;
  if (LINK_LOST_CENTER && !stabilize) setFins(0, 0);
}

void setup() {
  servo1.attach(A1);
  servo2.attach(A2);
  setFins(0, 0);
  gyroSetup();

  Bluefruit.begin();
  Bluefruit.setName("BottleRocket");
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);
  bleuart.begin();
  startAdv();
}

void loop() {
  while (bleuart.available() >= 5) {
    if (bleuart.peek() != '!') {
      bleuart.read();
      continue;
    }
    uint8_t p[5];
    for (int i = 0; i < 5; i++) p[i] = bleuart.read();
    if (p[1] != 'B') continue;
    uint8_t sum = 0;
    for (int i = 0; i < 4; i++) sum += p[i];
    if ((uint8_t)~sum != p[4]) continue;
    handleButton((char)p[2], p[3] == '1');
  }

  if (stabilize) updateStabilize();
}
