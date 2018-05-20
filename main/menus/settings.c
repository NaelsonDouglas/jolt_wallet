#include "menu8g2.h"
#include "../vault.h"
#include "submenus.h"
#include "../globals.h"
#include "../wifi.h"
#include "../loading.h"

static void get_serial_input(char *serial_rx, int buffersize){
    
    int line_pos = 0;
    
    while(1){
        int c = getchar();
        
        if(c < 0) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            continue;
        }
        if(c == '\n' || c == '\r') {
            
            // terminate the string
            serial_rx[line_pos] = '\0';
            printf("\n");
            break;
        }
        else {
            putchar(c);
            serial_rx[line_pos] = c;
            line_pos++;
            
            // buffer full!
            if(line_pos == buffersize) {
                
                printf("\nCommand buffer full!\n");
                serial_rx[line_pos] = '\0';
                
                break;
            }
        }
        
        
    }
}

static void flush_uart(){
    //This is a terrible hack to flush the uarts buffer, a far better option would be rewrite all uart related code
    // to use proper uart code from driver/uart.h
    for(int bad_hack = 0; bad_hack <= 10; bad_hack++){
        getchar();
    };
}

static void menu_factory_reset_confirm(menu8g2_t *prev){
    bool res;
    vault_rpc_t rpc;
    menu8g2_t menu;
    menu8g2_copy(&menu, prev);

    const char title[] = "Are you sure?";

    const char *options[] = {"No", "Yes"};
    res = menu8g2_create_simple(&menu, title, options, 2);

    if(res==false || menu.index==0){ //Pressed Back
        return;
    }

    rpc.type = FACTORY_RESET;
    vault_rpc(&rpc);
}

static void wifi_details(menu8g2_t *prev){
    bool res;
    menu8g2_t menu;
    menu8g2_copy(&menu, prev);

    char new_ap_info[45];
    get_ap_info(new_ap_info);
    for(;;){
        if(menu8g2_display_text(&menu, new_ap_info) == (1ULL << EASY_INPUT_BACK)){
            return;
        }
    }
}

static void wifi_update(menu8g2_t *prev){
    bool res;
    menu8g2_t menu;
    menu8g2_copy(&menu, prev);
    menu.post_draw = NULL;
    
    loading_enable();
    loading_text("Enter WiFi Credentials via UART");
    
    char wifi_ssid[32];
    flush_uart();
    
    printf("\nWiFi SSID: ");
    get_serial_input(wifi_ssid, sizeof(wifi_ssid));
    
    char wifi_pass[64];
    flush_uart();
    
    printf("\nWiFi Password: ");
    get_serial_input(wifi_pass, sizeof(wifi_pass));
    
    nvs_handle wifi_nvs_handle;
    init_nvm_namespace(&wifi_nvs_handle, "user");
    nvs_set_str(wifi_nvs_handle, "wifi_ssid", wifi_ssid);
    nvs_set_str(wifi_nvs_handle, "wifi_pass", wifi_pass);
    esp_err_t err = nvs_commit(wifi_nvs_handle);
    
    nvs_close(wifi_nvs_handle);
    
    loading_disable();
    
    if (err != ESP_OK) {
        menu8g2_display_text(&menu, "Error Updating WiFi Settings");
    }
    else {
        menu8g2_display_text(&menu, "Updated WiFi Settings - Click to Reset");
        esp_restart();
    }
}

void menu_settings(menu8g2_t *prev){
    menu8g2_t menu;
    menu8g2_copy(&menu, prev);
    const char title[] = "Settings";

    menu8g2_elements_t elements;
    menu8g2_elements_init(&elements, 5);
    menu8g2_set_element(&elements, "Screen Brightness", NULL);
    menu8g2_set_element(&elements, "WiFi Details", &wifi_details);
    menu8g2_set_element(&elements, "WiFi Update (uart)", &wifi_update);
    menu8g2_set_element(&elements, "Bluetooth", NULL);
    menu8g2_set_element(&elements, "Charge PoW", NULL);
    menu8g2_set_element(&elements, "Factory Reset", &menu_factory_reset_confirm);
    menu8g2_create_vertical_element_menu(&menu, title, &elements);
    menu8g2_elements_free(&elements);
}
