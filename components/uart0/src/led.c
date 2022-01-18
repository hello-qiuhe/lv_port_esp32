#include "led.h"
#include <stdio.h>
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include"driver/ledc.h"
#include "esp_system.h"
//********************************************************
//led��ʼ��
//********************************************************
void led_init(void)
{   
    //��ʼ��gpio
    gpio_pad_select_gpio(ESP_LED);
    gpio_set_direction(ESP_LED, GPIO_MODE_OUTPUT);
    //��ʼ��pwm��ʱ��
    /*
    ledc_timer_config_t LED_TIMER;
    LED_TIMER.speed_mode     =LEDC_HIGH_SPEED_MODE;  //����ģʽ
    LED_TIMER.duty_resolution=LEDC_TIMER_13_BIT;    //pwm�ֱ���
    LED_TIMER.freq_hz        =5000;                 //Ƶ��
    LED_TIMER.clk_cfg = LEDC_USE_APB_CLK;          //ʱ��Դ
    LED_TIMER.timer_num      =LEDC_TIMER_0;        //��ʱ�����
    ledc_timer_config(&LED_TIMER);
    //pwmͨ������
    ledc_channel_config_t led_cha;
    led_cha.channel         =LEDC_CHANNEL_0;            //ͨ������
    led_cha.duty            =0;                         //��ʼ��ռ�ձ�
    led_cha.gpio_num        =ESP_LED;                   //�ܽ�
    led_cha.speed_mode      =LEDC_HIGH_SPEED_MODE;      //����ģʽ
    led_cha.timer_sel       =LEDC_TIMER_0;              //�����ĳ�ʼ���Ķ�ʱ��0��
    ledc_channel_config(&led_cha);
    */


}
//********************************************************
//led������ʾ����
//num��0Ϊ����ģʽ��1Ϊ������ģʽ��2Ϊ����
//********************************************************
void led_dispaly(int num)
{
    if(num==0)
    {
        gpio_set_level(ESP_LED,0);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(ESP_LED,1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        /*ledc_fade_func_install(1);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LED_MIN_DUTY);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LED_MAX_DUTY);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0);
        vTaskDelay(300 / portTICK_PERIOD_MS);*/
    }
    else if(num==1)
    {
        gpio_set_level(ESP_LED,0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(ESP_LED,1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /*
        ledc_fade_func_install(0);
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LED_MAX_DUTY,LEDC_MAX_TIME);
        ledc_fade_start(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_MAX_TIME / portTICK_PERIOD_MS);
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LED_MIN_DUTY,LEDC_MAX_TIME);
        ledc_fade_start(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_MAX_TIME / portTICK_PERIOD_MS);
        */
    }

}