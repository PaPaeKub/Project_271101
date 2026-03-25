#ifndef ESP32_HARDWARE_H
#define ESP32_HARDWARE_H

#define PWM_Max 1023        
#define PWM_Min PWM_Max * -1

// --- Motor Settings ---
#define PWM_FREQUENCY  20000    // Speed of the motor signal 
#define PWM_BITS       10       // How many bits for speed (10 bits = 0 to 1023)

// --- Motor Control Pins ---
#define STBY      23       

// --- Left Motor Pins ---
#define PWMA      25       
#define AIN1      26       
#define AIN2      27       
#define AINV      false    // Set to 'true' to change direction

// --- Right Motor Pins ---
#define PWMB      14       
#define BIN1      16       
#define BIN2      17       
#define BINV      true     // Set to 'true' to change direction

// --- Servo Pins ---
#define INTAKE_PIN 18     
#define DROP_PIN   19     
#define DROP2_PIN 5 

// --- Sensor Pins (QRE1113 x 6) ---
#define SENSOR_1  36      
#define SENSOR_2  39      
#define SENSOR_3  34      
#define SENSOR_4  35      
#define SENSOR_5  32      
#define SENSOR_6  33      

// --- Sensor IR Control Pin ---
#define IR_PIN    4       

#endif