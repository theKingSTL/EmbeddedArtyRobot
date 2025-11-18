#include "xil_printf.h"
#include "xgpio.h"
#include "sleep.h"

// ---------------------------------------------------------------------------
// BASE ADDRESSES — FROM YOUR VIVADO PROJECT
// ---------------------------------------------------------------------------
#define DHB1_GPIO_BASEADDR   0x44A10000  // Direction + Enable (GPIO)
#define MOTORFB_BASEADDR     0x44A20000  // Hall effect feedback (optional)
#define PWM_BASEADDR         0x44A30000  // PWM duty cycle + enable
#define LS1_BASEADDR         0x40020000  // From xparameters.h

// ---------------------------------------------------------------------------
// OPTION 1: If DHB1 GPIO is a SINGLE 32-bit register with bit fields
// ---------------------------------------------------------------------------
#define DHB1_GPIO_REG   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x00))

// Bit positions (adjust if your IP is different)
#define DIR1_BIT   0  // Motor 1 Direction
#define EN1_BIT    1  // Motor 1 Enable
#define DIR2_BIT   2  // Motor 2 Direction
#define EN2_BIT    3  // Motor 2 Enable

// ---------------------------------------------------------------------------
// OPTION 2: If using separate registers (original approach)
// ---------------------------------------------------------------------------
#define DHB1_DIR_M1   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x00))
#define DHB1_EN_M1    (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x04))
#define DHB1_DIR_M2   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x08))
#define DHB1_EN_M2    (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x0C))

// PWM registers
#define DHB1_PWM_M1   (*(volatile u32*)(PWM_BASEADDR + 0x00))
#define DHB1_PWM_M2   (*(volatile u32*)(PWM_BASEADDR + 0x04))
#define DHB1_PWM_EN   (*(volatile u32*)(PWM_BASEADDR + 0x08))

// ---------------------------------------------------------------------------
// LINE SENSOR GPIO
// ---------------------------------------------------------------------------
XGpio ls1_gpio;

// ---------------------------------------------------------------------------
// CHOOSE YOUR METHOD: 0 = separate registers, 1 = bit shifts
// ---------------------------------------------------------------------------
#define USE_BIT_SHIFTS 1  // Change to 0 if using separate registers

// ---------------------------------------------------------------------------
// REDUCED BUSY WAIT (was 90000, now 900)
// ---------------------------------------------------------------------------
void busy_wait_ms(int ms) {
    for (volatile int i = 0; i < ms * 900; i++)
        asm("nop");
}

// ---------------------------------------------------------------------------
// BIT MANIPULATION HELPERS
// ---------------------------------------------------------------------------
void set_bit(volatile u32* reg, int bit) {
    *reg |= (1 << bit);
}

void clear_bit(volatile u32* reg, int bit) {
    *reg &= ~(1 << bit);
}

// ---------------------------------------------------------------------------
// Motor control using BIT SHIFTS (OPTION 1)
// ---------------------------------------------------------------------------
#if USE_BIT_SHIFTS

void motor_forward() {
    clear_bit(&DHB1_GPIO_REG, DIR1_BIT);  // DIR1=0
    clear_bit(&DHB1_GPIO_REG, DIR2_BIT);  // DIR2=0
}

void motor_backward() {
    set_bit(&DHB1_GPIO_REG, DIR1_BIT);    // DIR1=1
    set_bit(&DHB1_GPIO_REG, DIR2_BIT);    // DIR2=1
}

void motor_left() {
    set_bit(&DHB1_GPIO_REG, DIR1_BIT);    // DIR1=1 (reverse)
    clear_bit(&DHB1_GPIO_REG, DIR2_BIT);  // DIR2=0 (forward)
}

void motor_right() {
    clear_bit(&DHB1_GPIO_REG, DIR1_BIT);  // DIR1=0 (forward)
    set_bit(&DHB1_GPIO_REG, DIR2_BIT);    // DIR2=1 (reverse)
}

void motor_enable() {
    set_bit(&DHB1_GPIO_REG, EN1_BIT);     // EN1=1
    set_bit(&DHB1_GPIO_REG, EN2_BIT);     // EN2=1
}

void motor_disable() {
    clear_bit(&DHB1_GPIO_REG, EN1_BIT);   // EN1=0
    clear_bit(&DHB1_GPIO_REG, EN2_BIT);   // EN2=0
}

// ---------------------------------------------------------------------------
// Motor control using SEPARATE REGISTERS (OPTION 2)
// ---------------------------------------------------------------------------
#else

void motor_forward() {
    DHB1_DIR_M1 = 0;  // Forward per truth table
    DHB1_DIR_M2 = 0;
}

void motor_backward() {
    DHB1_DIR_M1 = 1;  // Reverse per truth table
    DHB1_DIR_M2 = 1;
}

void motor_left() {
    DHB1_DIR_M1 = 1;  // M1 reverse
    DHB1_DIR_M2 = 0;  // M2 forward
}

void motor_right() {
    DHB1_DIR_M1 = 0;  // M1 forward
    DHB1_DIR_M2 = 1;  // M2 reverse
}

void motor_enable() {
    DHB1_EN_M1 = 1;
    DHB1_EN_M2 = 1;
}

void motor_disable() {
    DHB1_EN_M1 = 0;
    DHB1_EN_M2 = 0;
}

#endif

// ---------------------------------------------------------------------------
// Stop motors (PWM + enable) - PROPER SEQUENCE PER DOCUMENTATION
// ---------------------------------------------------------------------------
void motor_stop() {
    // Per H-Bridge note: disable EN BEFORE changing DIR
    motor_disable();      // EN=0 first
    busy_wait_ms(1);      // Small delay
    DHB1_PWM_M1 = 0;      // Turn off PWM
    DHB1_PWM_M2 = 0;
    DHB1_PWM_EN = 0;
}

// ---------------------------------------------------------------------------
// Set motor speed using PWM
// ---------------------------------------------------------------------------
void motor_set_speed(int duty) {
    DHB1_PWM_M1 = duty;
    DHB1_PWM_M2 = duty;
    DHB1_PWM_EN = 1;
    motor_enable();
}

// ---------------------------------------------------------------------------
// EASY MOTOR PULSE: turn on → wait → turn off
// ---------------------------------------------------------------------------
void motor_pulse_forward(int duty, int duration_ms) {
    motor_forward();
    motor_set_speed(duty);
    busy_wait_ms(duration_ms);
    motor_stop();
}

// ---------------------------------------------------------------------------
// DEBUG: Print GPIO register value
// ---------------------------------------------------------------------------
void debug_print_gpio() {
#if USE_BIT_SHIFTS
    xil_printf("GPIO Register: 0x%08X\n", DHB1_GPIO_REG);
    xil_printf("  DIR1=%d EN1=%d DIR2=%d EN2=%d\n",
               (DHB1_GPIO_REG >> DIR1_BIT) & 1,
               (DHB1_GPIO_REG >> EN1_BIT) & 1,
               (DHB1_GPIO_REG >> DIR2_BIT) & 1,
               (DHB1_GPIO_REG >> EN2_BIT) & 1);
#else
    xil_printf("DIR1=%d EN1=%d DIR2=%d EN2=%d\n",
               DHB1_DIR_M1, DHB1_EN_M1, DHB1_DIR_M2, DHB1_EN_M2);
#endif
}

// ---------------------------------------------------------------------------
// MAIN PROGRAM
// ---------------------------------------------------------------------------
int main() {
    xil_printf("ArtyBot starting...\n");
    xil_printf("Using %s\n", USE_BIT_SHIFTS ? "BIT SHIFTS" : "SEPARATE REGISTERS");

    // Initialize LS1 GPIO
    XGpio_Initialize(&ls1_gpio, LS1_BASEADDR);
    XGpio_SetDataDirection(&ls1_gpio, 1, 0xFF);  // inputs
    xil_printf("LS1 initialized.\n");

    // Ensure motors start disabled
    motor_stop();
    xil_printf("Motors stopped.\n");
    debug_print_gpio();

    // -------------------------------------------------------------
    // MOTOR TESTS
    // -------------------------------------------------------------
    xil_printf("\nPulse forward...\n");
    motor_pulse_forward(40000, 500);
    debug_print_gpio();

    xil_printf("\nPulse backward...\n");
    motor_backward();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();
    debug_print_gpio();

    xil_printf("\nPulse right turn...\n");
    motor_right();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();
    debug_print_gpio();

    xil_printf("\nPulse left turn...\n");
    motor_left();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();
    debug_print_gpio();

    xil_printf("\nEntering control loop...\n");

    // -------------------------------------------------------------
    // BASIC LINE-FOLLOWING LOOP
    // -------------------------------------------------------------
    while (1) {
        u32 ls = XGpio_DiscreteRead(&ls1_gpio, 1);
        int left = (ls & 0x01);
        int right = (ls & 0x02);

        if (left && right) {
            motor_forward();
            motor_set_speed(35000);
        } else if (left) {
            motor_left();
            motor_set_speed(30000);
        } else if (right) {
            motor_right();
            motor_set_speed(30000);
        } else {
            motor_stop();
        }

        usleep(20000);
    }

    return 0;
}
