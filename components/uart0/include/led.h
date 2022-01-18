#ifndef __LED_H_
#define __LED_H_

#define ESP_LED  2

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       2
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TEST_CH_NUM       (1)
#define LED_MAX_DUTY           (8192)    // ����ı������Ŀ��ռ�ձ�
#define LED_MIN_DUTY           (0)    // �������С����Ŀ��ռ�ձ�
#define LEDC_MAX_TIME          (1800)    // �仯ʱ��
void led_init(void);
void led_dispaly(int num);

#endif