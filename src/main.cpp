#include <Arduino.h>
#include <motor.h>      
#include <config.h>     
#include <ESP32Servo.h> 
#include <PIDF.h>       
#include <PIDF_config.h>

// --- Include Dabble library for Bluetooth gamepad ---
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// --- Function Prototypes ---
void steering(int x, int omega);
void MoveMent(); 
void autonomus();
void tagline(int Power);

// Initialize Left and Right motors with PWM settings from config.h
Controller motorL(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, AINV, true, PWMA, AIN1, AIN2);
Controller motorR(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, BINV, true, PWMB, BIN1, BIN2);

// Initialize PID controller for line tracking
PIDF Tagline(PWM_Min, PWM_Max, Tagline_KP, Tagline_KI, Tagline_I_Min, Tagline_I_Max, Tagline_KD, Tagline_KF, Tagline_ERROR_TOLERANCE);

Servo intakeServo, dropservo, dropservo2; 

// --- Global Variables ---
int speed = 0; 
int r = 0;
int intakeState = 90; 

// Toggle states for Gamepad buttons
bool isDropped = false;         
bool lastTriangleState = false;
bool isAutoMode = false;      
bool lastStartState = false;  

// Sensor reading variables
int S1, S2, S3, S4, S5, S6 = 0;

// Autonomous mode state machine variables
unsigned long autoStartTime = 0; 
int autoState = 0; 


void setup() {
    Serial.begin(115200);
    
    // Enable Standby Pin for the Motor Driver
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);

    // Setup and enable the IR LED control pin for the sensors
    pinMode(IR_PIN, OUTPUT);
    digitalWrite(IR_PIN, HIGH); 
   
    // Setup QRE1113 line sensor pins as inputs
    pinMode(SENSOR_1, INPUT);
    pinMode(SENSOR_2, INPUT);
    pinMode(SENSOR_3, INPUT);
    pinMode(SENSOR_4, INPUT);
    pinMode(SENSOR_5, INPUT);
    pinMode(SENSOR_6, INPUT);
    

    // --- Allocate Hardware Timers for ESP32Servo ---
    // This prevents PWM signal conflicts between the motors and servos
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    intakeServo.setPeriodHertz(50);
    dropservo.setPeriodHertz(50);
    dropservo2.setPeriodHertz(50);

    // Attach Servos to pins and set initial positions
    intakeServo.attach(INTAKE_PIN, 500, 2500); 
    intakeServo.write(90);
    
    dropservo.attach(DROP_PIN, 500, 2500); 
    dropservo.write(0);

    dropservo2.attach(DROP2_PIN, 500, 2500); 
    dropservo2.write(0); 
    
    // Initialize Dabble Bluetooth communication
    Dabble.begin("MyRobot_PPAP"); 
}

void loop() {
    // Process incoming data from Dabble app
    Dabble.processInput(); 

    // ==========================================
    // 1. Start Button Toggle Logic (Auto/Manual)
    // ==========================================
    bool currentStartState = GamePad.isStartPressed();
    
    // Detect button press (edge detection)
    if (currentStartState == true && lastStartState == false) {
        isAutoMode = !isAutoMode; // Toggle mode
        
        if (!isAutoMode) {
            // Switched to Manual Mode: Stop motors immediately for safety
            steering(0, 0); 
            Serial.println("--- Switched to MANUAL Mode ---");
        } else {
            // Switched to Auto Mode: Reset the state machine and timer
            autoStartTime = 0; 
            autoState = 0;     
            Serial.println("--- Switched to AUTO Mode ---");
        }
    }
    lastStartState = currentStartState;

    // ==========================================
    // 2. Mode Execution (Auto or Manual)
    // ==========================================
    if (isAutoMode) {
        // Execute autonomous line tracking routine
        autonomus(); 

    } else {
        // Execute manual control via D-Pad
        MoveMent();

        // Manual Intake Servo Control (Hold to move)
        if(GamePad.isSquarePressed()){
            intakeServo.write(0);
        }
        else if(GamePad.isCrossPressed()){
            intakeServo.write(90);
        }
        else if(GamePad.isCirclePressed()){
            intakeServo.write(180);
        }
        
        // Manual Drop Servo Control (Toggle button)
        bool currentTriangleState = GamePad.isTrianglePressed();
        if (currentTriangleState == true && lastTriangleState == false) {
            isDropped = !isDropped; 
            if (isDropped) {
                // Open drop gates
                dropservo.write(90);
                dropservo2.write(90);
            } else {
                // Close drop gates
                dropservo.write(0);
                dropservo2.write(180);
            }
        }
        lastTriangleState = currentTriangleState;
    }
}

// --- Manual Movement Function ---
void MoveMent() {
    int velocity = 1023; // Maximum forward/backward speed
    int rotation = 511;  // Maximum turning speed

    // Determine speed and rotation based on D-Pad input using ternary operators
    speed = GamePad.isUpPressed() ? velocity : (GamePad.isDownPressed() ? -velocity : 0);
    r = GamePad.isLeftPressed() ? -rotation : (GamePad.isRightPressed() ? rotation : 0);
    
    steering(speed, r);
}

// --- Autonomous Line Tracking Sequence ---
void autonomus() {
    int Power = 1000;      // Base speed for line tracking
    int threshold = 2000;  // Analog threshold to detect black line (0-4095)

    // Initialize the timer on the first execution of the state machine
    if (autoStartTime == 0 && autoState == 0) {
        autoStartTime = millis(); 
        Serial.println("--- Auto State 0: Tracking for 10 seconds ---");
    }

    // State 2: Program finished. Stop motors and exit function.
    if (autoState == 2) {
        steering(0, 0);
        return; 
    }

    // --- 1. Read Line Sensors ---
    S1 = analogRead(SENSOR_1);
    S2 = analogRead(SENSOR_2);
    S3 = analogRead(SENSOR_3);
    S4 = analogRead(SENSOR_4);
    S5 = analogRead(SENSOR_5);
    S6 = analogRead(SENSOR_6);

    // --- 2. State Machine Logic ---
    if (autoState == 0) {
        // [State 0] Track the line continuously until 10 seconds (10,000 ms) pass
        if (millis() - autoStartTime >= 10000) {
            intakeServo.write(0); // Retract intake servo
            autoState = 1;        // Move to the next state
            Serial.println("--- 10 Seconds Reached! Servo Retracted. Moving to State 1 ---");
        }
    } 
    else if (autoState == 1) {
        // [State 1] Track the line until ALL sensors detect the black crossline
        if (S1 > threshold && S2 > threshold && S3 > threshold && 
            S4 > threshold && S5 > threshold && S6 > threshold) {
            
            steering(0, 0);         // Hard brake
            intakeServo.write(90);  // Deploy intake servo
            autoState = 2;          // Move to final stop state
            
            Serial.println("--- Crossline Detected! Servo Deployed. Auto Sequence Finished ---");
            return; // Exit immediately to prevent further PID calculation
        }
    }

    // --- 3. PID Line Tracking Calculation (Active during State 0 and 1) ---
    // Calculate the weighted average for left and right sensor arrays
    int Left_error  = ((S1*3) + (S2*2) + (S3*1));
    int Right_error = ((S6*3) + (S5*2) + (S4*1));

    // Calculate the difference and compute the PID steering correction
    float error = Right_error - Left_error;
    float Steer = Tagline.compute_with_error(error);
    
    // Apply speed and steering correction to the motors
    steering(Power, Steer);
}

// --- Differential Steering Control ---
void steering(int x, int omega) {
    // Calculate individual PWM values for left and right motors
    int pwmLeft = x + omega;
    int pwmRight = (x - omega) * 0.95;

    int max_power = 1023; // Maximum allowed PWM value
    // Constrain the calculated PWM values to prevent hardware overload
    pwmLeft = constrain(pwmLeft, -max_power, max_power);
    pwmRight = constrain(pwmRight, -max_power, max_power);
    
    // Spin the motors
    motorL.spin(pwmLeft);
    motorR.spin(pwmRight);
}