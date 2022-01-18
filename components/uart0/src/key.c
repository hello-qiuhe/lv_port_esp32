#include"key.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t key_SemaphoreHandler;//按键二值信号句柄
static void intrHandler(void *arg);
//*******************************************************************
//按键初始化
//*******************************************************************
void key_init()
{
    gpio_config_t key_gpio;
    key_gpio.intr_type      =GPIO_INTR_NEGEDGE;         //下降沿触发
    key_gpio.pin_bit_mask   =esp_key;                       //gpio引脚
    key_gpio.mode           =GPIO_MODE_INPUT;           //输入模式       
    key_gpio.pull_up_en     =1;                         //上拉使能
    gpio_config(&key_gpio);

    gpio_set_intr_type(esp_key,GPIO_INTR_NEGEDGE);          //配置下降沿触发
    gpio_install_isr_service(0);                        //开启中断
    gpio_isr_handler_add(esp_key,intrHandler,(void*)key_state);
    
}
//*******************************************************************
//按键中断
//*******************************************************************
static void intrHandler(void *arg)
{    
    BaseType_t key_isr_handler;
    xSemaphoreGiveFromISR(key_SemaphoreHandler,&key_isr_handler);//按键切换led二值句柄
    portYIELD_FROM_ISR(key_isr_handler);//任务调度
}