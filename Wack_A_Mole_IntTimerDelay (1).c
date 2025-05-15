#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_exti.h"         
#include "gd32vf103_rcu.h"
#include "gd32vf103_timer.h"
#include <stdlib.h>
#include <time.h>
// Define LED and Button pins
#define LED_PORT GPIOA
#define MOLE GPIO_PIN_7           // LED is connected to PA7
#define GREEN_LED GPIO_PIN_6      // LED is connected to PA6
#define LOSE_LED GPIO_PIN_5       // LED is connected to PA5
#define BUTTON_PORT GPIOB
#define BUTTON_PIN GPIO_PIN_8        // Button is connected to PB8

int GAME_ON = 1;
int LIVES = 3;
int WINS = 0;

int random_delay() {
    return (rand() % 9001) + 1000;  // Gives number between 1,000 and 10,000 inclusive
}

// === Software debounce delay ===
void delay_debounce(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 1000; ++i) {
        __NOP();  // burn time
    }
}

// === Timer2 setup for delay_ms ===
void configure_timer2(void) {
    timer_parameter_struct timer2Config;
    timer_deinit(TIMER2);
    timer2Config.prescaler = 107;
    timer2Config.alignedmode = TIMER_COUNTER_EDGE;
    timer2Config.counterdirection = TIMER_COUNTER_UP;
    timer2Config.period = 1000 - 1;
    timer2Config.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER2, &timer2Config);
    timer_enable(TIMER2);
}

// === Delay in milliseconds using Timer2 ===
void delay_ms(uint32_t ms) {
    while (ms--) {
        timer_counter_value_config(TIMER2, 0);
        while (timer_flag_get(TIMER2, TIMER_FLAG_UP) == RESET);
        timer_flag_clear(TIMER2, TIMER_FLAG_UP);
    }
}

int main(void) {

    srand(time(NULL));  // Seed with current time
    
    // === GPIO & RCU Setup ===
    rcu_periph_clock_enable(RCU_GPIOB); // For button
    rcu_periph_clock_enable(RCU_GPIOA); // For LED
    rcu_periph_clock_enable(RCU_AF);    // For EXTI
    rcu_periph_clock_enable(RCU_TIMER2);

    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, MOLE);              // Blue LED output
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GREEN_LED);         // Green LED output
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LOSE_LED);          // Red LED output     
    gpio_init(BUTTON_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BUTTON_PIN);        // Button input pull-up

    configure_timer2();

    // === Configure EXTI for PB8 ===
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_8);
    exti_init(EXTI_8, EXTI_INTERRUPT, EXTI_TRIG_FALLING);  // Falling edge
    exti_interrupt_flag_clear(EXTI_8);

    gpio_bit_reset(LED_PORT, MOLE);      // Ensure LED starts off
    gpio_bit_reset(LED_PORT, LOSE_LED);  // Ensure LED starts off
    gpio_bit_reset(LED_PORT, GREEN_LED); // Ensure LED starts off
    delay_ms(1000); // startup delay
    gpio_bit_set(LED_PORT, GREEN_LED); // game is about to start
    delay_ms(5000); // startup delay

    // === Game Loop ===
    while (1) {
        gpio_bit_reset(LED_PORT, GREEN_LED);
        if (GAME_ON) {
            int delay = random_delay();      // Random number 1,000 and 10,000
            delay_ms(delay);                 // Random delay before mole appears
            gpio_bit_set(LED_PORT, MOLE);    // Mole (LED) appears
            int hit = 0;

            // Reaction window = 3 seconds (3000 ms)
            for (int t = 0; t < 1000; t++) {
                delay_ms(1); // 1 ms tick

                // Check if button triggered the interrupt (polling flag)
                if (exti_interrupt_flag_get(EXTI_8)) {
                    delay_debounce(20); // debounce
                    if (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN) == RESET) {
                        hit = 1;
                        WINS++;
                        break;
                    }
                    exti_interrupt_flag_clear(EXTI_8); // Clear even if not valid press
                }
            }

            gpio_bit_reset(LED_PORT, MOLE); // Mole disappears
            delay_ms(1000); // Time between rounds

            if (!hit) {
                LIVES--;
            }

            // Game end logic
            if (WINS == 3) {
                GAME_ON = 0;
                for (int i = 0; i < 3; i++) {
                    gpio_bit_set(LED_PORT, GREEN_LED);
                    delay_ms(500);
                    gpio_bit_reset(LED_PORT, GREEN_LED);
                    delay_ms(500);
                }
            } else if (LIVES == 0) {
                GAME_ON = 0;
                for (int i = 0; i < 3; i++) {
                    gpio_bit_set(LED_PORT, LOSE_LED);
                    delay_ms(500);
                    gpio_bit_reset(LED_PORT, LOSE_LED);
                    delay_ms(500);
                }
            }

            // Clear any remaining interrupt flag
            exti_interrupt_flag_clear(EXTI_8);
        }
    }
}