#include "uart0.h"
#include "driver/uart.h"  
#include  "string.h"
#include <stdio.h>
#include "sdkconfig.h"

//********************************************************
//uart0初始化
//********************************************************
void uart_init(void)
{
    uart_config_t uart0_config;
    uart0_config.baud_rate = 115200;//波特率
    uart0_config.data_bits =UART_DATA_8_BITS;//数据位
    uart0_config.parity    =UART_PARITY_DISABLE;//校验位
    uart0_config.stop_bits =UART_STOP_BITS_1;//停止位
    uart0_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;//硬件控流
    uart_param_config(UART_NUM_0,&uart0_config);//设置串口0

    uart_set_pin(UART_NUM_0,TXD0_PIN,RXD0_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);//设置串口引脚
    uart_driver_install(UART_NUM_0,RX0_BUF_SIZE,TX0_BUF_SIZE,0,NULL,0);//使能串口，设置缓存大小
}