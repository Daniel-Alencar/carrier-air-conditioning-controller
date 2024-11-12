#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" 

// Pinos para UART2 
#define TXD_PIN (GPIO_NUM_17)  // TX2
#define RXD_PIN (UART_PIN_NO_CHANGE)  // RX2

void init_uart()
{
    // Configuração da UART2
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    // Configura a UART2 com os parâmetros definidos
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);
}

void send_ir_signal() {
    const uart_port_t uart_num = UART_NUM_2;
    // Envia a sequência de configuração
    const uint8_t init_sequence[] = {0xA1, 0xF1};
    uart_write_bytes(uart_num, (const char *)init_sequence, sizeof(init_sequence));
    // Pequeno atraso para garantir a configuração
    vTaskDelay(50 / portTICK_PERIOD_MS);  

    // Envia o comando de ligar o ar-condicionado
    const uint8_t ac_command[] = {0xC8, 0xF5, 0x00};
    uart_write_bytes(uart_num, (const char *)ac_command, sizeof(ac_command));

    ESP_LOGI("IR_SEND", "Sinal IR enviado");
}


void app_main()
{
    // Inicializa a UART2
    init_uart();
    
    while (1) {
        // Envia o sinal IR
        send_ir_signal();
        
        // Espera 1 segundo antes de enviar o próximo sinal
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}