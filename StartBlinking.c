#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define LED_PIN GPIO_PIN_7
#define LED_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_8
#define BUTTON_PORT GPIOB

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 1000; ++i) {
        __NOP();  // Simple delay loop
    }
}

int main(void) {
    // Enable clock for GPIOB
    rcu_periph_clock_enable(RCU_GPIOB);

    // Configure LED pin as push-pull output
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_PIN);

    // Configure Button pin as input with pull-up
    gpio_init(BUTTON_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BUTTON_PIN);
 
    while(1){
        int LED_ON = 1;
        gpio_bit_set(LED_PORT, LED_PIN);
        while(1) {
            // Check if button is pressed (active low)
            if (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN) == 0) {
                break;
            }
        }
        while(1){
            if(gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN) == 0){
                break;
            }
            gpio_bit_reset(LED_PORT, LED_PIN);
            delay_ms(15000);
            gpio_bit_set(LED_PORT, LED_PIN);
            delay_ms(15000);
        }
    }
    
    
        

        
    
}