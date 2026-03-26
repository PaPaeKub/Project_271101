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
int Right_Errors, Left_Errors = 0;
float error, Steer = 0.0;

// Toggle states for Gamepad buttons
bool isDropped = false;         
bool lastTriangleState = false;
bool isAutoMode = false;      
bool lastStartState = false;  

// Sensor reading variables
int S1, S3, S6, S8 = 0;

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
    pinMode(SENSOR_3, INPUT);
    pinMode(SENSOR_6, INPUT);
    pinMode(SENSOR_8, INPUT);
    

    // --- Allocate Hardware Timers for ESP32Servo ---
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
    dropservo.write(90);

    dropservo2.attach(DROP2_PIN, 500, 2500); 
    dropservo2.write(90); 
    
    // Initialize Dabble Bluetooth communication
    Dabble.begin("MyRobot_PPAP"); 
}


void loop() {
    Dabble.processInput(); 

     // ตัวแปรเก็บเวลาสำหรับการโชว์ค่า (Static เพื่อให้จำค่าได้)
//  static unsigned long lastPrintTime = 0;
    
//     if (GamePad.isCrossPressed()) {
//         tagline(1023); // ให้หุ่นวิ่งแทร็กเส้นปกติ

//         // โชว์ค่าแบบหน่วงเวลา (ทุก 200ms) ค่าที่โชว์จะเป็น 0-100 แล้ว
//         if (millis() - lastPrintTime >= 200) { 
//             Serial.print("S1:"); Serial.print(S1);
//             Serial.print(" S3:"); Serial.print(S3);
//             Serial.print(" S6:"); Serial.print(S6);
//             Serial.print(" S8:"); Serial.print(S8);
//             Serial.print(" | Err:"); Serial.println(error);
//             lastPrintTime = millis(); 
//         }
//         return; // สำคัญมาก! ถ้ากด X อยู่ ให้ข้ามโค้ดด้านล่างทั้งหมดไปเลย
//     }

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
                dropservo.write(30);
                dropservo2.write(150);
            } else {
                // Close drop gates
                dropservo.write(90);
                dropservo2.write(90);
            }
        }
        lastTriangleState = currentTriangleState;
    }
}

//----------------------------------------------------------------------------------------------------------------------

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
    int Power = 1023;      // ความเร็วสูงสุด 1000

    // เริ่มจับเวลาครั้งแรกเมื่อเข้าโหมด Auto
    if (autoStartTime == 0 && autoState == 0) {
        autoStartTime = millis(); 
        Serial.println("--- State 0: วิ่งตามเส้น 7 วินาที ---");
    }

    // [State 4] จบโปรแกรม
    if (autoState == 4) {
        steering(0, 0);
        return; 
    }

    // ==========================================
    // State Machine Logic (ลำดับการทำงาน)
    // ==========================================
    
    if (autoState == 0) {
        // [State 0] วิ่งตามเส้น 7 วินาที
        tagline(Power); 
        
        if (millis() - autoStartTime >= 6000) {
            autoStartTime = millis(); // รีเซ็ตนาฬิกาจับเวลา
            autoState = 1;            // ขยับไปทำท่าต่อไป
            Serial.println("--- State 1: เลี้ยวซ้ายล้อเดียว 1 วินาที ---");
        }
    } 
    else if (autoState == 1) {
        // [State 1] เลี้ยวซ้ายล้อเดียว (ล้อซ้ายหยุด ล้อขวาเดินหน้า) 1 วินาที
        motorL.spin(0);
        motorR.spin(Power);
       
        if (millis() - autoStartTime >= 1100) {
            intakeServo.write(0);     // หุบเซอร์โว
            autoStartTime = millis(); // รีเซ็ตนาฬิกาจับเวลา
            autoState = 2;
            Serial.println("--- State 2: หุบเซอร์โวแล้ว เลี้ยวซ้ายล้อเดียวต่อ 1 วินาที ---");
        }
    }
    else if (autoState == 2) {
        // [State 2] เลี้ยวซ้ายล้อเดียวต่อไปอีก 1 วินาที
        motorL.spin(0);
        motorR.spin(Power);

        if (millis() - autoStartTime >= 1100) {
            autoStartTime = millis(); // รีเซ็ตนาฬิกาจับเวลา
            autoState = 3;
            Serial.println("--- State 3: วิ่งตามเส้นต่อ 7 วินาที ---");
        }
    }
    else if (autoState == 3) {
        // [State 3] วิ่งตามเส้นต่อ 7 วินาที
        tagline(Power);

        if (millis() - autoStartTime >= 6000) {
            autoState = 4; // ไปยังสถานะหยุดการทำงาน
            Serial.println("--- จบการทำงาน Auto ---");
        }
    }
}

void tagline(int Power) {
    // 1. กำหนดค่าดิบต่ำสุดของสีขาว และสูงสุดของสีดำ (ปรับให้เข้ากับเซนเซอร์ของคุณ)
    int minWhite = 2200; 
    int maxBlack = 4095;

    // 2. อ่านค่าดิบ -> แปลงเป็นสเกล 0-100 -> ล็อคไม่ให้เกิน 0-100
    S1 = constrain(map(analogRead(SENSOR_1), minWhite, maxBlack, 0, 100), 0, 100);
    S3 = constrain(map(analogRead(SENSOR_3), minWhite, maxBlack, 0, 100), 0, 100);
    S6 = constrain(map(analogRead(SENSOR_6), minWhite, maxBlack, 0, 100), 0, 100);
    S8 = constrain(map(analogRead(SENSOR_8), minWhite, maxBlack, 0, 100), 0, 100);

    // 3. กำหนด Threshold สำหรับพื้นขาว (บนสเกล 0-100)
    // ถ้าค่าของเซนเซอร์ต่ำกว่า 40 ถือว่าเจอสีขาว
    int whiteThreshold = 40; 

    // 4. เช็คเส้นปะ (ถ้าเจอสีขาวหมดทุกตัว)
    if (S1 < whiteThreshold && S3 < whiteThreshold && S6 < whiteThreshold && S8 < whiteThreshold) {
        // สั่งเดินหน้าตรงๆ โดยไม่คำนวณ PID
        steering(Power, 0); 
        return; 
    }

    // 5. คำนวณ PID บนสเกลใหม่ (0-100)
    Left_Errors  = ((S1*3) + (S3*1));
    Right_Errors = ((S8*3) + (S6*1));

    error = Left_Errors - Right_Errors;
    Steer = Tagline.compute_with_error(error);
    
    steering(Power, Steer);
}

// --- Differential Steering Control ---
void steering(int x, int omega) {
    // Calculate individual PWM values for left and right motors
    int pwmLeft = x + omega;
    int pwmRight = (x - omega) * 0.85;

    int max_power = 1023; // Maximum allowed PWM value
    // Constrain the calculated PWM values to prevent hardware overload
    pwmLeft = constrain(pwmLeft, -max_power, max_power);
    pwmRight = constrain(pwmRight, -max_power, max_power);
    
    // Spin the motors
    motorL.spin(pwmLeft);
    motorR.spin(pwmRight);
}