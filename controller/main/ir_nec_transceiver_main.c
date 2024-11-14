#include "driver/rmt.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define RMT_TX_CHANNEL    RMT_CHANNEL_0  
#define RMT_TX_GPIO_NUM   GPIO_NUM_18    
#define RMT_CLK_DIV       80             
#define PULSE_HIGH_US     13            
#define PULSE_LOW_US      13             
#define TOTAL_DURATION_US 9000       
#define HIGH_DURATION_US  4500           
#define BIT_DURATION_0_US 560            
#define BIT_DURATION_1_US 1680           

#define BUTTON_GPIO_NUM_1 GPIO_NUM_25 // Botão 1
#define BUTTON_GPIO_NUM_2 GPIO_NUM_26 // Botão 2
#define BUTTON_GPIO_NUM_3 GPIO_NUM_27 // Botão 3
#define BUTTON_GPIO_NUM_4 GPIO_NUM_14 // Botão 4

static const char *TAG = "IR_Transmitter";

int command_sequence[] = {
    0,0,1,0,1,0,0,0,
    0,1,0,0,1,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    1,0,0,1
};

// Inicializa o botão de entrada
void init_button(int gpio_num) {
    gpio_config_t io_conf;

    // Interrupção na borda de descida (pressionamento)
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}

// Verifica o estado do botão (pressionado ou não)
int is_button_pressed(int gpio_num) {
    // Pressionado retorna 0 (ativo em nível baixo)
    return gpio_get_level(gpio_num) == 0; 
}

// Função para ligar o ar-condicionado
void ligar_ar_condicionado() {
    int subarray[] = {1, 0, 0, 0}; 
    int subarray_size = sizeof(subarray) / sizeof(subarray[0]);
    replace_subarray(
        command_sequence, 
        sizeof(command_sequence) / sizeof(command_sequence[0]), 
        4, 7, subarray, subarray_size
    );
}

// Função para desligar o ar-condicionado
void desligar_ar_condicionado() {
    int subarray[] = {0, 0, 0, 0}; 
    int subarray_size = sizeof(subarray) / sizeof(subarray[0]);
    replace_subarray(
        command_sequence, 
        sizeof(command_sequence) / sizeof(command_sequence[0]), 
        4, 7, subarray, subarray_size
    );
}

// Função para aumentar a temperatura
void aumentar_temperatura() {
    int limit[] = {1, 0, 1, 1};
    int limit_length = sizeof(limit) / sizeof(limit[0]);
    if (!is_subarray_equal(command_sequence, 12, 15, limit, limit_length)) {
        add_to_bits(command_sequence, 12, 15, 1);
        subtract_from_bits(command_sequence, 40, 43, 1);
    }
}

// Função para diminuir a temperatura
void diminuir_temperatura() {
    int limit[] = {0, 0, 0, 0};
    int limit_length = sizeof(limit) / sizeof(limit[0]);
    if (!is_subarray_equal(command_sequence, 12, 15, limit, limit_length)) {
        subtract_from_bits(command_sequence, 12, 15, 1);
        add_to_bits(command_sequence, 40, 43, 1);
    }
}

void app_main(void) {
    // Configuração do transmissor IR
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

    // Inicializa os botões
    init_button(BUTTON_GPIO_NUM_1);
    init_button(BUTTON_GPIO_NUM_2);
    init_button(BUTTON_GPIO_NUM_3);
    init_button(BUTTON_GPIO_NUM_4);

    while (1) {
        ESP_LOGI(TAG, "Esperando comandos dos botões...");

        // Verifica se algum botão foi pressionado
        if (is_button_pressed(BUTTON_GPIO_NUM_1)) {
            ESP_LOGI(TAG, "Botão 1 pressionado: Ligando ar-condicionado.");
            ligar_ar_condicionado();
            send_signal(
                command_sequence, 
                sizeof(command_sequence) / sizeof(command_sequence[0])
            );
        }

        if (is_button_pressed(BUTTON_GPIO_NUM_2)) {
            ESP_LOGI(TAG, "Botão 2 pressionado: Desligando ar-condicionado.");
            desligar_ar_condicionado();
            send_signal(
                command_sequence, 
                sizeof(command_sequence) / sizeof(command_sequence[0])
            );
        }

        if (is_button_pressed(BUTTON_GPIO_NUM_3)) {
            ESP_LOGI(TAG, "Botão 3 pressionado: Aumentando temperatura.");
            aumentar_temperatura();
            send_signal(
                command_sequence, 
                sizeof(command_sequence) / sizeof(command_sequence[0])
            );
        }

        if (is_button_pressed(BUTTON_GPIO_NUM_4)) {
            ESP_LOGI(TAG, "Botão 4 pressionado: Diminuindo temperatura.");
            diminuir_temperatura();
            send_signal(
                command_sequence, 
                sizeof(command_sequence) / sizeof(command_sequence[0])
            );
        }

        // Aguarda 100 ms antes de verificar novamente
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}
