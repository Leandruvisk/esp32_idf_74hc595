#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

/* ===== CONFIG CHIP ===== */
#define esp32_chip 1

#if esp32_chip
    #define PIN_DATA   12
    #define PIN_CLK    14
    #define PIN_LATCH  13
#endif

/* ===== DEFINIÇÕES DE SEGMENTOS =====
   bit0 → A
   bit1 → B
   bit2 → C
   bit3 → D
   bit4 → E
   bit5 → F
   bit6 → G
*/

#define DISPLAY_LEFT    0x0EFF
#define DISPLAY_RIGTH   0x0DFF

#define SEG1_A (1 << 0)
#define SEG1_B (1 << 1)
#define SEG1_C (1 << 2)
#define SEG1_D (1 << 3)
#define SEG1_E (1 << 4)
#define SEG1_F (1 << 5)
#define SEG1_G (1 << 6)
#define SEG1_N (1 << 14)

#define SEG2_A (1 << 7)
#define SEG2_B (1 << 8)
#define SEG2_C (1 << 9)
#define SEG2_D (1 << 10)
#define SEG2_E (1 << 11)
#define SEG2_F (1 << 12)
#define SEG2_G (1 << 13)
#define SEG2_N (1 << 15)

// #define SEG2_A (1 << 7)
// #define SEG2_B (1 << 8)
// #define SEG2_C (1 << 9)
// #define SEG2_D (1 << 10)
// #define SEG2_E (1 << 11)
// #define SEG2_F (1 << 12)
// #define SEG2_G (1 << 13)

/* ===== SHIFT ===== */

static inline void clk_pulse(void)
{
    gpio_set_level(PIN_CLK, 0);
    esp_rom_delay_us(12);
    gpio_set_level(PIN_CLK, 1);
    esp_rom_delay_us(12);
}

void shift16(uint16_t v)
{
    gpio_set_level(PIN_LATCH, 1);
    // vTaskDelay(1);
    // printf("\n");
    // printf("binario: ");
    // LSB first (como seu hardware exige)
    for (int i = 0; i < 16; i++) {
        if(i==8){
            esp_rom_delay_us(300);
            gpio_set_level(PIN_LATCH, 0);
        }

        int bit = (v >> (15-i)) & 1; // Pega o bit atual
        printf("%d", bit);      // Imprime 0 ou 1
        gpio_set_level(PIN_DATA, bit);
        clk_pulse();
    }
    // printf("\n");
}

/* ===== GERA PADRÃO DO DISPLAY ESQUERDO =====
   enable = 0x7F
   segmentos ativos em LOW
*/

uint16_t make_lef_right_segments(uint16_t display1, uint16_t seg_byte)
{

    if (display1 & SEG1_A) {
        seg_byte &= ~(1 << 7);
    }
    if (display1 & SEG1_B){
        seg_byte &= ~(1 << 6);
    } 
    if (display1 & SEG1_C){
        seg_byte &= ~(1 << 5);
    }

    if (display1 & SEG1_D){
        seg_byte &= ~(1 << 3);
    }

    if (display1 & SEG1_E){
        seg_byte &= ~(1 << 2);
    } 

    if (display1 & SEG1_F){
        seg_byte &= ~(1 << 1);
    } 
    if (display1 & SEG1_G){
        seg_byte &= ~(1 << 0);
    }

    // return ((uint16_t)seg_byte << 8) | 0x7F;
    return ((uint16_t)seg_byte);

}

/* ===== DÍGITOS 0–9 ===== */

uint8_t digit_to_segments_left(uint8_t d)
{
    switch (d) {
        case 0: return SEG1_A|SEG1_B|SEG1_C|SEG1_D|SEG1_E|SEG1_F;
        case 1: return SEG1_B|SEG1_C;
        case 2: return SEG1_A|SEG1_B|SEG1_D|SEG1_E|SEG1_G;
        case 3: return SEG1_A|SEG1_B|SEG1_C|SEG1_D|SEG1_G;
        case 4: return SEG1_B|SEG1_C|SEG1_F|SEG1_G;
        case 5: return SEG1_A|SEG1_C|SEG1_D|SEG1_F|SEG1_G;
        case 6: return SEG1_A|SEG1_C|SEG1_D|SEG1_E|SEG1_F|SEG1_G;
        case 7: return SEG1_A|SEG1_B|SEG1_C;
        case 8: return SEG1_A|SEG1_B|SEG1_C|SEG1_D|SEG1_E|SEG1_F|SEG1_G;
        case 9: return SEG1_A|SEG1_B|SEG1_C|SEG1_D|SEG1_F|SEG1_G;
        default: return 0;
    }
}


uint8_t digit_to_segments_right(uint8_t d)
{
    switch (d) {
        case 0: return SEG2_A|SEG2_B|SEG2_C|SEG2_D|SEG2_E|SEG2_F;
        case 1: return SEG2_B|SEG2_C;
        case 2: return SEG2_A|SEG2_B|SEG2_D|SEG2_E|SEG2_G;
        case 3: return SEG2_A|SEG2_B|SEG2_C|SEG2_D|SEG2_G;
        case 4: return SEG2_B|SEG2_C|SEG2_F|SEG2_G;
        case 5: return SEG2_A|SEG2_C|SEG2_D|SEG2_F|SEG2_G;
        case 6: return SEG2_A|SEG2_C|SEG2_D|SEG2_E|SEG2_F|SEG2_G;
        case 7: return SEG2_A|SEG2_B|SEG2_C;
        case 8: return SEG2_A|SEG2_B|SEG2_C|SEG2_D|SEG2_E|SEG2_F|SEG2_G;
        case 9: return SEG2_A|SEG2_B|SEG2_C|SEG2_D|SEG2_F|SEG2_G;
        default: return 0;
    }
}
/* ===== MAIN ===== */

void app_main(void)
{
    gpio_config_t io = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask =
            (1ULL << PIN_DATA) |
            (1ULL << PIN_CLK)  |
            (1ULL << PIN_LATCH),
    };
    gpio_config(&io);

    gpio_set_level(PIN_DATA, 0);
    gpio_set_level(PIN_CLK, 0);
    gpio_set_level(PIN_LATCH, 1);

    printf("\n=== TESTE DISPLAY ESQUERDO ===\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        // shift16(make_lef_right_segments(digit_to_segments_left(1), DISPLAY_LEFT));
        // vTaskDelay(100);
        // shift16(make_lef_right_segments(digit_to_segments_left(0), DISPLAY_RIGTH));
        // vTaskDelay(100);

        // for(uint16_t i =0; i< 16; i++){
        //     shift16(0x14 | (i<<8));
        //     vTaskDelay(100);
        //     for(uint16_t j =0; j< 16; j++){
        //         shift16(0x50 | (j<<8));
        //         vTaskDelay(100);

        //     }
        // }


        while(1){
            for(uint16_t i =0; i< 1000; i++){
                shift16(make_lef_right_segments(digit_to_segments_left(0x9), DISPLAY_LEFT));
                esp_rom_delay_us(100);
                shift16(make_lef_right_segments(digit_to_segments_left(0x6), DISPLAY_RIGTH));
                esp_rom_delay_us(100);
            }
            vTaskDelay(1);
            for(uint16_t i =0; i< 1000; i++){
                shift16(make_lef_right_segments(digit_to_segments_left(0x3), DISPLAY_LEFT));
                esp_rom_delay_us(100);
                shift16(make_lef_right_segments(digit_to_segments_left(0x4), DISPLAY_RIGTH));
                esp_rom_delay_us(100);
            }
        }
        
    }
}
