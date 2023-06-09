#include <stdio.h>

#include "pico/stdlib.h"
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/textRenderer/TextRenderer.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "images.h"
#include "zap.pio.h"

//Comment this line out if you want to use a cpu implementation of the rotary encoder.
#define USE_PIO_ROTARY
//The cpu implementation work well enough but the PIO implementation seems to be better.
//Fewer missed or duplicated inputs.

#ifdef USE_PIO_ROTARY
#include "rotary.pio.h"
#endif

//Rotary Encoder to set the length of the zapping.
#define ROTARY_CLK_PIN 20
#define ROTARY_DT_PIN 21
//Rotary switch currently not used.
#define ROTARY_SW_PIN 22

//Relay that controls the zapping.
#define RELAY_PIN 17

//Trigger button to start the zapping.
#define BUTTON_PIN 10

PIO zap_pio = pio0;
uint sm;

//Global variable for the pulse length of the zap.
uint32_t pulseLength = 550;

//Global variable for if zapping is currently happening.
bool zapping = false;
//Global variable for if the safety cool down is happening.
bool cooling = false;

//Inits the default i2c and pins for the display.
inline void initDisplay(){
    //May want to move this to updateDisplay somehow.
    i2c_init(i2c_default, 1000000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
}

//Clears and redraws the display based on global variables.
void updateDisplay(){
    static pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(i2c_default, 0x3C, pico_ssd1306::Size::W128xH64);

    display.clear();

    drawText(&display, font_8x8, "Microseconds", 0, 50);
    char buf[5];
    sprintf(buf, "%4u", pulseLength);
    drawText(&display, font_16x32, buf, 0, 10);

    if(zapping){
        display.addBitmapImage(88, 0, 40, 64, lightning);
    }
    if(cooling){
        display.addBitmapImage(84, 0, 48, 64, watch);
    }

    display.sendBuffer();
}

#ifdef USE_PIO_ROTARY
PIO rot_pio = pio1;
uint rot_sm;

//Callback function for up action from rotary pio.
void rotary_up_callback(){
    if(pulseLength < 1000) {
        pulseLength += 50;
        updateDisplay();
    }
    pio_interrupt_clear(rot_pio, 0);
}

//Callback function for down action from rotary pio.
void rotary_down_callback(){
    if(pulseLength > 50) {
        pulseLength -= 50;
        updateDisplay();
    }
    pio_interrupt_clear(rot_pio, 1);
}

inline void initRotaryPIO(){
    uint other_offset = pio_add_program(rot_pio, &rotary_program);
    rot_sm = pio_claim_unused_sm(rot_pio, true);

    //Enable irq interrupts for up and down actions.
    pio_set_irq0_source_enabled(rot_pio, pis_interrupt0, true);
    irq_add_shared_handler(PIO1_IRQ_0, rotary_up_callback, 0);
    irq_set_enabled(PIO1_IRQ_0, true);
    pio_set_irq1_source_enabled(rot_pio, pis_interrupt1, true);
    irq_add_shared_handler(PIO1_IRQ_1, rotary_down_callback, 0);
    irq_set_enabled(PIO1_IRQ_1, true);

    rotary_program_init(rot_pio, rot_sm, other_offset, ROTARY_CLK_PIN);
}
#endif

//Callback to end safety cool down.
int64_t de_cool_callback(alarm_id_t id, void *user_data){
    cooling = false;
    updateDisplay();
    return 0;
}

//Callback for after zapping is complete.
//Flips the zapping flag off, starts the cool down timer, updates the display.
void de_zap_callback(){
    cooling = true;
    zapping = false;
    updateDisplay();
    pio_interrupt_clear(zap_pio, 0);
    add_alarm_in_ms(1000, &de_cool_callback, nullptr, false);
}

//Callback function for the rotary encoder and trigger button.
void irq_callback(uint gpio, uint32_t event_mask){
#ifndef USE_PIO_ROTARY
    static bool dt_down = false;
    static bool clk_down = false;
#endif
    switch (gpio) {
#ifndef USE_PIO_ROTARY
        case ROTARY_CLK_PIN:
        case ROTARY_DT_PIN:
            if(event_mask == GPIO_IRQ_EDGE_RISE){
                dt_down = false;
                clk_down = false;
            } else {
                if( gpio == ROTARY_DT_PIN){
                    if(clk_down && !dt_down){
                        if(pulseLength < 1000) {
                            pulseLength += 50;
                            updateDisplay();
                        }
                    }
                    dt_down = true;
                } else {
                    if(dt_down && !clk_down){
                        if(pulseLength > 50) {
                            pulseLength -= 50;
                            updateDisplay();
                        }
                    }
                    clk_down = true;
                }
            }
            break;
#endif
        case BUTTON_PIN:
            if (zapping || cooling) break;
            zapping = true;
            updateDisplay();
            pio_sm_put_blocking(zap_pio, sm, 125000*pulseLength);
            break;
        default:
            printf("Da fuk you doing?\n");
    }
}

//Init a gpio input.
//Just reducing boilerplate code.
inline void initGPIOin(uint gpio, uint32_t event_mask){
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
    gpio_set_irq_enabled(gpio, event_mask, true);
}

//Init the gpio pins for the rotary encoder and the trigger button.
inline void initGPIO(){
#ifndef USE_PIO_ROTARY
    initGPIOin(ROTARY_CLK_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

    initGPIOin(ROTARY_DT_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
#endif

    initGPIOin(BUTTON_PIN, GPIO_IRQ_EDGE_FALL);

    gpio_set_irq_callback(&irq_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

//Init the PIO pins and statemachine to control the solid state relay.
inline void initPIO(){
    uint offset = pio_add_program(zap_pio, &zap_program);
    sm = pio_claim_unused_sm(zap_pio, true);
    pio_set_irq0_source_enabled(zap_pio, pis_interrupt0, true);
    irq_add_shared_handler(PIO0_IRQ_0, de_zap_callback, 0);
    irq_set_enabled(PIO0_IRQ_0, true);

    zap_program_init(zap_pio, sm, offset, RELAY_PIN);

#ifdef USE_PIO_ROTARY
    initRotaryPIO();
#endif
}



int main() {
    stdio_init_all();

    initGPIO();

    initDisplay();

    initPIO();

    updateDisplay();

    while(true) {
        sleep_ms(1000);
    }
}
