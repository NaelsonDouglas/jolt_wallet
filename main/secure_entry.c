#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_system.h>
#include "sodium.h"
#include "easy_input.h"
#include "menu8g2.h"
#include "u8g2.h"
#include "vault.h"
#include "secure_entry.h"
#include "helpers.h"
#include "statusbar.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"


#define PIN_SPACING 13

bool pin_entry(menu8g2_t *prev, unsigned char *pin_hash, const char *title){
    /* Screen for Pin Entry 
     * Saves results into pin_entries*/
    menu8g2_t local_menu;
    menu8g2_t *menu = &local_menu;
    menu8g2_copy(menu, prev);

    u8g2_t *u8g2 = menu->u8g2;
    uint8_t max_pos = MAX_PIN_DIGITS - 1;
    u8g2_SetFont(u8g2, u8g2_font_profont12_tf);
    u8g2_uint_t title_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) +
            CONFIG_MENU8G2_BORDER_SIZE;
    u8g2_SetFont(u8g2, u8g2_font_profont17_tf);
    u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) +
            CONFIG_MENU8G2_BORDER_SIZE;
    int8_t entry_pos = 0; // which element the user is currently entering
    uint8_t border = (u8g2_GetDisplayWidth(u8g2) - 
            (PIN_SPACING * max_pos)) / 2;

	uint64_t input_buf;
    char buf[24];
    int8_t pin_entries[MAX_PIN_DIGITS] = { 0 };
    statusbar_disable(menu);
    for(;;){
        MENU8G2_BEGIN_DRAW(menu)
            u8g2_SetFont(u8g2, u8g2_font_profont12_tf);
            u8g2_DrawStr(u8g2, get_center_x(u8g2, title), title_height,
                    title);
            u8g2_DrawHLine(u8g2, 0, line_height, u8g2_GetDisplayWidth(u8g2));
            u8g2_SetFont(u8g2, u8g2_font_profont17_tf);

            for(int i = 0; i < max_pos; i++){
                // Set Background color for position selection
                if(i==entry_pos){
                    u8g2_SetDrawColor(u8g2, 0);
                }
                else{
                    u8g2_SetDrawColor(u8g2, 1);
                }
                sprintf(buf, "%d", pin_entries[i]);
                u8g2_DrawStr(u8g2, border + (i * PIN_SPACING),
                        (u8g2_GetDisplayHeight(u8g2) + line_height)/2 ,
                        buf);
            }
        MENU8G2_END_DRAW(menu)

        u8g2_SetFont(u8g2, u8g2_font_profont12_tf);
        u8g2_SetDrawColor(u8g2, 1); // Set it back to default background

		if(xQueueReceive(menu->input_queue, &input_buf, portMAX_DELAY)) {
            if(input_buf & (1ULL << EASY_INPUT_BACK)){
                if(entry_pos>0){
                    entry_pos--;
                }
                else{
                    statusbar_enable(menu);
                    return false;
                }
            }
            else if(input_buf & (1ULL << EASY_INPUT_UP)){
                if(++pin_entries[entry_pos]>9){
                    pin_entries[entry_pos] = 0;
                }

            }
            else if(input_buf & (1ULL << EASY_INPUT_DOWN)){
                if(--pin_entries[entry_pos]<0){
                    pin_entries[entry_pos] = 9;
                }
            }
            else if(input_buf & (1ULL << EASY_INPUT_ENTER)){
                if(entry_pos < max_pos-1){
                    entry_pos++;
                }
                else{
                    // Convert pin into a 256-bit key
                    crypto_generichash_blake2b_state hs;
                    crypto_generichash_init(&hs, NULL, 32, 32);
                    crypto_generichash_update(&hs, 
                            (unsigned char *) pin_entries, MAX_PIN_DIGITS);
                    crypto_generichash_final(&hs, pin_hash, 32);

                    statusbar_enable(menu);
                    return true;
                }
            }
        }
    }
}

