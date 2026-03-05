#include <Arduino.h>
#include <motor.h>    // Use < > because it is in the /lib folder
#include <config.h>   // Use " " because it is in the /include folder

// --- เรียกใช้ไลบรารี Dabble สำหรับ Bluetooth ---
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

#define STBY 32

// --- Function Prototypes ---
void steering(int x, int omega);

// --- Create Motor Objects ---
// Parameters: (Mode, Frequency, Bits, Invert, Brake, PWM_Pin, IN1, IN2)
Controller motorL(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, AINV, true, PWMA, AIN1, AIN2);
Controller motorR(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, BINV, true, PWMB, BIN1, BIN2);

int speed = 0; 
int r = 0;

void setup() {
    Serial.begin(115200);
    
    // เปิดใช้งาน Standby Pin ของ Driver มอเตอร์
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);
    
    // ตั้งชื่อ Bluetooth ที่จะไปโผล่ในแอป (สามารถแก้ชื่อตรงนี้ได้เลย)
    Dabble.begin("MyRobot_ESP32"); 
    
    Serial.println("Bluetooth Started! Waiting for Dabble app to connect...");
}

void loop() {
    // 1. อัปเดตข้อมูลจาก Bluetooth ตลอดเวลา (ขาดบรรทัดนี้แอปจะค้าง/คุมไม่ได้)
    Dabble.processInput(); 

    // 2. ตรวจสอบการกดปุ่มบน Gamepad ของ Dabble
    if (GamePad.isUpPressed()) {
        speed = 511; // เดินหน้าเต็มกำลัง
        r = 0;
    } 
    else if (GamePad.isDownPressed()) {
        speed = -511; // ถอยหลังเต็มกำลัง
        r = 0;
    } 
    else if (GamePad.isLeftPressed()) {
        speed = 0;
        r = -511; // หมุนอยู่กับที่ไปทางซ้าย
    } 
    else if (GamePad.isRightPressed()) {
        speed = 0;
        r = 511;  // หมุนอยู่กับที่ไปทางขวา
    } 
    else {
        // ถ้าปล่อยปุ่มทั้งหมด ให้หุ่นหยุด
        speed = 0;
        r = 0;
    }

    // 3. ส่งค่าไปคำนวณให้ล้อซ้าย-ขวาหมุน
    steering(speed, r);
}

// --- Motor Control Function ---
void steering(int x, int omega) {
    // คำนวณความเร็วแต่ละล้อแบบ Differential Drive
    int pwmLeft = x + omega;
    int pwmRight = x - omega;

    // ล็อกเพดานความเร็ว ป้องกันค่าทะลุ Limit ของ PWM (511 หรือ -511)
    pwmLeft = constrain(pwmLeft, -511, 511);
    pwmRight = constrain(pwmRight, -511, 511);

    // สั่งมอเตอร์หมุน
    motorL.spin(pwmLeft);
    motorR.spin(pwmRight);
}   