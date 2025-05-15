#include "gd32vf103.h"
#include "gd32vf103_gpio.h" 
#include "gd32vf103_timer.h"
#include "gd32vf103_rcu.h"
#include <stdlib.h>
#include <stdio.h>

// Define the game state
#define GAME_OFF 0
#define GAME_ON 1

// Define LED states
#define LED_OFF 0
#define LED_ON 1

// Game start status
int gameStatus = GAME_OFF;

// LED states
int yellowLEDState = LED_OFF;


// Number of lives remaining in the game
int playerLives = 3;

// Pin Definitions                   
#define START_BUTTON_PIN GPIO_PIN_1                   // Start button (PA1)
#define ACTION_BUTTON_PIN GPIO_PIN_2                  // Action button (PA2)
#define ERROR_LED_PIN GPIO_PIN_7                      // Error LED (PA7)
#define YELLOW_LED_PIN GPIO_PIN_13                    // Yellow LED (PC13)


// Toggle the game start state
void toggleGameStatus(uint8_t newStatus) {
    gameStatus = newStatus;
}

void configure_timer2(void) {
    // Configure TIMER2 for 1ms delay
    timer_parameter_struct timer2Config;
    timer2Config.prescaler = 107;  // 1 MHz (1 tick = 1 µs)
    timer2Config.alignedmode = TIMER_COUNTER_EDGE;
    timer2Config.counterdirection = TIMER_COUNTER_UP;
    timer2Config.period = 1000 - 1;  // 1000 ticks = 1 ms
    timer2Config.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER2, &timer2Config);
}

void delay_ms(uint32_t delayTime) {
    while (delayTime--) {
        timer_counter_value_config(TIMER2, 0);  // Reset the counter for each delay
        timer_enable(TIMER2);                    // Start the timer
        
        // Wait until the timer overflows (1ms has passed)
        while (timer_flag_get(TIMER2, TIMER_FLAG_UP) == RESET);  // Wait for overflow
        timer_flag_clear(TIMER2, TIMER_FLAG_UP);  // Clear the overflow flag
    }
}

// Check the reaction time and update lives if necessary
void checkReactionTime(uint32_t reactionTime) {
    if (reactionTime > 100000) {
        playerLives--;                        // penalty for slow reaction
        gpio_bit_reset(GPIOA, ERROR_LED_PIN);
        gpio_bit_set(GPIOA, ERROR_LED_PIN);   // turn on error light
        delay_ms(500);
        gpio_bit_reset(GPIOA, ERROR_LED_PIN); // turn off error light
    }
}

// Main game loop
int main(void) {
    // Enable clocks for GPIOA, GPIOC, and TIMER4
    rcu_periph_clock_enable(RCU_GPIOA);  // Clock for PA1, PA2
    rcu_periph_clock_enable(RCU_AF);     // Clock for alternate functions (if any)
    rcu_periph_clock_enable(RCU_TIMER4); // Clock for TIMER4 
    rcu_periph_clock_enable(RCU_TIMER3); // Clock for TIMER3

    // Initialize buttons as input
    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, START_BUTTON_PIN); // Start button (PA1)
    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, ACTION_BUTTON_PIN); // Action button (PA2)

    // Initialize LEDs as output
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, YELLOW_LED_PIN);  // Yellow LED (PC13)
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ERROR_LED_PIN);   // Error LED (PA7)

    // Configure TIMER3 for 1µs precision
    timer_deinit(TIMER3); // Reset TIMER3
    timer_parameter_struct timer3Config;
    timer3Config.prescaler = 107; 
    timer3Config.alignedmode = TIMER_COUNTER_EDGE;
    timer3Config.counterdirection = TIMER_COUNTER_UP;
    timer3Config.period = 1000000; 
    timer3Config.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER3, &timer3Config);
    
    // Configure TIMER2 for 1ms delay
    configure_timer2();  

    gpio_bit_reset(GPIOA, ERROR_LED_PIN);
    gpio_bit_set(GPIOA, ERROR_LED_PIN); // turn on error light indicate ready for start input
    // Main loop
    while (1) {
        while (gpio_input_bit_get(GPIOA, START_BUTTON_PIN) == 0); // Wait for press
        delay_ms(20); // Debounce
        while (gpio_input_bit_get(GPIOA, START_BUTTON_PIN) == 1); // Wait for release
        gpio_bit_reset(GPIOA, ERROR_LED_PIN); // turn off error light indicate input received
        toggleGameStatus(GAME_ON); // Start game

        // Game running
        while (gameStatus == GAME_ON) {
            if (playerLives == 0) {  // Check if the player has lost all lives
                toggleGameStatus(GAME_OFF);  // Turn off the game
                playerLives = 3;  // Reset lives to 3
            }

            int randomDelay = rand() % 1000 + 10000;  // Generate a random number between 1 and 2000ms

            uint32_t reactionTime = 0;  

            delay_ms(randomDelay + 1000);  // Random delay
                gpio_bit_reset(GPIOC, YELLOW_LED_PIN);
                gpio_bit_set(GPIOC, YELLOW_LED_PIN);     // Turn on yellow LED
                yellowLEDState = LED_ON;                 // Yellow LED is on
                timer_counter_value_config(TIMER3, 0);   // Reset TIMER4 to 0
                timer_enable(TIMER3);                    // Start TIMER4
                delay_ms(500);                           // LED stays on for 500ms
                gpio_bit_reset(GPIOC, YELLOW_LED_PIN);   // Turn off yellow LED
                
                // Wait for action button press
                while (yellowLEDState) {
                    while (gpio_input_bit_get(GPIOA, ACTION_BUTTON_PIN) == 0);
                    delay_ms(20);                 // Debounce delay
                    while (gpio_input_bit_get(GPIOA, ACTION_BUTTON_PIN) == 1); // Wait for release
                    reactionTime = timer_counter_read(TIMER3);
                    timer_disable(TIMER3);           // Stop the timer
                    yellowLEDState = LED_OFF;        // Stop waiting
                }

                checkReactionTime(reactionTime);
        }  
    }
    return 0;  // This line will never be reached
}