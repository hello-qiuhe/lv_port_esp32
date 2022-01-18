#include"key.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t key_SemaphoreHandler;//������ֵ�źž��
static void intrHandler(void *arg);
//*******************************************************************
//������ʼ��
//*******************************************************************
void key_init()
{
    gpio_config_t key_gpio;
    key_gpio.intr_type      =GPIO_INTR_NEGEDGE;         //�½��ش���
    key_gpio.pin_bit_mask   =esp_key;                       //gpio����
    key_gpio.mode           =GPIO_MODE_INPUT;           //����ģʽ       
    key_gpio.pull_up_en     =1;                         //����ʹ��
    gpio_config(&key_gpio);

    gpio_set_intr_type(esp_key,GPIO_INTR_NEGEDGE);          //�����½��ش���
    gpio_install_isr_service(0);                        //�����ж�
    gpio_isr_handler_add(esp_key,intrHandler,(void*)key_state);
    
}
//*******************************************************************
//�����ж�
//*******************************************************************
static void intrHandler(void *arg)
{    
    BaseType_t key_isr_handler;
    xSemaphoreGiveFromISR(key_SemaphoreHandler,&key_isr_handler);//�����л�led��ֵ���
    portYIELD_FROM_ISR(key_isr_handler);//�������
}