#include <Arduino.h>
#include <motor.h>    // Use < > because it is in the /lib folder
#include <config.h>   // Use " " because it is in the /include folder
#include <WiFi.h>
#include <WebServer.h>

// --- 1. Function Prototypes ---
void handleRoot();
void forward();
void backward();
void turnleft();
void turnright();
void stopRobot();
void connectWiFi();
void steering(int x, int omega);

// --- 2. Create Motor Objects ---
// Parameters: (Mode, Frequency, Bits, Invert, Brake, PWM_Pin, IN1, IN2)
Controller motorL(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, MOT_L_INV, true, MOT_L_PWM, MOT_L_IN1, MOT_L_IN2);
Controller motorR(Controller::PRIK_NO_ENA, PWM_FREQUENCY, PWM_BITS, MOT_R_INV, true, MOT_R_PWM, MOT_R_IN1, MOT_R_IN2);

WebServer server(80);

// กำหนดค่าเริ่มต้นให้เป็น 0 เพื่อความปลอดภัยตอนเปิดเครื่อง
int speed = 0; 
int r = 0;

void setup() {
    Serial.begin(115200);
    
    connectWiFi();

    // --- 3. Setup Web Routes ---
    // กำหนดเส้นทางว่าถ้าเว็บส่งคำสั่งอะไรมา ให้ไปเรียกฟังก์ชันไหนทำงาน
    server.on("/", handleRoot);
    server.on("/forward", forward);
    server.on("/backward", backward);
    server.on("/left", turnleft);
    server.on("/right", turnright);
    server.on("/stop", stopRobot);
    
    server.begin();
    
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();      // รอรับคำสั่งจากหน้าเว็บตลอดเวลา
    steering(speed, r);         // อัปเดตความเร็วมอเตอร์ตลอดเวลาตามค่า speed และ r
    Serial.println("\n Speed :"); 
    Serial.println(speed); 
    Serial.println("\n Rotaion :"); 
    Serial.println(r); 
}

void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); 
    //Serial.println("\n Speed :"); 
    //Serial.println(speed); 
    //Serial.println("\n Rotaion :"); 
    //Serial.println(r); 
}

// --- 4. Web Page HTML & UI ---
void handleRoot() {
    String s = "<html><head>";
    // บังคับสเกลหน้าเว็บไม่ให้ขยายแต่แรก
    s += "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>";
    
    s += "<script>";
    s += "function sendCmd(cmd) { fetch('/' + cmd); }";
    s += "document.addEventListener('contextmenu', event => event.preventDefault());";
    s += "</script>";

    s += "<style>";
    // ล็อกหน้าจอไม่ให้เลื่อนขยับได้
    s += "body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #222; overflow: hidden; overscroll-behavior: none; }";
    s += ".dpad { display: grid; grid-template-columns: repeat(3, 80px); grid-template-rows: repeat(3, 80px); gap: 10px; }";
    
    // ตรง touch-action: manipulation; และ touch-action: none; คือตัวป้องกันการซูมเมื่อกดย้ำๆ ครับ
    s += ".btn { background-color: #444; color: white; border: none; border-radius: 15px; font-size: 24px; font-weight: bold; cursor: pointer; user-select: none; -webkit-touch-callout: none; touch-action: none; }";
    
    s += ".btn:active { background-color: #00cc66; transform: scale(0.95); }";
    s += ".up { grid-column: 2; grid-row: 1; }";
    s += ".left { grid-column: 1; grid-row: 2; }";
    s += ".right { grid-column: 3; grid-row: 2; }";
    s += ".down { grid-column: 2; grid-row: 3; }";
    s += "</style>";
    s += "</head><body>";

    s += "<div class='dpad'>";
    s += "<button class='btn up' onmousedown=\"sendCmd('forward')\" onmouseup=\"sendCmd('stop')\" ontouchstart=\"sendCmd('forward')\" ontouchend=\"sendCmd('stop')\">W</button>";
    s += "<button class='btn left' onmousedown=\"sendCmd('left')\" onmouseup=\"sendCmd('stop')\" ontouchstart=\"sendCmd('left')\" ontouchend=\"sendCmd('stop')\">A</button>";
    s += "<button class='btn right' onmousedown=\"sendCmd('right')\" onmouseup=\"sendCmd('stop')\" ontouchstart=\"sendCmd('right')\" ontouchend=\"sendCmd('stop')\">D</button>";
    s += "<button class='btn down' onmousedown=\"sendCmd('backward')\" onmouseup=\"sendCmd('stop')\" ontouchstart=\"sendCmd('backward')\" ontouchend=\"sendCmd('stop')\">S</button>";
    s += "</div>";

    s += "</body></html>";
    
    server.send(200, "text/html", s);
}

// --- 5. Movement Functions ---
void forward() {
    speed = 511;
    r = 0;
    server.send(200, "text/plain", "OK");
}

void backward() {
    speed = -511;
    r = 0;
    server.send(200, "text/plain", "OK");
}

void turnleft() {
    speed = 0;
    r = -511;
    server.send(200, "text/plain", "OK");
}

void turnright() {
    speed = 0;
    r = 511;
    server.send(200, "text/plain", "OK");
}

void stopRobot() {
    speed = 0;
    r = 0;
    server.send(200, "text/plain", "OK");
}

// --- 6. Motor Control Function ---
void steering(int x, int omega) {
    motorL.spin(x + omega);
    motorR.spin(x - omega);
}