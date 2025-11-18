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
// Try BOTH addressing methods - we'll test which one works!
// ---------------------------------------------------------------------------
// Method 1: Individual registers at different offsets
#define DHB1_REG_0   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x00))
#define DHB1_REG_4   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x04))
#define DHB1_REG_8   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x08))
#define DHB1_REG_C   (*(volatile u32*)(DHB1_GPIO_BASEADDR + 0x0C))

// Method 2: PWM registers
#define PWM_REG_0   (*(volatile u32*)(PWM_BASEADDR + 0x00))
#define PWM_REG_4   (*(volatile u32*)(PWM_BASEADDR + 0x04))
#define PWM_REG_8   (*(volatile u32*)(PWM_BASEADDR + 0x08))

// LINE SENSOR GPIO
XGpio ls1_gpio;

// ---------------------------------------------------------------------------
// Simple delay
// ---------------------------------------------------------------------------
void busy_wait_ms(int ms) {
    for (volatile int i = 0; i < ms * 90000; i++)  // Back to 90000 for now
        asm("nop");
}

// ---------------------------------------------------------------------------
// RAW REGISTER TEST - Let's see what the hardware actually does!
// ---------------------------------------------------------------------------
void test_raw_registers() {
    xil_printf("\n=== RAW REGISTER TEST ===\n");
    
    // Read initial values
    xil_printf("Initial DHB1 values:\n");
    xil_printf("  [0x00] = 0x%08X\n", DHB1_REG_0);
    xil_printf("  [0x04] = 0x%08X\n", DHB1_REG_4);
    xil_printf("  [0x08] = 0x%08X\n", DHB1_REG_8);
    xil_printf("  [0x0C] = 0x%08X\n", DHB1_REG_C);
    
    xil_printf("\nInitial PWM values:\n");
    xil_printf("  [0x00] = 0x%08X\n", PWM_REG_0);
    xil_printf("  [0x04] = 0x%08X\n", PWM_REG_4);
    xil_printf("  [0x08] = 0x%08X\n", PWM_REG_8);
    
    // Test writing to each register
    xil_printf("\n--- Writing 0x00000001 to DHB1[0x00] ---\n");
    DHB1_REG_0 = 0x00000001;
    xil_printf("  Result: 0x%08X\n", DHB1_REG_0);
    busy_wait_ms(500);
    
    xil_printf("\n--- Writing 0x00000001 to DHB1[0x04] ---\n");
    DHB1_REG_4 = 0x00000001;
    xil_printf("  Result: 0x%08X\n", DHB1_REG_4);
    busy_wait_ms(500);
    
    xil_printf("\n--- Writing 0x00000001 to DHB1[0x08] ---\n");
    DHB1_REG_8 = 0x00000001;
    xil_printf("  Result: 0x%08X\n", DHB1_REG_8);
    busy_wait_ms(500);
    
    xil_printf("\n--- Writing 0x00000001 to DHB1[0x0C] ---\n");
    DHB1_REG_C = 0x00000001;
    xil_printf("  Result: 0x%08X\n", DHB1_REG_C);
    busy_wait_ms(500);
    
    // Clear all
    xil_printf("\n--- Clearing all DHB1 registers ---\n");
    DHB1_REG_0 = 0;
    DHB1_REG_4 = 0;
    DHB1_REG_8 = 0;
    DHB1_REG_C = 0;
    
    // Test PWM
    xil_printf("\n--- Testing PWM registers ---\n");
    xil_printf("Setting PWM_REG_0 = 40000...\n");
    PWM_REG_0 = 40000;
    xil_printf("  Result: 0x%08X (%d)\n", PWM_REG_0, PWM_REG_0);
    
    xil_printf("Setting PWM_REG_4 = 40000...\n");
    PWM_REG_4 = 40000;
    xil_printf("  Result: 0x%08X (%d)\n", PWM_REG_4, PWM_REG_4);
    
    xil_printf("Setting PWM_REG_8 = 1 (enable)...\n");
    PWM_REG_8 = 1;
    xil_printf("  Result: 0x%08X\n", PWM_REG_8);
    
    busy_wait_ms(1000);
    
    xil_printf("\nClearing PWM...\n");
    PWM_REG_0 = 0;
    PWM_REG_4 = 0;
    PWM_REG_8 = 0;
    
    xil_printf("\n=== END RAW REGISTER TEST ===\n\n");
}

// ---------------------------------------------------------------------------
// SIMPLE MOTOR TEST - Based on your ORIGINAL working code
// ---------------------------------------------------------------------------
void simple_motor_test() {
    xil_printf("\n=== SIMPLE MOTOR TEST (Original Method) ===\n");
    
    // Test 1: Forward
    xil_printf("\nTest 1: FORWARD\n");
    xil_printf("  Setting DIR1=1, DIR2=1\n");
    DHB1_REG_0 = 1;  // DIR_M1
    DHB1_REG_4 = 1;  // DIR_M2
    
    xil_printf("  Setting EN1=1, EN2=1\n");
    DHB1_REG_8 = 1;  // EN_M1
    DHB1_REG_C = 1;  // EN_M2
    
    xil_printf("  Setting PWM=40000\n");
    PWM_REG_0 = 40000;  // PWM_M1
    PWM_REG_4 = 40000;  // PWM_M2
    PWM_REG_8 = 1;      // PWM_EN
    
    xil_printf("  Motors should run for 2 seconds...\n");
    busy_wait_ms(2000);
    
    xil_printf("  Stopping...\n");
    PWM_REG_0 = 0;
    PWM_REG_4 = 0;
    PWM_REG_8 = 0;
    DHB1_REG_8 = 0;
    DHB1_REG_C = 0;
    
    busy_wait_ms(1000);
    
    // Test 2: Backward
    xil_printf("\nTest 2: BACKWARD\n");
    xil_printf("  Setting DIR1=0, DIR2=0\n");
    DHB1_REG_0 = 0;  // DIR_M1
    DHB1_REG_4 = 0;  // DIR_M2
    
    xil_printf("  Setting EN1=1, EN2=1\n");
    DHB1_REG_8 = 1;  // EN_M1
    DHB1_REG_C = 1;  // EN_M2
    
    xil_printf("  Setting PWM=40000\n");
    PWM_REG_0 = 40000;
    PWM_REG_4 = 40000;
    PWM_REG_8 = 1;
    
    xil_printf("  Motors should run for 2 seconds...\n");
    busy_wait_ms(2000);
    
    xil_printf("  Stopping...\n");
    PWM_REG_0 = 0;
    PWM_REG_4 = 0;
    PWM_REG_8 = 0;
    DHB1_REG_8 = 0;
    DHB1_REG_C = 0;
    
    xil_printf("\n=== END SIMPLE MOTOR TEST ===\n\n");
}

// ---------------------------------------------------------------------------
// Check if using AXI GPIO instead
// ---------------------------------------------------------------------------
void test_axi_gpio() {
    xil_printf("\n=== TESTING AXI GPIO METHOD ===\n");
    
    XGpio dhb1_gpio;
    int status;
    
    // Try to initialize GPIO at DHB1_GPIO_BASEADDR
    status = XGpio_Initialize(&dhb1_gpio, DHB1_GPIO_BASEADDR);
    
    if (status == XST_SUCCESS) {
        xil_printf("AXI GPIO initialized successfully!\n");
        xil_printf("Trying to control motors via XGpio functions...\n");
        
        // Set as outputs
        XGpio_SetDataDirection(&dhb1_gpio, 1, 0x00);
        
        // Try setting all pins high
        xil_printf("Setting GPIO channel 1 = 0xFF\n");
        XGpio_DiscreteWrite(&dhb1_gpio, 1, 0xFF);
        u32 val = XGpio_DiscreteRead(&dhb1_gpio, 1);
        xil_printf("  Read back: 0x%08X\n", val);
        
        busy_wait_ms(2000);
        
        xil_printf("Setting GPIO channel 1 = 0x00\n");
        XGpio_DiscreteWrite(&dhb1_gpio, 1, 0x00);
        val = XGpio_DiscreteRead(&dhb1_gpio, 1);
        xil_printf("  Read back: 0x%08X\n", val);
        
    } else {
        xil_printf("AXI GPIO initialization failed (status=%d)\n", status);
        xil_printf("This means DHB1 is NOT using standard AXI GPIO.\n");
    }
    
    xil_printf("\n=== END AXI GPIO TEST ===\n\n");
}

// ---------------------------------------------------------------------------
// MAIN PROGRAM
// ---------------------------------------------------------------------------
int main() {
    xil_printf("\n\n");
    xil_printf("====================================\n");
    xil_printf("    ArtyBot DIAGNOSTIC MODE\n");
    xil_printf("====================================\n");
    
    // Initialize line sensor
    XGpio_Initialize(&ls1_gpio, LS1_BASEADDR);
    XGpio_SetDataDirection(&ls1_gpio, 1, 0xFF);
    xil_printf("LS1 initialized.\n");
    
    // Run all diagnostic tests
    xil_printf("\nStarting diagnostics in 2 seconds...\n");
    busy_wait_ms(2000);
    
    // Test 1: Raw register access
    test_raw_registers();
    busy_wait_ms(2000);
    
    // Test 2: Simple motor control (your original working method)
    simple_motor_test();
    busy_wait_ms(2000);
    
    // Test 3: Check if it's AXI GPIO
    test_axi_gpio();
    
    xil_printf("\n====================================\n");
    xil_printf("    DIAGNOSTICS COMPLETE\n");
    xil_printf("====================================\n");
    xil_printf("\nWatch the motors and report:\n");
    xil_printf("1. Did motors move during any test?\n");
    xil_printf("2. What values did registers read back?\n");
    xil_printf("3. Did AXI GPIO init succeed?\n");
    
    // Infinite loop
    while(1) {
        busy_wait_ms(1000);
    }
    
    return 0;
}
