#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_exti.h"         
#include "gd32vf103_rcu.h"

// Define LED and Button pins
#define LED_PIN GPIO_PIN_7           // LED is connected to PA7
#define LED_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_8        // Button is connected to PB8
#define BUTTON_PORT GPIOB

int GAME_ON = 0;                      // Flag to control LED blinking loop
void toggle_LED(LED_ON){
    if(GAME_ON){
        GAME_ON = 0;
    }
    else{
        GAME_ON = 1;
    }
}
// === Simple software delay using NOP instructions ===
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 1000; ++i) {
        __NOP();  // No operation – burns CPU cycles
    }
}

int main(void) {


    // === GPIO Setup ===
    rcu_periph_clock_enable(RCU_GPIOB);                                    // Enable clock for GPIOB (button)
    rcu_periph_clock_enable(RCU_GPIOA);                                    // Enable clock for GPIOA (LED)

    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_PIN);     // Set PA7 as push-pull output (LED)
    gpio_init(BUTTON_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BUTTON_PIN);  // Set PB8 as input with pull-up (Button)

    gpio_bit_set(LED_PORT, LED_PIN);                                       // Turn LED on initially
    delay_ms(1000);                                                        // Wait 1 second before starting loop

    // === External Interrupt Configuration ===
    rcu_periph_clock_enable(RCU_AF);                                       // Enable alternate function clock (needed for EXTI mapping)
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_8);    // Map PB8 to EXTI1
    exti_init(EXTI_8, EXTI_INTERRUPT, EXTI_TRIG_FALLING);                  // Configure EXTI8 to trigger on falling edge (button press)
    exti_interrupt_flag_clear(EXTI_8);                                     // Clear any existing EXTI8 interrupt flag

    gpio_bit_set(LED_PORT, LED_PIN);  
    // === Main Program Loop ===
    while (1) {
        gpio_bit_set(LED_PORT, LED_PIN);       // Ensure LED is initially off
        if(exti_interrupt_flag_get(EXTI_8)){
            toggle_LED(GAME_ON);                        // Clear flag to stop LED blinking
            exti_interrupt_flag_clear(EXTI_8); // Clear interrupt pending flag
        }
        while (GAME_ON) {     
            if(exti_interrupt_flag_get(EXTI_8)){
                delay_ms(25);                      // Debounce delay
                toggle_LED(GAME_ON);               // Clear flag to stop LED blinking
                exti_interrupt_flag_clear(EXTI_8); // Clear interrupt pending flag
            }                                      // Blink LED as long as flag is set
            gpio_bit_reset(LED_PORT, LED_PIN);     // Turn LED off
            delay_ms(1000);
            gpio_bit_set(LED_PORT, LED_PIN);       
            delay_ms(1000);
        }

        // Once LED_ON is cleared by EXTI interrupt, blinking stops
        
    }
}