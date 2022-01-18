#include "uart0.h"
#include "driver/uart.h"  
#include  "string.h"
#include <stdio.h>
#include "sdkconfig.h"

//********************************************************
//uart0��ʼ��
//********************************************************
void uart_init(void)
{
    uart_config_t uart0_config;
    uart0_config.baud_rate = 115200;//������
    uart0_config.data_bits =UART_DATA_8_BITS;//����λ
    uart0_config.parity    =UART_PARITY_DISABLE;//У��λ
    uart0_config.stop_bits =UART_STOP_BITS_1;//ֹͣλ
    uart0_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;//Ӳ������
    uart_param_config(UART_NUM_0,&uart0_config);//���ô���0

    uart_set_pin(UART_NUM_0,TXD0_PIN,RXD0_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);//���ô�������
    uart_driver_install(UART_NUM_0,RX0_BUF_SIZE,TX0_BUF_SIZE,0,NULL,0);//ʹ�ܴ��ڣ����û����С
}