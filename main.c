#include "xil_printf.h"
#include "xgpio.h"
#include "sleep.h"
#include "xparameters.h"

// ---------------------------------------------------------------------------
// BASE ADDRESSES — FROM YOUR VIVADO PROJECT
// ---------------------------------------------------------------------------
#define DHB1_GPIO_BASEADDR   0x44A10000  // Direction + Enable (GPIO)
#define MOTORFB_BASEADDR     0x44A20000  // Hall effect feedback (optional)
#define PWM_BASEADDR         0x44A30000  // PWM duty cycle + enable
#define LS1_BASEADDR         0x40020000  // From xparameters.h

// ---------------------------------------------------------------------------
// DHB1 REGISTER MAP
// ---------------------------------------------------------------------------
// Offsets may vary, but this matches standard UARK DHB1 IP structure:
#define DHB1_DIR_M1   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x00))
#define DHB1_DIR_M2   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x08))
#define DHB1_EN_M1    (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x04))
#define DHB1_EN_M2    (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x0C))

#define DHB1_PWM_M1   (*(volatile u32*)(PWM_BASEADDR + 0x00))
#define DHB1_PWM_M2   (*(volatile u32*)(PWM_BASEADDR + 0x04))
#define DHB1_PWM_EN   (*(volatile u32*)(PWM_BASEADDR + 0x08))

// ---------------------------------------------------------------------------
// LINE SENSOR GPIO (AXI GPIO with dual channels)
// channel 1 = LS1 sensor data
// ---------------------------------------------------------------------------
XGpio ls1_gpio;

// ---------------------------------------------------------------------------
// SIMPLE, RELIABLE MICROBLAZE DELAY (no OS timer needed)
// ---------------------------------------------------------------------------
void busy_wait_ms(int ms) {
    for (volatile int i = 0; i < ms * 900; i++)
        asm("nop");
}

// ---------------------------------------------------------------------------
// Motor direction helper functions
// ---------------------------------------------------------------------------
void motor_forward() {
    DHB1_DIR_M1 = 1;
    DHB1_DIR_M2 = 1;
}

void motor_backward() {
    DHB1_DIR_M1 = 0;
    DHB1_DIR_M2 = 0;
}

void motor_left() {
    DHB1_DIR_M1 = 0;
    DHB1_DIR_M2 = 1;
}

void motor_right() {
    DHB1_DIR_M1 = 1;
    DHB1_DIR_M2 = 0;
}

void motor_enable() {
    DHB1_EN_M1 = 1;
    DHB1_EN_M2 = 1;
}

void motor_disable() {
    DHB1_EN_M1 = 0;
    DHB1_EN_M2 = 0;
}

// ---------------------------------------------------------------------------
// Stop motors (PWM + enable)
// ---------------------------------------------------------------------------
void motor_stop() {
    DHB1_PWM_M1 = 0;
    DHB1_PWM_M2 = 0;
    DHB1_PWM_EN = 0;
    motor_disable();
}

// ---------------------------------------------------------------------------
// Set motor speed using PWM
// duty = 0–65535 range typically
// FIXED: No longer turns off motors immediately
// ---------------------------------------------------------------------------
void motor_set_speed(int duty, int duration) {
   
    xil_printf("beginning of set speed\n");

    DHB1_PWM_M1 = duty;
    DHB1_PWM_M2 = duty;
    xil_printf("end of duty\n");

    DHB1_PWM_EN = 0;
    motor_enable();

    busy_wait_ms(duration);

    xil_printf("end of set speed\n");

}

// ---------------------------------------------------------------------------
// EASY MOTOR PULSE: turn on → wait → turn off
// ---------------------------------------------------------------------------
void motor_pulse_forward(int duty, int duration) {
    motor_forward();
    motor_set_speed(duty, duration);
    motor_stop();
}

// ---------------------------------------------------------------------------
// MAIN PROGRAM
// ---------------------------------------------------------------------------
int main() {
    xil_printf("ArtyBot starting...\n");

    xil_printf("Init Pmod Ls1");
    XGpio_Initialize(&ls1_gpio, LS1_BASEADDR);

    

    // -------------------------------------------------------------
    // MOTOR TESTS
    // -------------------------------------------------------------
    xil_printf("Pulse forward...\n");
    motor_pulse_forward(400, 1000);  // 0.5 seconds

    xil_printf("Pulse backward...\n");
    motor_backward();
    motor_set_speed(400, 1000);
    busy_wait_ms(50);
    motor_stop();

    xil_printf("Pulse right turn...\n");
    motor_right();
    motor_set_speed(400, 1000);
    busy_wait_ms(50);
    motor_stop();

    xil_printf("Pulse left turn...\n");
    motor_left();
    motor_set_speed(400, 1000);
    busy_wait_ms(50);
    motor_stop();


    return 0;
}
