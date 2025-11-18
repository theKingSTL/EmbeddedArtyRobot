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
// DHB1 REGISTER MAP
// ---------------------------------------------------------------------------
// Offsets may vary, but this matches standard UARK DHB1 IP structure:
#define DHB1_DIR_M1   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x00))
#define DHB1_DIR_M2   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x04))
#define DHB1_EN_M1    (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x08))
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
void busy_wait_ms(int ms)
{
    for (volatile int i = 0; i < ms * 90000; i++)
        asm("nop");
}

// ---------------------------------------------------------------------------
// Motor direction helper functions
// ---------------------------------------------------------------------------
void motor_forward()
{
    DHB1_DIR_M1 = 1;
    DHB1_DIR_M2 = 1;
}

void motor_backward()
{
    DHB1_DIR_M1 = 0;
    DHB1_DIR_M2 = 0;
}

void motor_left()
{
    DHB1_DIR_M1 = 0;
    DHB1_DIR_M2 = 1;
}

void motor_right()
{
    DHB1_DIR_M1 = 1;
    DHB1_DIR_M2 = 0;
}

void motor_enable()
{
    DHB1_EN_M1 = 0;
    DHB1_EN_M2 = 1;
}

void motor_disable()
{
    DHB1_EN_M1 = 0;
    DHB1_EN_M2 = 0;
}

// ---------------------------------------------------------------------------
// Stop motors (PWM + enable)
// ---------------------------------------------------------------------------
void motor_stop()
{
    DHB1_PWM_M1 = 0;
    DHB1_PWM_M2 = 0;

    // disable outputs
    motor_disable();
}

// ---------------------------------------------------------------------------
// Set motor speed using PWM
// duty = 0–65535 range typically
// ---------------------------------------------------------------------------
void motor_set_speed(int duty)
{
    DHB1_PWM_M1 = duty;
    DHB1_PWM_M2 = duty;
    DHB1_PWM_EN = 1;

    motor_enable();

    busy_wait_ms(100);

    DHB1_PWM_M1 = 0;
    DHB1_PWM_M2 = 0;
    DHB1_PWM_EN = 0;

    motor_disable();
}

// ---------------------------------------------------------------------------
// EASY MOTOR PULSE: turn on → wait → turn off
// ---------------------------------------------------------------------------
void motor_pulse_forward(int duty, int duration_ms)
{
    motor_forward();
    motor_set_speed(duty);
    busy_wait_ms(duration_ms);
    motor_stop();
}

// ---------------------------------------------------------------------------
// MAIN PROGRAM
// ---------------------------------------------------------------------------
int main()
{
    xil_printf("ArtyBot starting...\n");

    // -------------------------------------------------------------
    // Initialize LS1 GPIO (using base address directly)
    // -------------------------------------------------------------
    XGpio_Initialize(&ls1_gpio, LS1_BASEADDR);
    XGpio_SetDataDirection(&ls1_gpio, 1, 0xFF); // inputs
    xil_printf("LS1 initialized.\n");

    // -------------------------------------------------------------
    // MOTOR TESTS
    // -------------------------------------------------------------
    xil_printf("Pulse forward...\n");
    motor_pulse_forward(40000, 500);   // 0.5 seconds

    xil_printf("Pulse backward...\n");
    motor_backward();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();

    xil_printf("Pulse right turn...\n");
    motor_right();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();

    xil_printf("Pulse left turn...\n");
    motor_left();
    motor_set_speed(35000);
    busy_wait_ms(500);
    motor_stop();

    xil_printf("Entering control loop...\n");

    // -------------------------------------------------------------
    // BASIC LINE-FOLLOWING LOOP
    // -------------------------------------------------------------
    while (1)
    {
        u32 ls = XGpio_DiscreteRead(&ls1_gpio, 1);

        int left  = (ls & 0x01);
        int right = (ls & 0x02);

        if (left && right)
        {
            motor_forward();
            motor_set_speed(35000);
        }
        else if (left)
        {
            motor_left();
            motor_set_speed(30000);
        }
        else if (right)
        {
            motor_right();
            motor_set_speed(30000);
        }
        else
        {
            motor_stop();
        }

        usleep(20000);
    }

    return 0;
}
