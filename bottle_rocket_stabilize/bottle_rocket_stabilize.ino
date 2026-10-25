#include <Servo.h>
#include <Wire.h>

int CENTER_1 = 72;        // servo 1 angle when fin is straight
int CENTER_2 = 45;        // servo 2 angle when fin is straight
int THROW = 20;           // degrees of fin deflection from center
bool INVERT_1 = false;
bool INVERT_2 = true;

// gyro settings (L3GD20H on the SDA / SCL pads)
float LEAN_GAIN = 0.03;   // fin push per degree of lean
float RATE_GAIN = 0.008;  // fin push per degree/sec
float SPIN_GAIN = 0.01;   // fin push against spinning
float STAB_SIGN = 1;      // set to -1 if the fins move the wrong way when you tilt it

Servo servo1;
Servo servo2;

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

void setup() {
  servo1.attach(A6);
  servo2.attach(A7);
  setFins(0, 0);

  Wire.begin();
  if (!findGyro()) {
    while (1) delay(100);
  }
  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
  delay(100);

  // keep the rocket still for a second or two here
  float sx = 0, sz = 0, x, z;
  for (int i = 0; i < 200; i++) {
    readGyro(x, z);
    sx += x;
    sz += z;
    delay(5);
  }
  biasX = sx / 200;
  biasZ = sz / 200;
  lastRead = millis();
}

void loop() {
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
  setFins(-spin, -lean);
}
