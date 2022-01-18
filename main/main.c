#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"  
#include "string.h"
#include "uart0.h"
#include "led.h"
#include "key.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "wifi.h"
#include "src\lv_core\lv_obj.h"
#include "mqtt_client.h"
//********************************************************************
//开始任务
#define START_TASK_PRIO   1             //优先级
#define START_STK_SIZE    2048           //栈大小
TaskHandle_t    StartTask_Handler;      //句柄
void start_task(void *pvParameters);    //句柄函数
//打印任务
#define Printf_TASK_PRIO     2
#define Printf_STK_SIZE     2048
TaskHandle_t    Printf_Handler;
void Printf_task(void *pvParameters);
//led任务
#define LED_TASK_PRIO       4
#define LED_STK_SIZE        2048
TaskHandle_t    LED_Handler;
void LED_task(void *pvParameters);

//串口读任务
#define UART_RX_TASK_PRIO       5
#define UART_RX_STK_SIZE      4048
TaskHandle_t    UART_RX_Handler;
void UART_RX_task(void *pvParameters);
//高德天气查询任务
#define Gaode_TASK_PRIO       3
#define Gaode_STK_SIZE      15*1024
TaskHandle_t    Gaode_Handler;
void Gaode_task(void *pvParameters);

//********************************************************************
//统计led次数的消息队列
int g_led_num=0;
#define LED_MESS    1
QueueHandle_t   LEDMS_Queue;
//按键中断二值信号量
SemaphoreHandle_t key_SemaphoreHandler;
nvs_handle_t WifiConfig_Set_Handler;//创建wifi密码保存句柄
extern EventGroupHandle_t s_wifi_event_group;//创建wifi事件句柄
extern const int CONNECTED_BIT;//wifi连接位

//********************************************************************

#define TAG "demo"
#define LV_TICK_PERIOD_MS 1
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);
SemaphoreHandle_t xGuiSemaphore;
//********************************************************************
void app_main() 
{
    
    vTaskDelay(pdMS_TO_TICKS(100));//等待系统初始化
    esp_err_t ret = nvs_flash_init();//初始化flash（wifi需要falsh保存）
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());//如果falsh出错，则格式化flash
      ret = nvs_flash_init();
      ESP_ERROR_CHECK(nvs_flash_init());//初始化flash（wifi需要falsh保存）
      wifi_nsv_init();//wifi账号密码初始化
    }
    ESP_ERROR_CHECK(ret);
    initialise_wifi();//一键联网
    uart_init();
    led_init();
    key_init(); 
    LEDMS_Queue=xQueueCreate(LED_MESS,sizeof(int));           //创建led消息队列
    key_SemaphoreHandler=xSemaphoreCreateBinary();            //创建key二值信号

    xTaskCreatePinnedToCore((TaskFunction_t  ) start_task,              //任务函数
							(const char *    ) "start_task",            //任务名称
							(uint32_t        ) START_STK_SIZE,          //任务栈大小
							(void *          ) NULL,                    //传递给函数参数
							(UBaseType_t     )START_TASK_PRIO,          //任务优先级
							(TaskHandle_t *  )&StartTask_Handler,       //任务句柄
							(BaseType_t      )tskNO_AFFINITY);          //指定cpu                                   
    vTaskStartScheduler();
    
}

//*************************************************************************************************
//开始任务
//*************************************************************************************************
void start_task(void *pvParameters)
{
    //taskENTER_CRITICAL();//进入临界区
    xTaskCreatePinnedToCore((TaskFunction_t  ) Printf_task,              //任务函数
							(const char *    ) "Printf_task",            //任务名称
							(uint32_t        ) Printf_STK_SIZE,          //任务栈大小
							(void *          ) NULL,                    //传递给函数参数
							(UBaseType_t     )Printf_TASK_PRIO,          //任务优先级a
							(TaskHandle_t *  )&Printf_Handler,       //任务句柄
							(BaseType_t      )tskNO_AFFINITY);          //指定cpu

    xTaskCreatePinnedToCore((TaskFunction_t  ) LED_task,              //任务函数
							(const char *    ) "LED_task",            //任务名称
							(uint32_t        ) LED_STK_SIZE,          //任务栈大小
							(void *          ) NULL,                    //传递给函数参数
							(UBaseType_t     )LED_TASK_PRIO,          //任务优先级
							(TaskHandle_t *  )&LED_Handler,       //任务句柄
							(BaseType_t      )tskNO_AFFINITY);          //指定cpu 
    xTaskCreatePinnedToCore((TaskFunction_t  ) UART_RX_task,              //任务函数
							(const char *    ) "UART_RX_task",            //任务名称
							(uint32_t        ) UART_RX_STK_SIZE,          //任务栈大小
							(void *          ) NULL,                    //传递给函数参数
							(UBaseType_t     )UART_RX_TASK_PRIO,          //任务优先级
							(TaskHandle_t *  )&UART_RX_Handler,       //任务句柄
							(BaseType_t      )tskNO_AFFINITY);          //指定cpu  
    xTaskCreatePinnedToCore((TaskFunction_t  ) Gaode_task,              //任务函数
							(const char *    ) "Gaode_task",            //任务名称
							(uint32_t        ) Gaode_STK_SIZE,          //任务栈大小
							(void *          ) NULL,                    //传递给函数参数
							(UBaseType_t     )Gaode_TASK_PRIO,          //任务优先级
							(TaskHandle_t *  )&Gaode_Handler,       //任务句柄
							(BaseType_t      )tskNO_AFFINITY);          //指定cpu  
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 30, NULL, 1);   //LVGL任务                                           
    vTaskDelete(StartTask_Handler);             //删除开始任务
   // taskEXIT_CRITICAL();                        //退出临界区
}

//*************************************************************************************************
//打印任务
//*************************************************************************************************
void Printf_task(void *pvParameters)
{
    int temp_i=0;
    BaseType_t err;
   while(1)
    {         
       err=xQueueReceive(LEDMS_Queue,(void*)&temp_i,portMAX_DELAY);//获取led灯次数的消息
        if(err==pdFALSE)//判断消息是否有效
        {
           printf("消息发送失败\n"); 
        }
        printf("LED:%d次\n",temp_i);//打印
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//*************************************************************************************************
//led任务
//*************************************************************************************************
void LED_task(void *pvParameters)
{
    static bool led_mode=0;//led模式选择位
    BaseType_t err;
   while(1)
    {          
        err=xSemaphoreTake(key_SemaphoreHandler,0);//获取led二值信号量
        if(err==pdTRUE)
        {
            led_mode=!led_mode;
        }
        led_dispaly(led_mode);//led显示
        g_led_num++;//led灯计数
        xQueueOverwrite(LEDMS_Queue,(void*)&g_led_num);//发送led次数消息
    }
}
//*************************************************************************************************
//串口任务
//*************************************************************************************************
void UART_RX_task(void *pvParameters)
{
    uint8_t *data=(uint8_t*)malloc(1024+1);
    while(1)
    {
        int rxBytes=uart_read_bytes(UART_NUM_0,data,1024,10/portTICK_RATE_MS);      
        if(rxBytes>0)
        {       
            if(data[0]=='$')
            {
                rxBytes=rxBytes-2;
                CommandDebug((char*)data,rxBytes);
            }
         data[rxBytes]=0; 
        }
    }
    free(data);
}
//*************************************************************************************************
//高德天气查询任务
//*************************************************************************************************
void Gaode_task(void *pvParameters)
{
    EventBits_t uxBits;//创建事件标志位
    Gaode_api Weather;
    while (1)
    {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, pdFALSE, false, portMAX_DELAY);//等待wifi事件连接或者连接完成事件
        if(uxBits & CONNECTED_BIT)
        {
            vTaskDelay(10000/portTICK_PERIOD_MS);//延时10秒
            Weather=http_client_task();
            if(Weather.state==1)
            {
                printf("\r\n/----------------------------------------------------------------/\r\n");
                printf("\r\ntask_province:%s\r\n",(char*)Weather.province);
                printf("\r\ntask_city:%s\r\n",(char*)Weather.city);
                printf("\r\ntask_data:%s\r\n",(char*)Weather.today.data);
                printf("\r\ntask_Weather:%s\r\n",(char*)Weather.today.dayweather);
                printf("\r\ntask_daytemp:%s\r\n",(char*)Weather.today.daytemp);
                printf("/----------------------------------------------------------------/\r\n");
                printf("\r\ntask_data:%s\r\n",(char*)Weather.tomorrow.data);
                printf("\r\ntask_Weather:%s\r\n",(char*)Weather.tomorrow.dayweather);
                printf("\r\ntask_daytemp:%s\r\n",(char*)Weather.tomorrow.daytemp);
                printf("/----------------------------------------------------------------/\r\n");
                printf("\r\ntask_data:%s\r\n",(char*)Weather.next_tomorrow.data);
                printf("\r\ntask_Weather:%s\r\n",(char*)Weather.next_tomorrow.dayweather);
                printf("\r\ntask_daytemp:%s\r\n",(char*)Weather.next_tomorrow.daytemp);
                printf("/----------------------------------------------------------------/\r\n"); 
                printf("\r\ntask_data:%s\r\n",(char*)Weather.last_day.data);
                printf("\r\ntask_Weather:%s\r\n",(char*)Weather.last_day.dayweather);
                printf("\r\ntask_daytemp:%s\r\n",(char*)Weather.last_day.daytemp);
                printf("/----------------------------------------------------------------/\r\n");                
            }
            vTaskDelay(10000/portTICK_PERIOD_MS);//延时10秒
        }   
    }
    
 
}

//********************************************************************
//LVGL初始化
//********************************************************************
static void guiTask(void *pvParameter) {

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    /* Actual size in pixels, not bytes. */
    size_in_px *= 8;
#endif

    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create the demo application */
    create_demo_application();

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}
//********************************************************************
//显示函数
//********************************************************************
static void create_demo_application(void)
{
    lv_obj_t * scr      = lv_disp_get_scr_act(NULL);
    lv_obj_t * label1   = lv_label_create(scr, NULL);
    lv_obj_t* obj1      = lv_obj_create(scr,NULL);
    lv_obj_t * label2   = lv_label_create(obj1, NULL);

    lv_style_t style1;
    //lv_style_init(&style1);
    
    //lv_obj_add_style(label2,0,&style1);
    lv_label_set_text(label1, "qiuhe");
    lv_label_set_text(label2, "qiuhe");
    //lv_style_set_text_color(&style1,LV_STATE_DEFAULT,LV_COLOR_RED);

    lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 5);
    lv_obj_set_size(obj1,50,50);
    lv_obj_set_pos(obj1,10,10);
}
//********************************************************************
//LVGL时钟源
//********************************************************************
static void lv_tick_task(void *arg)
 {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
