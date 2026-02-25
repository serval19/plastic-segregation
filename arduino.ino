/*
 * Plastic Segregation Robotic Arm
 * 6-DOF Arm controlled via Serial from Laptop
 * CORRECTED PIN CONFIGURATION:
 * Pin 2: Base motor
 * Pin 3: Shoulder motor
 * Pin 4: Elbow motor
 * Pin 5: Wrist Rotation motor (ROLL)
 * Pin 6: Wrist motor (PITCH)
 * Pin 7: Gripper motor
 * 
 * Receives codes 1-4 for different plastic types
 * Picks from ground, places in appropriate bin, returns to home
 * Bins placed at 4 corners of a square around the arm
 */

#include <Servo.h>

// Create servo objects for 6 motors with CORRECT pin assignments
Servo baseServo;           // Pin 2 - Base rotation
Servo shoulderServo;       // Pin 3 - Shoulder
Servo elbowServo;          // Pin 4 - Elbow
Servo wristRotServo;       // Pin 5 - Wrist ROTATION (roll/spin) - NOW ON PIN 5
Servo wristServo;          // Pin 6 - Wrist (pitch/up-down) - NOW ON PIN 6
Servo gripperServo;        // Pin 7 - Gripper - NOW ON PIN 7

// Current servo positions
int basePos = 90;
int shoulderPos = 90;
int elbowPos = 90;
int wristRotPos = 90;      // Wrist rotation (pin 5)
int wristPos = 90;         // Wrist up/down (pin 6)
int gripperPos = 90;       // Gripper (pin 7)

// Movement speed (delay between steps - lower = faster)
const int MOVE_SPEED = 25;
const int GRIPPER_SPEED = 20;

// State variables
bool armBusy = false;
int currentPlasticType = 0;
unsigned long lastActivityTime = 0;
bool debugMode = true;  // Set to false to disable debug messages

// ============ POSITION CALIBRATION ============
// IMPORTANT: YOU MUST CALIBRATE THESE ANGLES FOR YOUR SPECIFIC ARM
// These are approximate values - adjust based on your arm's geometry

// HOME POSITION (resting position - arm folded/retracted)
const int HOME_BASE = 90;
const int HOME_SHOULDER = 30;     // Shoulder up
const int HOME_ELBOW = 120;        // Elbow bent
const int HOME_WRIST_ROT = 90;     // Wrist rotation neutral
const int HOME_WRIST = 90;         // Wrist straight
const int HOME_GRIPPER = 100;      // Slightly open

// PICKUP POSITION (where plastic is on ground)
// Assume plastic is picked from front of arm (base angle 90)
const int PICKUP_BASE = 90;
const int PICKUP_SHOULDER = 70;     // Lower shoulder to reach ground
const int PICKUP_ELBOW = 60;        // Extend elbow
const int PICKUP_WRIST_ROT = 90;    // Keep rotation neutral for pickup
const int PICKUP_WRIST = 80;        // Angle wrist down to grab from ground
const int PICKUP_GRIPPER_OPEN = 130;  // Fully open gripper
const int PICKUP_GRIPPER_CLOSED = 40; // Fully closed gripper

// APPROACH POSITION (just above the pickup point)
const int APPROACH_SHOULDER = 50;
const int APPROACH_ELBOW = 70;

// BIN POSITIONS (4 corners of a square around the arm)
// Bin 1: Milk Packet - Front-Right corner (45°)
const int BIN1_BASE = 45;
const int BIN1_SHOULDER = 40;
const int BIN1_ELBOW = 80;
const int BIN1_WRIST_ROT = 90;      // Adjust if bins need specific orientation
const int BIN1_WRIST = 90;

// Bin 2: Color LD - Back-Right corner (135°)
const int BIN2_BASE = 135;
const int BIN2_SHOULDER = 40;
const int BIN2_ELBOW = 80;
const int BIN2_WRIST_ROT = 90;
const int BIN2_WRIST = 90;

// Bin 3: MLP - Back-Left corner (225°)
const int BIN3_BASE = 225;
const int BIN3_SHOULDER = 40;
const int BIN3_ELBOW = 80;
const int BIN3_WRIST_ROT = 90;
const int BIN3_WRIST = 90;

// Bin 4: PP - Front-Left corner (315°)
const int BIN4_BASE = 315;
const int BIN4_SHOULDER = 40;
const int BIN4_ELBOW = 80;
const int BIN4_WRIST_ROT = 90;
const int BIN4_WRIST = 90;

// Common drop position (height above bins)
const int DROP_SHOULDER = 45;
const int DROP_ELBOW = 75;
const int DROP_WRIST_ROT = 90;
const int DROP_WRIST = 90;
const int DROP_GRIPPER_OPEN = 130;

void setup() {
  // Attach servos to pins with CORRECT assignments
  baseServo.attach(2);        // Base on pin 2
  shoulderServo.attach(3);    // Shoulder on pin 3
  elbowServo.attach(4);       // Elbow on pin 4
  wristRotServo.attach(5);    // Wrist ROTATION on pin 5
  wristServo.attach(6);       // Wrist on pin 6
  gripperServo.attach(7);     // Gripper on pin 7
  
  // Initialize serial communication (matches laptop's 9600 baud)
  Serial.begin(9600);
  Serial.setTimeout(50);
  
  // Initial delay for serial to stabilize
  delay(1000);
  
  if (debugMode) {
    Serial.println("Robotic Arm Initializing...");
    Serial.println("Pin Configuration:");
    Serial.println("Pin 2: Base");
    Serial.println("Pin 3: Shoulder");
    Serial.println("Pin 4: Elbow");
    Serial.println("Pin 5: Wrist Rotation");
    Serial.println("Pin 6: Wrist");
    Serial.println("Pin 7: Gripper");
  }
  
  // Move to home position at startup
  moveToHome();
  
  if (debugMode) {
    Serial.println("Ready");
    Serial.println("Waiting for plastic type codes (1-4)...");
  }
  
  lastActivityTime = millis();
}

void loop() {
  // Check for incoming serial data from laptop
  if (Serial.available() > 0) {
    String received = Serial.readStringUntil('\n');
    received.trim();  // Remove whitespace
    
    if (received.length() > 0) {
      int receivedCode = received.toInt();
      
      if (debugMode) {
        Serial.print("Received: ");
        Serial.println(receivedCode);
      }
      
      // Process valid plastic codes (1-4)
      if (receivedCode >= 1 && receivedCode <= 4) {
        if (!armBusy) {
          currentPlasticType = receivedCode;
          
          // Send acknowledgment back to laptop
          Serial.println("ACK");
          
          // Execute pick and place sequence
          executePickAndPlace(currentPlasticType);
        } else {
          // Arm is busy, tell laptop to wait
          Serial.println("BUSY");
        }
      } else if (receivedCode == 0 && debugMode) {
        // Test command or unknown
        Serial.println("UNKNOWN");
      }
      
      lastActivityTime = millis();
    }
  }
  
  // Optional: Add timeout to return to home if idle too long
  if (!armBusy && (millis() - lastActivityTime > 30000)) {  // 30 seconds idle
    if (debugMode) {
      Serial.println("Auto-returning to home");
    }
    moveToHome();
    lastActivityTime = millis();
  }
}

void executePickAndPlace(int plasticType) {
  armBusy = true;
  
  if (debugMode) {
    Serial.print("Starting pick-and-place for plastic type ");
    Serial.println(plasticType);
  }
  
  // STEP 1: Move to approach position (above pickup)
  if (debugMode) Serial.println("1. Moving to approach position...");
  smoothMove(
    PICKUP_BASE,
    APPROACH_SHOULDER,
    APPROACH_ELBOW,
    PICKUP_WRIST_ROT,    // Wrist rotation (pin 5)
    PICKUP_WRIST,        // Wrist up/down (pin 6)
    PICKUP_GRIPPER_OPEN  // Gripper (pin 7)
  );
  delay(300);
  
  // STEP 2: Move down to pickup position
  if (debugMode) Serial.println("2. Lowering to pickup position...");
  smoothMove(
    PICKUP_BASE,
    PICKUP_SHOULDER,
    PICKUP_ELBOW,
    PICKUP_WRIST_ROT,
    PICKUP_WRIST,
    PICKUP_GRIPPER_OPEN
  );
  delay(500);
  
  // STEP 3: Close gripper to grab plastic
  if (debugMode) Serial.println("3. Grabbing plastic...");
  smoothMoveGripper(PICKUP_GRIPPER_CLOSED);
  delay(500);
  
  // STEP 4: Lift plastic (back to approach position)
  if (debugMode) Serial.println("4. Lifting plastic...");
  smoothMove(
    PICKUP_BASE,
    APPROACH_SHOULDER,
    APPROACH_ELBOW,
    PICKUP_WRIST_ROT,
    PICKUP_WRIST,
    PICKUP_GRIPPER_CLOSED
  );
  delay(300);
  
  // STEP 5: Rotate to appropriate bin and move to drop height
  if (debugMode) Serial.println("5. Moving to bin...");
  int targetBase = getBinBaseAngle(plasticType);
  smoothMove(
    targetBase,
    DROP_SHOULDER,
    DROP_ELBOW,
    getBinWristRotAngle(plasticType),  // Wrist rotation for this bin
    getBinWristAngle(plasticType),      // Wrist angle for this bin
    PICKUP_GRIPPER_CLOSED
  );
  delay(500);
  
  // STEP 6: Lower into bin
  if (debugMode) Serial.println("6. Lowering into bin...");
  smoothMove(
    targetBase,
    getBinShoulderAngle(plasticType),
    getBinElbowAngle(plasticType),
    getBinWristRotAngle(plasticType),
    getBinWristAngle(plasticType),
    PICKUP_GRIPPER_CLOSED
  );
  delay(500);
  
  // STEP 7: Open gripper to release plastic
  if (debugMode) Serial.println("7. Releasing plastic...");
  smoothMoveGripper(DROP_GRIPPER_OPEN);
  delay(500);
  
  // STEP 8: Lift back to drop height
  if (debugMode) Serial.println("8. Lifting from bin...");
  smoothMove(
    targetBase,
    DROP_SHOULDER,
    DROP_ELBOW,
    getBinWristRotAngle(plasticType),
    getBinWristAngle(plasticType),
    DROP_GRIPPER_OPEN
  );
  delay(300);
  
  // STEP 9: Return to home position
  if (debugMode) Serial.println("9. Returning to home...");
  moveToHome();
  
  if (debugMode) {
    Serial.println("✅ Pick and place complete!");
    Serial.println("Ready for next plastic.");
  }
  
  armBusy = false;
}

void moveToHome() {
  smoothMove(
    HOME_BASE,
    HOME_SHOULDER,
    HOME_ELBOW,
    HOME_WRIST_ROT,   // Wrist rotation
    HOME_WRIST,       // Wrist
    HOME_GRIPPER      // Gripper
  );
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
  switch(plasticType) {
    case 1: return BIN1_WRIST_ROT;
    case 2: return BIN2_WRIST_ROT;
    case 3: return BIN3_WRIST_ROT;
    case 4: return BIN4_WRIST_ROT;
    default: return DROP_WRIST_ROT;
  }
}

int getBinWristAngle(int plasticType) {
  switch(plasticType) {
    case 1: return BIN1_WRIST;
    case 2: return BIN2_WRIST;
    case 3: return BIN3_WRIST;
    case 4: return BIN4_WRIST;
    default: return DROP_WRIST;
  }
}

void smoothMove(int targetBase, int targetShoulder, int targetElbow, 
                int targetWristRot, int targetWrist, int targetGripper) {
  
  // Calculate maximum steps needed
  int steps = max(
    abs(targetBase - basePos),
    abs(targetShoulder - shoulderPos)
  );
  steps = max(steps, abs(targetElbow - elbowPos));
  steps = max(steps, abs(targetWristRot - wristRotPos));
  steps = max(steps, abs(targetWrist - wristPos));
  steps = max(steps, abs(targetGripper - gripperPos));
  
  // If no movement needed, return
  if (steps == 0) return;
  
  // Move all servos simultaneously
  for (int i = 1; i <= steps; i++) {
    if (targetBase != basePos) {
      basePos = map(i, 0, steps, basePos, targetBase);
      baseServo.write(basePos);
    }
    
    if (targetShoulder != shoulderPos) {
      shoulderPos = map(i, 0, steps, shoulderPos, targetShoulder);
      shoulderServo.write(shoulderPos);
    }
    
    if (targetElbow != elbowPos) {
      elbowPos = map(i, 0, steps, elbowPos, targetElbow);
      elbowServo.write(elbowPos);
    }
    
    if (targetWristRot != wristRotPos) {
      wristRotPos = map(i, 0, steps, wristRotPos, targetWristRot);
      wristRotServo.write(wristRotPos);
    }
    
    if (targetWrist != wristPos) {
      wristPos = map(i, 0, steps, wristPos, targetWrist);
      wristServo.write(wristPos);
    }
    
    if (targetGripper != gripperPos) {
      gripperPos = map(i, 0, steps, gripperPos, targetGripper);
      gripperServo.write(gripperPos);
    }
    
    delay(MOVE_SPEED);
  }
  
  // Ensure exact final positions
  baseServo.write(targetBase);
  shoulderServo.write(targetShoulder);
  elbowServo.write(targetElbow);
  wristRotServo.write(targetWristRot);
  wristServo.write(targetWrist);
  gripperServo.write(targetGripper);
  
  // Update position variables
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

// Emergency stop function - can be called if needed
void emergencyStop() {
  if (debugMode) {
    Serial.println("⚠️ EMERGENCY STOP!");
  }
  armBusy = false;
  // Don't move servos, just stop
}