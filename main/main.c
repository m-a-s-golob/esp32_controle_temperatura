/*
Programa de um sistema de controle de temperatura
Por: Marcos Augusto Soares Golob
Data: 04/12/2021
Plataforma: ESP32
*/

#include <stdio.h>
#include "driver/gpio.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

TaskHandle_t xTask1Handle, xTask2Handle, xTask3Handle, xTask4Handle, xTask5Handle;
SemaphoreHandle_t xSemaphore1;

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

void task_temperature(void *pvParameters);
void task_keybord(void *pvParameters);
void task_print(void *pvParameters);
void task_display_7seg(void *pvParameters);
void task_control(void *pvParameters);

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
  
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

  xSemaphore1 = xSemaphoreCreateBinary();
  xSemaphoreGive(xSemaphore1);

  xTaskCreate(task_temperature, "task_temperature", 2048, NULL, 3, &xTask1Handle);
  xTaskCreate(task_keybord, "task_keybord", 2048, NULL, 4, &xTask2Handle);
  xTaskCreate(task_print, "task_print", 2048, NULL, 2, &xTask3Handle);
  xTaskCreate(task_display_7seg, "task_display_7seg", 2048, NULL, 1, &xTask4Handle);
  xTaskCreate(task_control, "task_control", 2048, NULL, 0, &xTask5Handle);
}

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

void task_keybord(void *pvParameters)
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

void task_print(void *pvParameters)
{
  (void)pvParameters;
  while(1)
  {
    printf("Temperatura: %u°C | Set point: %c%c°C | Sistema: %c%c%c\n", temperature, dez, un, system_state[0], system_state[1], system_state[2]);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

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

void task_control(void *pvParameters)
{
  (void)pvParameters;

  while(1){
    if(temperature > set_point){
      gpio_set_level(OUT,1);
      system_state[0] = 'O';
      system_state[1] = 'N';
      system_state[2] = ' ';
    }
    else{
      gpio_set_level(OUT,0);
      system_state[0] = 'O';
      system_state[1] = 'F';
      system_state[2] = 'F';
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
