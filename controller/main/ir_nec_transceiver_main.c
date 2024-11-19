#include <math.h>
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

// Função para converter um grupo de bits para um número inteiro
int bits_to_int(int *bits, int size) {
    int result = 0;
    for (int i = 0; i < size; i++) {
        result += bits[i] * (int)pow(2, size - 1 - i);
    }
    return result;
}

// Função para converter um número inteiro para bits
void int_to_bits(int value, int *bits, int size) {
    for (int i = size - 1; i >= 0; i--) {
        bits[i] = value % 2;
        value /= 2;
    }
}

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

void replace_subarray(
    int *main_array, int main_size, 
    int start_index, int end_index, 
    int *subarray, int sub_size
) {
    // Verifica se os índices são válidos
    if (
        start_index < 0 || end_index >= main_size || 
        end_index - start_index + 1 != sub_size
    ) {
        printf("Erro: Tamanho ou índices inválidos para substituição.\n");
        return;
    }

    // Substitui os valores do trecho pelo subarray
    for (int i = 0; i < sub_size; i++) {
        main_array[start_index + i] = subarray[i];
    }
}

// Função para verificar se um trecho do array é igual a um subarray
int is_subarray_equal(
    int *array, int start, int end, 
    int *subarray, int subarray_length
) {
    // Verifica se os tamanhos dos segmentos são compatíveis
    if ((end - start + 1) != subarray_length) {
        // Tamanhos incompatíveis
        return 0; 
    }
    // Compara os elementos um a um
    for (int i = 0; i < subarray_length; i++) {
        if (array[start + i] != subarray[i]) {
            // Um elemento é diferente
            return 0; 
        }
    }
    // Todos os elementos são iguais
    return 1; 
}

// Função para somar valor a um grupo de bits em um array
void add_to_bits(
    int *sequence, 
    int start_index, 
    int end_index, 
    int value_to_add
) {
    int size = end_index - start_index + 1;
    int current_value = bits_to_int(&sequence[start_index], size);
    int new_value = current_value + value_to_add;
    int_to_bits(new_value, &sequence[start_index], size);
}

// Função para subtrair valor de um grupo de bits em um array
void subtract_from_bits(
    int *sequence, 
    int start_index, 
    int end_index, 
    int value_to_subtract
) {
    int size = end_index - start_index + 1;
    int current_value = bits_to_int(&sequence[start_index], size);
    int new_value = current_value - value_to_subtract;

    // Garante que o valor não seja negativo
    if (new_value < 0) new_value = 0; 
    int_to_bits(new_value, &sequence[start_index], size);
}

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
