#include <Adafruit_CircuitPlayground.h>
#include <Servo.h>

int CENTER_1 = 90;
int CENTER_2 = 90;
int THROW = 25;
bool INVERT_1 = false;
bool INVERT_2 = true;

float LAUNCH_ACCEL = 25;      // m/s^2, about 2.5 g
unsigned long SECOND = 1000;

Servo servo1;
Servo servo2;

void setFins(int a, int b) {
  if (INVERT_1) a = -a;
  if (INVERT_2) b = -b;
  servo1.write(constrain(CENTER_1 + a, 0, 180));
  servo2.write(constrain(CENTER_2 + b, 0, 180));
}

// one back and forth sweep with both fins tilting together
void sweep() {
  for (int i = 0; i <= THROW; i++) { setFins(i, i); delay(10); }
  for (int i = THROW; i >= -THROW; i--) { setFins(i, i); delay(10); }
  for (int i = -THROW; i <= 0; i++) { setFins(i, i); delay(10); }
}

// fins go left, right, left, right
void zigzag() {
  for (int n = 0; n < 6; n++) {
    int d = (n % 2 == 0) ? THROW : -THROW;
    setFins(d, d);
    delay(400);
  }
  setFins(0, 0);
}

bool launched() {
  float x = CircuitPlayground.motionX();
  float y = CircuitPlayground.motionY();
  float z = CircuitPlayground.motionZ();
  float total = sqrt(x * x + y * y + z * z);
  return total > LAUNCH_ACCEL;
}

void setup() {
  CircuitPlayground.begin();
  servo1.attach(A1);
  servo2.attach(A2);
  setFins(0, 0);

  // left button starts it early, handy for testing on the bench
  while (!launched() && !CircuitPlayground.leftButton()) {
    delay(5);
  }

  unsigned long start = millis();
  zigzag();

  // 30 seconds in the air, then three sweeps 30 seconds apart
  for (int i = 0; i < 3; i++) {
    unsigned long wait = start + (i + 1) * 30 * SECOND;
    while (millis() < wait) delay(10);
    sweep();
  }
}

void loop() {
}
