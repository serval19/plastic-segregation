/*
 * Plastic Segregation Robotic Arm
 * 6-DOF Arm controlled via Serial from Laptop
 * BINS ARRANGED IN SEMI-CIRCLE (0°–180° SAFE)
 */

#include <Servo.h>

// Create servo objects
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo wristRotServo;
Servo wristServo;
Servo gripperServo;

// Current servo positions
int basePos = 90;
int shoulderPos = 90;
int elbowPos = 90;
int wristRotPos = 90;
int wristPos = 90;
int gripperPos = 90;

const int MOVE_SPEED = 25;
const int GRIPPER_SPEED = 20;

bool armBusy = false;
int currentPlasticType = 0;
unsigned long lastActivityTime = 0;
bool debugMode = true;

// ================= HOME POSITION =================
const int HOME_BASE = 90;
const int HOME_SHOULDER = 30;
const int HOME_ELBOW = 120;
const int HOME_WRIST_ROT = 90;
const int HOME_WRIST = 90;
const int HOME_GRIPPER = 100;

// ================= PICKUP POSITION =================
const int PICKUP_BASE = 90;
const int PICKUP_SHOULDER = 70;
const int PICKUP_ELBOW = 60;
const int PICKUP_WRIST_ROT = 90;
const int PICKUP_WRIST = 80;
const int PICKUP_GRIPPER_OPEN = 130;
const int PICKUP_GRIPPER_CLOSED = 40;

const int APPROACH_SHOULDER = 50;
const int APPROACH_ELBOW = 70;

// ================= SEMI-CIRCLE BIN POSITIONS =================
// All base angles between 30° and 150°

const int BIN1_BASE = 30;   // Front-Right
const int BIN1_SHOULDER = 40;
const int BIN1_ELBOW = 80;
const int BIN1_WRIST_ROT = 90;
const int BIN1_WRIST = 90;

const int BIN2_BASE = 70;   // Right
const int BIN2_SHOULDER = 40;
const int BIN2_ELBOW = 80;
const int BIN2_WRIST_ROT = 90;
const int BIN2_WRIST = 90;

const int BIN3_BASE = 110;  // Left
const int BIN3_SHOULDER = 40;
const int BIN3_ELBOW = 80;
const int BIN3_WRIST_ROT = 90;
const int BIN3_WRIST = 90;

const int BIN4_BASE = 150;  // Front-Left
const int BIN4_SHOULDER = 40;
const int BIN4_ELBOW = 80;
const int BIN4_WRIST_ROT = 90;
const int BIN4_WRIST = 90;

// Common drop position
const int DROP_SHOULDER = 45;
const int DROP_ELBOW = 75;
const int DROP_WRIST_ROT = 90;
const int DROP_WRIST = 90;
const int DROP_GRIPPER_OPEN = 130;

void setup() {

  baseServo.attach(2);
  shoulderServo.attach(3);
  elbowServo.attach(4);
  wristRotServo.attach(5);
  wristServo.attach(6);
  gripperServo.attach(7);

  Serial.begin(9600);
  Serial.setTimeout(50);
  delay(1000);

  moveToHome();

  Serial.println("Robotic Arm Ready (Semi-Circle Mode)");
  lastActivityTime = millis();
}

void loop() {

  if (Serial.available() > 0) {

    String received = Serial.readStringUntil('\n');
    received.trim();

    if (received.length() > 0) {

      int receivedCode = received.toInt();

      if (receivedCode >= 1 && receivedCode <= 4) {

        if (!armBusy) {
          currentPlasticType = receivedCode;
          Serial.println("ACK");
          executePickAndPlace(currentPlasticType);
        } else {
          Serial.println("BUSY");
        }
      }

      lastActivityTime = millis();
    }
  }

  if (!armBusy && (millis() - lastActivityTime > 30000)) {
    moveToHome();
    lastActivityTime = millis();
  }
}

void executePickAndPlace(int plasticType) {

  armBusy = true;

  smoothMove(PICKUP_BASE, APPROACH_SHOULDER, APPROACH_ELBOW,
             PICKUP_WRIST_ROT, PICKUP_WRIST, PICKUP_GRIPPER_OPEN);
  delay(300);

  smoothMove(PICKUP_BASE, PICKUP_SHOULDER, PICKUP_ELBOW,
             PICKUP_WRIST_ROT, PICKUP_WRIST, PICKUP_GRIPPER_OPEN);
  delay(500);

  smoothMoveGripper(PICKUP_GRIPPER_CLOSED);
  delay(500);

  smoothMove(PICKUP_BASE, APPROACH_SHOULDER, APPROACH_ELBOW,
             PICKUP_WRIST_ROT, PICKUP_WRIST, PICKUP_GRIPPER_CLOSED);
  delay(300);

  int targetBase = getBinBaseAngle(plasticType);

  smoothMove(targetBase, DROP_SHOULDER, DROP_ELBOW,
             getBinWristRotAngle(plasticType),
             getBinWristAngle(plasticType),
             PICKUP_GRIPPER_CLOSED);
  delay(500);

  smoothMove(targetBase,
             getBinShoulderAngle(plasticType),
             getBinElbowAngle(plasticType),
             getBinWristRotAngle(plasticType),
             getBinWristAngle(plasticType),
             PICKUP_GRIPPER_CLOSED);
  delay(500);

  smoothMoveGripper(DROP_GRIPPER_OPEN);
  delay(500);

  smoothMove(targetBase, DROP_SHOULDER, DROP_ELBOW,
             getBinWristRotAngle(plasticType),
             getBinWristAngle(plasticType),
             DROP_GRIPPER_OPEN);
  delay(300);

  moveToHome();

  armBusy = false;
}

void moveToHome() {
  smoothMove(HOME_BASE, HOME_SHOULDER, HOME_ELBOW,
             HOME_WRIST_ROT, HOME_WRIST, HOME_GRIPPER);
}

int getBinBaseAngle(int plasticType) {
  switch(plasticType) {
    case 1: return BIN1_BASE;
    case 2: return BIN2_BASE;
    case 3: return BIN3_BASE;
    case 4: return BIN4_BASE;
    default: return HOME_BASE;
  }
}

int getBinShoulderAngle(int plasticType) {
  switch(plasticType) {
    case 1: return BIN1_SHOULDER;
    case 2: return BIN2_SHOULDER;
    case 3: return BIN3_SHOULDER;
    case 4: return BIN4_SHOULDER;
    default: return DROP_SHOULDER;
  }
}

int getBinElbowAngle(int plasticType) {
  switch(plasticType) {
    case 1: return BIN1_ELBOW;
    case 2: return BIN2_ELBOW;
    case 3: return BIN3_ELBOW;
    case 4: return BIN4_ELBOW;
    default: return DROP_ELBOW;
  }
}

int getBinWristRotAngle(int plasticType) {
  return 90;
}

int getBinWristAngle(int plasticType) {
  return 90;
}

void smoothMove(int targetBase, int targetShoulder, int targetElbow,
                int targetWristRot, int targetWrist, int targetGripper) {

  int steps = max(
    abs(targetBase - basePos),
    abs(targetShoulder - shoulderPos)
  );
  steps = max(steps, abs(targetElbow - elbowPos));
  steps = max(steps, abs(targetWristRot - wristRotPos));
  steps = max(steps, abs(targetWrist - wristPos));
  steps = max(steps, abs(targetGripper - gripperPos));

  if (steps == 0) return;

  for (int i = 1; i <= steps; i++) {

    baseServo.write(map(i, 0, steps, basePos, targetBase));
    shoulderServo.write(map(i, 0, steps, shoulderPos, targetShoulder));
    elbowServo.write(map(i, 0, steps, elbowPos, targetElbow));
    wristRotServo.write(map(i, 0, steps, wristRotPos, targetWristRot));
    wristServo.write(map(i, 0, steps, wristPos, targetWrist));
    gripperServo.write(map(i, 0, steps, gripperPos, targetGripper));

    delay(MOVE_SPEED);
  }

  basePos = targetBase;
  shoulderPos = targetShoulder;
  elbowPos = targetElbow;
  wristRotPos = targetWristRot;
  wristPos = targetWrist;
  gripperPos = targetGripper;
}

void smoothMoveGripper(int targetGripper) {

  if (targetGripper == gripperPos) return;

  int step = (targetGripper > gripperPos) ? 1 : -1;

  for (int pos = gripperPos; pos != targetGripper; pos += step) {
    gripperServo.write(pos);
    delay(GRIPPER_SPEED);
  }

  gripperServo.write(targetGripper);
  gripperPos = targetGripper;
}