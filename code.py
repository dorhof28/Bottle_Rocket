
import time
import board
import pwmio
from adafruit_motor import servo
from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService
from adafruit_bluefruit_connect.packet import Packet
from adafruit_bluefruit_connect.button_packet import ButtonPacket

# ---- Settings you may want to tweak ----
CENTER_1 = 90        # servo 1 angle when fin is straight (trim so fin looks flush)
CENTER_2 = 90        # servo 2 angle when fin is straight
THROW = 20           # degrees of fin deflection from center
INVERT_1 = False     # flip a servo if its fin tilts the wrong way
INVERT_2 = True      # fins face opposite ways, so servo 2 is usually mirrored.
                     # CALIBRATE: press UP - both fins' trailing edges must
                     # move toward the same side. If not, flip this.
LINK_LOST_CENTER = True   # center fins if the phone disconnects

# Servo signal wires: A1 and A2 pads. Power servos from a separate 4xAA pack
# (common ground with the board), NOT the board's 3.3V pad.
pwm1 = pwmio.PWMOut(board.A1, duty_cycle=0, frequency=50)
pwm2 = pwmio.PWMOut(board.A2, duty_cycle=0, frequency=50)
servo1 = servo.Servo(pwm1, min_pulse=500, max_pulse=2500)
servo2 = servo.Servo(pwm2, min_pulse=500, max_pulse=2500)


def clamp(a):
    return max(0, min(180, a))


def set_fins(roll, pitch):
    """roll, pitch in {-1, 0, 1}: roll = left/right, pitch = down/up."""
    a = max(-1, min(1, pitch + roll))   # fin A: collective + differential
    b = max(-1, min(1, pitch - roll))   # fin B: collective - differential
    a = -a if INVERT_1 else a
    b = -b if INVERT_2 else b
    servo1.angle = clamp(CENTER_1 + a * THROW)
    servo2.angle = clamp(CENTER_2 + b * THROW)


set_fins(0, 0)

ble = BLERadio()
uart = UARTService()
advert = ProvideServicesAdvertisement(uart)

x_cmd = 0
y_cmd = 0

while True:
    ble.start_advertising(advert)
    print("Waiting for iPhone... open Bluefruit LE Connect and connect.")
    while not ble.connected:
        pass
    ble.stop_advertising()
    print("Connected")

    while ble.connected:
        if uart.in_waiting:
            try:
                packet = Packet.from_stream(uart)
            except ValueError:
                continue
            if isinstance(packet, ButtonPacket):
                b, down = packet.button, packet.pressed
                if b == ButtonPacket.LEFT:
                    x_cmd = -1 if down else (0 if x_cmd == -1 else x_cmd)
                elif b == ButtonPacket.RIGHT:
                    x_cmd = 1 if down else (0 if x_cmd == 1 else x_cmd)
                elif b == ButtonPacket.DOWN:
                    y_cmd = -1 if down else (0 if y_cmd == -1 else y_cmd)
                elif b == ButtonPacket.UP:
                    y_cmd = 1 if down else (0 if y_cmd == 1 else y_cmd)
                elif down and b == ButtonPacket.BUTTON_1:
                    x_cmd = y_cmd = 0
                elif down and b == ButtonPacket.BUTTON_2:
                    THROW = 10
                elif down and b == ButtonPacket.BUTTON_3:
                    THROW = 20
                elif down and b == ButtonPacket.BUTTON_4:
                    THROW = 30
                set_fins(x_cmd, y_cmd)
        time.sleep(0.005)

    # Phone disconnected
    x_cmd = y_cmd = 0
    if LINK_LOST_CENTER:
        set_fins(0, 0)
