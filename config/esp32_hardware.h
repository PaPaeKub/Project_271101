#ifndef ESP32_HARDWARE_H
#define ESP32_HARDWARE_H

#define PWM_Max 1023
#define PWM_Min PWM_Max * -1

// --- Motor Settings ---
#define PWM_FREQUENCY  20000    // Speed of the motor signal (20kHz is quiet)
#define PWM_BITS       10       // How many bits for speed (10 bits = 0 to 1023)
#define PWM_MAX        1023     // Maximum speed value

// --- Left Motor Pins ---
#define PWMA      25       // Speed pin for Left motor
#define AIN1      26       // Direction pin 1
#define AIN2      27       // Direction pin 2
#define AINV      false    // Set to 'true' to change direction

// --- Right Motor Pins ---
#define PWMB      14       // Speed pin for Right motor
#define BIN1      12       // Direction pin 1
#define BIN2      13       // Direction pin 2
#define BINV      true     // Set to 'true' to change direction

// --- Servo Pins ---
#define INTAKE_PIN 17      // Pin for Intake Servo (MG996R 360 degree)

// --- Sensor Pins ---

#endif