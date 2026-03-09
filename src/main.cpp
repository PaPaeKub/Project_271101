#include <Arduino.h>
#include <motor.h>      // Use < > because it is in the /lib folder
#include <config.h>     // Includes INTAKE_PIN and motor settings
#include <ESP32Servo.h> // Include library for Servo motor
#include <PIDF.h>
#include <PIDF_config.h>

// --- Include Dabble library for Bluetooth ---
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

#define STBY 32

// --- Function Prototypes ---
void steering(int x, int omega);
void MoveMent(); 
void intake(); // Function for intake servo

// --- Create Motor and Servo Objects ---
// Parameters: (Mode, Frequency, Bits, Invert, Brake, PWM_Pin, IN1, IN2)
Controller motorL(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, AINV, true, PWMA, AIN1, AIN2);
Controller motorR(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, BINV, true, PWMB, BIN1, BIN2);
PIDF Tagline(PWM_Min, PWM_Max, Tagline_KP, Tagline_KI, Tagline_I_Min, Tagline_I_Max, Tagline_KD, Tagline_KF, Tagline_ERROR_TOLERANCE);
Servo intakeServo; // Create servo object

int speed = 0; 
int r = 0;

void setup() {
    Serial.begin(115200);
    
    // Enable Standby Pin for Motor Driver
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);

    // Attach the intake servo to pin 17 (from config.h)
    intakeServo.attach(INTAKE_PIN);
    intakeServo.write(90); // Stop servo at startup (90 means stop for 360 servos)
    
    // Set Bluetooth name for the app (You can change this)
    Dabble.begin("MyRobot_PPAP"); 
    
    Serial.println("Bluetooth Started! Waiting for Dabble app to connect...");
}

void loop() {
    // 1. Update Bluetooth data continuously (Required, or the app will freeze)
    Dabble.processInput(); 

    // 2. Check gamepad buttons for movement and intake
    MoveMent();
    intake();

    // 3. Send values to move the robot
    steering(speed, r);
}

void MoveMent() {
    int v = 1023; 

    speed = GamePad.isUpPressed() ? v : (GamePad.isDownPressed() ? -v : 0);
    r = GamePad.isLeftPressed() ? -v : (GamePad.isRightPressed() ? v : 0);
}

void intake() {
    // Check if the 'Cross' (X) button is pressed
    if (GamePad.isCrossPressed()) {
        intakeServo.write(180); // Spin full speed (Change to 0 if it spins the wrong way)
    } else {
        intakeServo.write(90);  // Stop spinning when released
    }
}

// --- Motor Control Function ---
void steering(int x, int omega) {
    // Calculate speed for each wheel (Differential Drive)
    int pwmLeft = x + omega;
    int pwmRight = x - omega;

    // Limit speed to prevent exceeding PWM limits (-1023 to 1023)
    pwmLeft = constrain(pwmLeft, -1023, 1023);
    pwmRight = constrain(pwmRight, -1023, 1023);
    
    motorL.spin(pwmLeft);
    motorR.spin(pwmRight);
}