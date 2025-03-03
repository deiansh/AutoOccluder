/*============================================================================\
|                                                                             |
| Title:    AutoOccluder.ino                                                  |
|           ----------------                                                  |
| Version:  0.1.1                                                   |
| Date:     30/01/2025 (curr)                                                 |
| Author:   deians@liv.ac.uk (W213@SoE@UoL)                                   |
| Customer: sgjtowle@liv.ac.uk                                                |
| End User: Alder Hey                                                         |
|                                                                          80>|
\============================================================================*/

/*-----
| TODO |
------*/
// 1. Code up the T switch to run a sequence of events.
//  a. Move stepper movement code to the main loop().
//  b. Consider implementing TODO-3 at the same time?
// 2. Check if all uncommented lines are still required.
// 3. Convert the program to pseudo state machine
//  a. Consider using switch case using and modifying flags...
//  b. Or find a State Machine library to implement.
// 4. Design in ways of modifying the workflow. See Todo-1 and maybe Todo-3.
// 5. Program in debug workflow, see commented line with DEBUG in it below.

//#define DEBUG false // Set to: 'true' for development, 'false' for production

/*----------
| LIBRARIES |
-----------*/

#include <SpeedyStepper.h> // Is there a better library to use? I think not at this stage.

/*-----
| PINS |
------*/

const int L_HOME = A0; // Analog Input Pin of the Optical light sensor.
const int R_HOME = A1; // 0 = !Home / 1 = Home.

const int L_DIR = 2; // Left Direction Pin for the Stepper Stick.
const int L_STEP = 3; // As above but the Step pin... the business end!

const int R_DIR = 4; // Right Direction Pin.
const int R_STEP = 5; // Right Step Pin.

const int L_BUTTON = 8;   // Will drop the Left Flap down.
const int R_BUTTON = 9;   // Same for the Right Flap.
const int T_BUTTON = 10;  // To code up to run the sequence.

const int ENABLE = 11;   // Enables BOTH left and right drivers to allow current into the motors.
const int LED_PIN = 13;  // Shows the state of play while in development. To be 'unused' in production. But can be left.

/*----------
| VARIABLES |
-----------*/

int prevTButtState;
int prevLButtState;
int prevRButtState;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

int prev_l_opto_val;
int prev_r_opto_val;

bool lastPowerState;

int ldown; // 0 = Up or Home
int rdown;

/*----------------------
 OBJECT INITIALISATION |
----------------------*/

SpeedyStepper stepperL;
SpeedyStepper stepperR;

/*------
| SETUP |
-------*/

void setup() {
  //Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(ENABLE, OUTPUT);

  pinMode(L_HOME, INPUT);
  pinMode(R_HOME, INPUT);

  pinMode(L_BUTTON, INPUT_PULLUP);
  pinMode(R_BUTTON, INPUT_PULLUP);
  pinMode(T_BUTTON, INPUT_PULLUP);

  stepperL.connectToPins(L_STEP, L_DIR);
  stepperR.connectToPins(R_STEP, R_DIR);

  digitalWrite(ENABLE, HIGH); // HIGH = MOTORS OFF / LOW = MOTORS ON
  digitalWrite(LED_PIN, LOW); // LOW = OFF / HIGH = ON

  stepperL.setStepsPerRevolution(1600);
  stepperL.setSpeedInRevolutionsPerSecond(8.0);
  stepperL.setAccelerationInRevolutionsPerSecondPerSecond(32.0);
  stepperR.setStepsPerRevolution(1600);
  stepperR.setSpeedInRevolutionsPerSecond(8.0);
  stepperR.setAccelerationInRevolutionsPerSecondPerSecond(32.0);
  
  goHome();
}

void enable() {
  digitalWrite(ENABLE, LOW);
  digitalWrite(LED_PIN, HIGH);
}

void disable() {
  digitalWrite(ENABLE, HIGH);
  digitalWrite(LED_PIN, LOW);
}

void goHome() {
  // Ideally a homing switch would have been better for home, they can be small and quiet.
  // Personally I'd have the optical switch on the down position, so the movement relies less on a constant value.
  // The logic below homes (moves up) the Left flap THEN the right flap, ONLY if they are NOT already home (L_HOME=HIGH).
  // If they are ALREADY home, then no need to do the traditional moving away thing like if you had a mechanical switch.
  // We might STILL try, the moving away then back thing, but I don't think it's needed in this case because of the nature of the optical switch vs mechanical switch.
 
  if (digitalRead(L_HOME) == LOW) {
    enable();
    while (digitalRead(L_HOME) == LOW) {
      stepperL.moveRelativeInSteps(1);
    }
    stepperL.setCurrentPositionInRevolutions(0); // Sets the reference point.
    ldown = 0;
  }
  if (digitalRead(R_HOME) == LOW) {
    enable();
    while (digitalRead(R_HOME) == LOW) {
      stepperR.moveRelativeInSteps(-1);
    }
    stepperR.setCurrentPositionInRevolutions(0);
    rdown = 0;
  }
}

void moveLhome() {
  // This is a move Home AFTER the initial Homing sequence that gets done upon power up.
  if ((digitalRead(L_HOME) == LOW) or (ldown == 1)) {
    enable();
    stepperL.setupMoveInRevolutions(0);
    while (!stepperL.motionComplete()) {
      stepperL.processMovement();
    }
  }
  ldown = 0;
}

void moveRhome() {
  if ((digitalRead(R_HOME) == LOW) or (rdown == 1)) {
    enable();
    stepperR.setupMoveInRevolutions(0); // See propositin below.
    while (!stepperR.motionComplete()) { // This While loop could potentially be moved to the main loop.
      stepperR.processMovement();
    }
  }
  rdown = 0;
}

void moveRdown() {
  if ((digitalRead(R_HOME) == HIGH) && (rdown == 0)) {
    enable();
    stepperR.setupMoveInRevolutions(0.235);
    while (!stepperR.motionComplete()) {
      stepperR.processMovement();
    }
  }
  rdown = 1;
}

void moveLdown() {
  if ((digitalRead(L_HOME) == HIGH) && (ldown == 0)) {
    enable();
    stepperL.setupMoveInRevolutions(-0.232);
    while (!stepperL.motionComplete()) {
      stepperL.processMovement();
    }
  }
  ldown = 1;
}

void readButtons() {
  readLButt();
  readRButt();
  readTButt();
}

void readLButt() {
  int lButtState = digitalRead(L_BUTTON);
  if (lButtState == LOW) {
    moveLdown();
  } else {
    moveLhome();
  }
}

void readRButt() {
  int rButtState = digitalRead(R_BUTTON);
  if (rButtState == LOW) {
    moveRdown();
  } else {
    moveRhome();
  }
}

void readTButt() {
  int tButtState = digitalRead(T_BUTTON);
  prevTButtState = tButtState;
}

void readOptos() {
  int l_opto_val = digitalRead(L_HOME);
  prev_l_opto_val = l_opto_val;
  int r_opto_val = digitalRead(R_HOME);
  prev_r_opto_val = r_opto_val;
}

/*----------
| MAIN LOOP |
-----------*/

void loop() {
  readButtons();
  readOptos();
  //ShowDebugInfo();
}
