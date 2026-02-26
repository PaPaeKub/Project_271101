#ifndef ESP32_HARDWARE_H
#define ESP32_HARDWARE_H

// --- Motor Settings ---
#define PWM_FREQUENCY  20000    // Speed of the motor signal (20kHz is quiet)
#define PWM_BITS       10       // How many bits for speed (10 bits = 0 to 1023)
#define PWM_MAX        1023     // Maximum speed value

// --- Left Motor Pins ---
#define MOT_L_PWM      14       // Speed pin for Left motor
#define MOT_L_IN1      12       // Direction pin 1
#define MOT_L_IN2      13       // Direction pin 2
#define MOT_L_INV      false    // Set to 'true' to change direction

// --- Right Motor Pins ---
#define MOT_R_PWM      32       // Speed pin for Right motor
#define MOT_R_IN1      25       // Direction pin 1
#define MOT_R_IN2      26       // Direction pin 2
#define MOT_R_INV      true     // Set to 'true' to change direction

// --- Servo Pins ---
#define SERVO_1_PIN    18       // Pin for Servo 1
#define SERVO_2_PIN    19       // Pin for Servo 2
#define SERVO_3_PIN    21       // Pin for Servo 3

// --- Sensor Pins ---

#endif