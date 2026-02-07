#include <Arduino.h>
#include <motor.h>    // Use < > because it is in the /lib folder
#include <config.h>   // Use " " because it is in the /include folder
#include <WiFi.h>
#include <WebServer.h>

// --- 1. Function Prototypes ---
void handleRoot();
void handleLEDOn();
void handleLEDOff();
void moveForward(int speed);
void stopRobot();
void connectWiFi();

// --- 1. Create Motor Objects ---
// Parameters: (Mode, Frequency, Bits, Invert, Brake, PWM_Pin, IN1, IN2)
Controller motorL(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, MOT_L_INV, true, MOT_L_PWM, MOT_L_IN1, MOT_L_IN2);
Controller motorR(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, MOT_R_INV, true, MOT_R_PWM, MOT_R_IN1, MOT_R_IN2);

WebServer server(80);

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT); // Ensure ledPin is in your config
    
    connectWiFi(); //

    // Setup Web Routes
    server.on("/", handleRoot);
    server.on("/on", handleLEDOn);
    server.on("/off", handleLEDOff);
    server.begin();
    
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}

void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); //
}
void handleRoot() {
    String s = "<html><head>";
    s += "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'>";
    
    s += "<script>";
    // ฟังก์ชันส่งคำสั่งลับๆ ไปหา ESP32
    s += "function ledOn() { fetch('/on'); }";
    s += "function ledOff() { fetch('/off'); }";
    
    // ป้องกันเมนูคลิกขวาเด้งขึ้นมาเวลากดค้างในมือถือ
    s += "document.addEventListener('contextmenu', event => event.preventDefault());";
    s += "</script>";

    s += "<style>";
    s += "body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }";
    s += ".momentary-btn { ";
    s += "  width: 200px; height: 200px; border-radius: 50%; border: none; ";
    s += "  background-color: red; color: white; font-size: 24px; font-weight: bold; ";
    s += "  box-shadow: 0 9px #999; cursor: pointer; user-select: none; -webkit-touch-callout: none; ";
    s += "}";
    s += ".momentary-btn:active { ";
    s += "  background-color: #3e8e41; box-shadow: 0 5px #666; transform: translateY(4px); "; // เปลี่ยนเป็นสีเขียวเมื่อกด
    s += "}";
    s += "</style>";

    s += "</head><body>";

    // ปุ่มกดค้าง: ใช้ทั้ง mousedown/up (คอม) และ touchstart/end (มือถือ)
    s += "<button class='momentary-btn' ";
    s += "onmousedown='ledOn()' onmouseup='ledOff()' ";
    s += "ontouchstart='ledOn()' ontouchend='ledOff()'>";
    s += "PUSH FOR LED";
    s += "</button>";

    s += "</body></html>";
    
    server.send(200, "text/html", s); //
}
void handleLEDOn() {
    digitalWrite(ledPin, HIGH);
    server.send(200, "text/plain", "OK"); // ส่งแค่คำว่า OK กลับไป
}

void handleLEDOff() {
    digitalWrite(ledPin, LOW);
    server.send(200, "text/plain", "OK");
}