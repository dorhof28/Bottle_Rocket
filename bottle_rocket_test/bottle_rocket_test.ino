#include <bluefruit.h>
#include <Servo.h>
#include <Wire.h>

// open the serial monitor (115200), type a letter and press enter
// servo 1 is on A3, servo 2 is on A2, gyro on SDA / SCL

int pin1 = A3;
int pin2 = A2;

Servo servo1;
Servo servo2;
BLEUart bleuart;

TwoWire *bus = &Wire;
int gyroAddr = 0x6B;
bool gyroOk = false;

void writeReg(int reg, int val) {
  bus->beginTransmission(gyroAddr);
  bus->write(reg);
  bus->write(val);
  bus->endTransmission();
}

// returns the who am i value, 0 if nothing answered
int checkAddress(int addr) {
  gyroAddr = addr;
  bus->beginTransmission(gyroAddr);
  bus->write(0x0F);
  if (bus->endTransmission(false) != 0) return 0;
  bus->requestFrom(gyroAddr, 1);
  if (!bus->available()) return 0;
  return bus->read();
}

float readAxis(int lowReg) {
  bus->beginTransmission(gyroAddr);
  bus->write(lowReg);
  bus->endTransmission(false);
  bus->requestFrom(gyroAddr, 2);
  int low = bus->read();
  int high = bus->read();
  int raw = high * 256 + low;
  if (raw > 32767) raw = raw - 65536;
  return raw * 0.0175;
}

// lists every i2c address that answers on one bus
void scanBus(TwoWire *b, String name) {
  Serial.print("scanning ");
  Serial.println(name);
  int found = 0;
  for (int addr = 1; addr < 127; addr++) {
    b->beginTransmission(addr);
    if (b->endTransmission() == 0) {
      Serial.print("  found 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println("  nothing found");
}

void scanAll() {
  scanBus(&Wire, "Wire");
#if WIRE_INTERFACES_COUNT > 1
  scanBus(&Wire1, "Wire1");
#endif
  Serial.println("the L3GD20H should show up as 0x6A or 0x6B");
  Serial.println("if the list is empty, the gyro has no power or the SDA/SCL wires are off");
}

bool lookOnBus(TwoWire *b) {
  bus = b;
  int id = checkAddress(0x6B);
  if (id == 0) id = checkAddress(0x6A);
  if (id == 0) return false;
  Serial.print("something answered at 0x");
  Serial.print(gyroAddr, HEX);
  Serial.print(", chip id 0x");
  Serial.println(id, HEX);
  return id == 0xD7;
}

void startGyro() {
  Wire.begin();
#if WIRE_INTERFACES_COUNT > 1
  Wire1.begin();
#endif

  if (lookOnBus(&Wire)) {
    Serial.println("gyro PASS: found on Wire");
    gyroOk = true;
  }
#if WIRE_INTERFACES_COUNT > 1
  else if (lookOnBus(&Wire1)) {
    Serial.println("gyro PASS: found on Wire1");
    gyroOk = true;
  }
#endif

  if (!gyroOk) {
    Serial.println("gyro FAIL: no L3GD20H found, type i to scan the bus");
    return;
  }
  writeReg(0x20, 0x0F);
  writeReg(0x23, 0x10);
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
  delay(600);
  s.write(30);
  delay(1000);
  s.write(150);
  delay(1000);
  s.write(90);
  delay(600);
  Serial.println("done, did it move?");
}

// wiggles a servo on every pad, so you can see which pad it is really on
void tryAllPads() {
  int pads[7] = {A1, A2, A3, A4, A5, A6, A7};
  Serial.println("watch the servo, I will say which pad I am on");
  for (int i = 0; i < 7; i++) {
    Serial.print("pad A");
    Serial.println(i + 1);
    Servo temp;
    temp.attach(pads[i]);
    temp.write(90);
    delay(500);
    temp.write(40);
    delay(800);
    temp.write(140);
    delay(800);
    temp.write(90);
    delay(300);
    temp.detach();
  }
  Serial.println("done. if it never moved, it is a power or ground problem");
}


// sends servo pulses by hand, no servo library
void rawTest() {
  Serial.println("raw pulses on both pins, no library");
  servo1.detach();
  servo2.detach();
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  int widths[4] = {1500, 1000, 2000, 1500};
  for (int i = 0; i < 4; i++) {
    for (int n = 0; n < 40; n++) {
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, HIGH);
      delayMicroseconds(widths[i]);
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
      delay(19);
    }
  }
  servo1.attach(pin1);
  servo2.attach(pin2);
  Serial.println("done. if this moves but 1 / 2 did not, tell me");
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
  Serial.println("1 = move servo 1 (A3)");
  Serial.println("2 = move servo 2 (A2)");
  Serial.println("3 = move both");
  Serial.println("a = try a servo on every pad");
  Serial.println("r = raw pulses, no servo library");
  Serial.println("i = scan i2c for the gyro");
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
  servo1.attach(pin1);
  servo2.attach(pin2);
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
    if (c == 'a') tryAllPads();
    if (c == 'r') rawTest();
    if (c == 'i') scanAll();
    if (c == 'g') showGyro();
    if (c == 'b') bluetoothStatus();
    if (c == 'h') showHelp();
  }
}
