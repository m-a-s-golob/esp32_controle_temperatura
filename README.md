----------------------------------------------------------------------------------

Projeto:
Sitema para Controle de Temperatura com ESP32

----------------------------------------------------------------------------------

Autor:
Marcos Augusto Soares Golob - RA: 130003180

----------------------------------------------------------------------------------

Professor:
Eng. Esp. Fábio Bezerra de Souza

----------------------------------------------------------------------------------

Disciplina:
Sistemas Operacionais e RTOS de Sistemas Embarcados

----------------------------------------------------------------------------------

Instituição:
Centro Universitário Salesiano de São Paulo - UNISAL

----------------------------------------------------------------------------------

Introdução:
O projeto consiste em um sistema para controle automático de temperatura, em que o valor de set point da temperatura é definido por um usuário através de um teclado e exibido em um display de LED de 7 segmentos. A partir do valor de temperatura medido na forma de sinal analógico e do set point de temperatura, o dispositivo faz o acionamento de um refrigerador, para que a temperatura do sistema fique abaixo do valor definido.

----------------------------------------------------------------------------------

Materiais e Recursos:
1x Módulo Espressif ESP32-WROOM-32
1x Teclado matricial 4x4
1x Display LED de 7 segmentos, vermelho e de catodo comum
7x Resistores 220Ω
1x Potenciômetro 5kΩ

----------------------------------------------------------------------------------

Explicação do Código Fonte:

Inicialmente foram incluídas as bibliotecas necessárias:

    #include <stdio.h>
    #include "driver/gpio.h"
    #include <driver/adc.h>
    #include "sdkconfig.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/timers.h"
    #include "freertos/semphr.h"

Foram declaradores os handlers para tratamento das tarefas e do semáforo criados posteriormente:

    TaskHandle_t xTask1Handle, xTask2Handle, xTask3Handle, xTask4Handle, xTask5Handle;
    SemaphoreHandle_t xSemaphore1;

    Foram então definidos os pinos de entrada e saída, referentes às linhas e colunas do teclado matricial, aos segmentos do display LED e à saída do sistema, e então uma macro de apoio:

    #define L1    4
    #define L2    16
    #define L3    17
    #define L4    5
    #define C1    18
    #define C2    19
    #define C3    22
    #define C5    23
    #define SEG_A 21
    #define SEG_B 27
    #define SEG_C 32
    #define SEG_D 33
    #define SEG_E 25
    #define SEG_F 26
    #define SEG_G 2
    #define OUT   0
    #define tst_bit(Y,bit_x)  (Y & (1<<bit_x))

Em seguida foram criadas as variáveis globais do código, incluindo a matriz para o mapeamento das teclas do teclado e o vetor para acionamento do display LED:

    uint32_t adc = 0;
    uint32_t temperature = 0;
    uint8_t set_point = 0;
    uint8_t row_size = 4;
    uint8_t col_size = 4;
    uint8_t r = 0;
    uint8_t c = 0;
    uint8_t row_pins[4] = {4,16,17,5};
    uint8_t col_pins[4] = {18,19,22,23};
    char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}};
    char new = 0;
    char display[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};
    char dez = '0';
    char un = '0';
    char system_state[3] = {'0','0','0'};
    uint8_t cont = 0;

Antes da função principal, foram incluídos os protótipos das cinco tasks utilizadas no código. As tasks foram criadas para leitura analógica da temperatura, para varredura e identificação das teclas do teclado matricial, para impressão na tela das informações do sistema, para indicação do display LED de 7 segmentos e para o controle da saída do sistema:

    void task_temperature(void *pvParameters);
    void task_keyboard(void *pvParameters);
    void task_print(void *pvParameters);
    void task_display_7seg(void *pvParameters);
    void task_control(void *pvParameters);

No início da função main foram definidos os sentidos e níveis dos pinos de entrada e saída. Inicialmente todas os pinos conectados ao teclado matricial foram configurados como entrada:

    void app_main(void)
    { 
    for(uint8_t r = 0; r < row_size; r++)
    {
        gpio_set_direction(row_pins[r], GPIO_MODE_INPUT);
        gpio_set_pull_mode(row_pins[r], GPIO_PULLUP_ONLY);
    }
    
    for(uint8_t c = 0; c < col_size; c++)
    {
        gpio_set_direction(col_pins[c], GPIO_MODE_INPUT);
    }

Os pinos conectados ao display LED foram configurados como saída e definidos inicialmente em nível baixo:

    gpio_set_direction(SEG_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_G, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT, GPIO_MODE_OUTPUT);
    
    gpio_set_level(SEG_A, 0);
    gpio_set_level(SEG_B, 0);
    gpio_set_level(SEG_C, 0);
    gpio_set_level(SEG_D, 0);
    gpio_set_level(SEG_E, 0);
    gpio_set_level(SEG_F, 0);
    gpio_set_level(SEG_G, 0);
    gpio_set_level(OUT, 0);

O canal 0 do primeiro conversor analógico/digital foi então configurado:

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

Foi criado o semáforo para desbloqueio da função de atualização do display LED e de definição do set point do sistema:

    xSemaphore1 = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore1);

As cinco tasks declaradas inicialmente foram então efetivamente criadas:

    xTaskCreate(task_temperature, "task_temperature", 2048, NULL, 3, &xTask1Handle);
    xTaskCreate(task_keyboard, "task_keyboard", 2048, NULL, 4, &xTask2Handle);
    xTaskCreate(task_print, "task_print", 2048, NULL, 2, &xTask3Handle);
    xTaskCreate(task_display_7seg, "task_display_7seg", 2048, NULL, 1, &xTask4Handle);
    xTaskCreate(task_control, "task_control", 2048, NULL, 0, &xTask5Handle);
    }

A task para medição da temperatura do sistema realiza a leitura do valor analógico presente na entrada do canal zero do conversor primeiro analógico/digital, em seguida armazena o valor final em uma variável de auxiliar e por fim realiza uma espera de um segundo, para que a medição da temperatura não seja feita constantemente:

    void task_temperature(void *pvParameters)
    {
    (void)pvParameters;

    while(1)
    {
        for (int i = 0; i < 100; i++)
        {
        adc += adc1_get_raw(ADC1_CHANNEL_0);
        ets_delay_us(30);
        }
        adc /= 4096;
        temperature = adc;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    }

A task para leitura do teclado matricial realiza a identificação da tecla pressionada através da multiplexação. Os pinos conectados às linhas são os barramentos compartilhados, enquanto que os pinos conectados às colunas da matriz são os dispositivos. Para a leitura, uma coluna por vez é definida como saída e definida em nível baixo, e então é realizada a leitura dos pinos de cada linha. Desta forma, quando for identificado o nível baixo em algum dos pinos das linhas, o valor de qual coluna estava definida como saída e de qual linha estava sendo analisada é guardado. Este valor corresponde à posição da tecla pressionada. Quando este valor é encontrado, a task disponibiliza o semáforo binário, para que as ações da task de atualização do display LED sejam executadas e os valores que compõem o set point sejam atualizados:

    void task_keyboard(void *pvParameters)
    {
    (void)pvParameters;

    while(1)
    {
        for(c = 0; c < col_size; c++)
        {
        gpio_set_direction(col_pins[c], GPIO_MODE_OUTPUT);
        gpio_set_level(col_pins[c],0);
        for(r = 0; r < row_size; r++)
        {
            if(gpio_get_level(row_pins[r]) == 0)
            {
            vTaskDelay(pdMS_TO_TICKS(50));
            new = keys[r][c];
            xSemaphoreGive(xSemaphore1);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        gpio_set_direction(col_pins[c], GPIO_MODE_INPUT);
        }
    }
    }

Como forma de interação com o usuário, uma task é executada periodicamente para exibir em um terminal o valor da medição da temperatura atual, o valor do set point definido e o estado do sistema:

    void task_print(void *pvParameters)
    {
    (void)pvParameters;

    while(1)
    {
        printf("Temperatura: %u°C | Set point: %c%c°C | Sistema: %c%c%c\n", temperature, dez, un, system_state[0], system_state[1], system_state[2]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    }

Quando o semáforo é disponibilizado pela task de leitura do teclado, as ações da task do display LED são executadas. Esta task associa a uma variável auxiliar um índice numérico relacionado à tecla preenchida, para que as saídas conectadas ao display LED sejam acionadas de acordo com as definições presentes no vetor com as configurações dos números a serem exibidos:

    void task_display_7seg(void *pvParameters)
    {
    (void)pvParameters;

    uint8_t x = 0;

    while(1)
    {
        if(xSemaphoreTake(xSemaphore1, portMAX_DELAY) == pdTRUE)
        {
        switch(new)
        {
            case '0':
            x = 0;
            break;
            case '1':
            x = 1;
            break;
            case '2':
            x = 2;
            break;
            case '3':
            x = 3;
            break;
            case '4':
            x = 4;
            break;
            case '5':
            x = 5;
            break;
            case '6':
            x = 6;
            break;
            case '7':
            x = 7;
            break;
            case '8':
            x = 8;
            break;
            case '9':
            x = 9;
            break;
            default:
            break;
        }

        gpio_set_level(SEG_A,tst_bit(display[x],0));
        gpio_set_level(SEG_B,tst_bit(display[x],1));
        gpio_set_level(SEG_C,tst_bit(display[x],2));
        gpio_set_level(SEG_D,tst_bit(display[x],3));
        gpio_set_level(SEG_E,tst_bit(display[x],4));
        gpio_set_level(SEG_F,tst_bit(display[x],5));
        gpio_set_level(SEG_G,tst_bit(display[x],6));

Cada execução desta task incrementa um contador, para que seja definido se o valor lido do teclado será utilizado como valor de dezena do set point ou como unidade. E, caso seja pressionada a tecla # no teclado, os valores das dezenas e unidades lidos são então definidos como o novo set point de temperatura, e o display LED passa a exibir um traço:

        if(cont == 0)
        {
            dez = new;
            cont++;
        }
        else if(cont == 1)
        {
            un = new;
            cont++;
        }
        else if(new == '#')
        {
            cont = 0;
            set_point = (dez-48)*10 + (un-48);
            gpio_set_level(SEG_A,0);
            gpio_set_level(SEG_B,0);
            gpio_set_level(SEG_C,0);
            gpio_set_level(SEG_D,0);
            gpio_set_level(SEG_E,0);
            gpio_set_level(SEG_F,0);
            gpio_set_level(SEG_G,1);
        }
        }
    }
    }

A task de controle do sistema compara os valores da temperatura lida e do set point, e caso o valor da temperatura seja maior que o set point, a saída digital relacionada ao acionamento do sistema é colocada em nível alto e a indicação do estado do sistema passa a ser “ON”; caso contrário, a saída digital é colocada em nível baixo e a indicação passa a ser “OFF”:

    void task_control(void *pvParameters)
    {
    (void)pvParameters;

    while(1)
    {
        if(temperature > set_point)
        {
        gpio_set_level(OUT, 1);
        system_state[0] = 'O';
        system_state[1] = 'N';
        system_state[2] = ' ';
        }
        else
        {
        gpio_set_level(OUT, 0);
        system_state[0] = 'O';
        system_state[1] = 'F';
        system_state[2] = 'F';
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    }

----------------------------------------------------------------------------------

Conclusão:
A utilização de um sistema operacional de tempo real permite a este e outros projetos um trabalho voltado com maior prioridade à aplicação em si, já que apresenta uma camada de abstração do hardware do dispositivo em uso. Além do direcionamento voltado ao resultado, esta camada de abstração facilita a portabilidade do código para outras plataformas de desenvolvimento.
O recurso de gerenciamento da memória dos RTOS oferece a vantagem de tornar previsível a frequência em que cada execução tarefa será executada.
Para o projeto em questão, as tarefas de leitura da temperatura, identificação de uma tecla do teclado pressionada, exibição das condições do sistema na tela, atualização do display e de controle do sistema se tornam periódicas com o uso do FreeRTOS, tal gerenciamento do tempo de execução, associado à capacidade de determinação de prioridades de execução, tornam o comportamento do sistema confiável e padronizado.

----------------------------------------------------------------------------------

Referências:
FreeRTOS. Espressif. Disponível em: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html>. Acesso em: 23 dez. 2021.

PICORETI, Rodolfo. Teclado Matricial e Multiplexação. Vida de Silício. 2017. Disponível em: <https://portal.vidadesilicio.com.br/teclado-matricial-e-multiplexacao/>. Acesso em: 23 dez. 2021.