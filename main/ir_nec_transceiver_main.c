#include "driver/rmt.h"
#include "esp_log.h"

#define RMT_TX_CHANNEL    RMT_CHANNEL_0  
#define RMT_TX_GPIO_NUM   GPIO_NUM_18    
#define RMT_CLK_DIV       80             
#define PULSE_HIGH_US     13            
#define PULSE_LOW_US      13             
#define TOTAL_DURATION_US 9000       
#define HIGH_DURATION_US  4500           
#define BIT_DURATION_0_US 560            
#define BIT_DURATION_1_US 1680           

static const char *TAG = "IR_Transmitter";

void send_signal(int command[], int num_of_bits) {
    int end_of_frame0 = BIT_DURATION_0_US / (PULSE_HIGH_US + PULSE_LOW_US);
    rmt_item32_t end_of_frame[end_of_frame0 + 1]; 

    // Configuração dos pulsos de 560 us de 38 kHz
    for (int i = 0; i < end_of_frame0; i++) {
        // LED IR ligado
        end_of_frame[i].level0 = 1;                
        // 13 µs nível alto
        end_of_frame[i].duration0 = PULSE_HIGH_US; 
        // LED IR desligado
        end_of_frame[i].level1 = 0;                
        // 13 µs nível baixo
        end_of_frame[i].duration1 = PULSE_LOW_US;  
    }

    // Define o item final da transmissão
    end_of_frame[end_of_frame0].level0 = 1;           
    end_of_frame[end_of_frame0].duration0 = 0;  
    end_of_frame[end_of_frame0].level1 = 1;           
    end_of_frame[end_of_frame0].duration1 = 0;

    // Lead code de 9ms de 38kHz seguido por 4.5ms em nível alto
    int num_pulses = TOTAL_DURATION_US / (PULSE_HIGH_US + PULSE_LOW_US);
    rmt_item32_t lead_code[num_pulses + 1];
    for (int i = 0; i < num_pulses; i++) {
        lead_code[i].level0 = 1;
        lead_code[i].duration0 = PULSE_HIGH_US;
        lead_code[i].level1 = 0;
        lead_code[i].duration1 = PULSE_LOW_US;
    }
    lead_code[num_pulses].level0 = 1;
    lead_code[num_pulses].duration0 = HIGH_DURATION_US;
    lead_code[num_pulses].level1 = 0;
    lead_code[num_pulses].duration1 = 0;

    // Envia o código de início (lead code)
    rmt_write_items(RMT_TX_CHANNEL, lead_code, num_pulses + 1, true);

    // Envia cada bit da sequência (0 ou 1)
    for (int i = 0; i < num_of_bits; i++) {
        int bit_duration = (command[i] == 1) ? BIT_DURATION_1_US : BIT_DURATION_0_US;
        int num_pulses_bit = BIT_DURATION_0_US / (PULSE_HIGH_US + PULSE_LOW_US);
        rmt_item32_t bit_pulse[num_pulses_bit + 1];
        for (int j = 0; j < num_pulses_bit; j++) {
            bit_pulse[j].level0 = 1;
            bit_pulse[j].duration0 = PULSE_HIGH_US;
            bit_pulse[j].level1 = 0;
            bit_pulse[j].duration1 = PULSE_LOW_US;
        }
        bit_pulse[num_pulses_bit].level0 = 1;
        bit_pulse[num_pulses_bit].duration0 = bit_duration;
        bit_pulse[num_pulses_bit].level1 = 0;
        bit_pulse[num_pulses_bit].duration1 = 0;

        // Envia o pulso do bit
        rmt_write_items(RMT_TX_CHANNEL, bit_pulse, num_pulses_bit + 1, true);
    }
    rmt_write_items(RMT_TX_CHANNEL, end_of_frame, end_of_frame0 + 1, true);
}

void app_main(void) {
    rmt_config_t rmt_tx_config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_TX_CHANNEL,
        .gpio_num = RMT_TX_GPIO_NUM,
        .clk_div = RMT_CLK_DIV,
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    rmt_config(&rmt_tx_config);
    rmt_driver_install(RMT_TX_CHANNEL, 0, 0);

    int command_sequence[] = {
        0,0,1,0,1,0,0,0,
        0,1,0,0,1,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        1,0,0,1
    };
    int num_of_bits = sizeof(command_sequence) / sizeof(command_sequence[0]);

    while (1) {
        ESP_LOGI(TAG, "Enviando sinal IR...");
        send_signal(command_sequence, num_of_bits);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
