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
//led初始化
//********************************************************
void led_init(void)
{   
    //初始化gpio
    gpio_pad_select_gpio(ESP_LED);
    gpio_set_direction(ESP_LED, GPIO_MODE_OUTPUT);
    //初始化pwm定时器
    /*
    ledc_timer_config_t LED_TIMER;
    LED_TIMER.speed_mode     =LEDC_HIGH_SPEED_MODE;  //高速模式
    LED_TIMER.duty_resolution=LEDC_TIMER_13_BIT;    //pwm分辨率
    LED_TIMER.freq_hz        =5000;                 //频率
    LED_TIMER.clk_cfg = LEDC_USE_APB_CLK;          //时钟源
    LED_TIMER.timer_num      =LEDC_TIMER_0;        //定时器编号
    ledc_timer_config(&LED_TIMER);
    //pwm通道配置
    ledc_channel_config_t led_cha;
    led_cha.channel         =LEDC_CHANNEL_0;            //通道设置
    led_cha.duty            =0;                         //初始化占空比
    led_cha.gpio_num        =ESP_LED;                   //管脚
    led_cha.speed_mode      =LEDC_HIGH_SPEED_MODE;      //高速模式
    led_cha.timer_sel       =LEDC_TIMER_0;              //与上文初始化的定时器0绑定
    ledc_channel_config(&led_cha);
    */


}
//********************************************************
//led多种显示函数
//num：0为快闪模式，1为呼吸灯模式，2为慢闪
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