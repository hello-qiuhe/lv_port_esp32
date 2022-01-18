#ifndef __WIFI_H_
#define __WIFI_H_
#include<stdint.h>
#include <string.h>  
#define MAX_CHAR                    25//结构体字符的长度
//**************************************************
struct Weather{
                char *data[MAX_CHAR]; 
                char *dayweather[MAX_CHAR];  
                char *nightweather[MAX_CHAR]; 
                char *daytemp[MAX_CHAR]; 
                char *nighttemp[MAX_CHAR]; 
                char *daywind[MAX_CHAR]; 
                char *nightwind[MAX_CHAR];
                char *daypower[MAX_CHAR]; 
                char *nightpower[MAX_CHAR];            
};
typedef struct  
{
                uint8_t state;
                char province[MAX_CHAR];
                char city[MAX_CHAR];
                char reporttime[MAX_CHAR]; 
                struct Weather today;
                struct Weather tomorrow;
                struct Weather next_tomorrow;
                struct Weather last_day;
}Gaode_api;
//**************************************************

void smartconfig_example_task(void * parm);
//void wifi_init_ap(void);
void initialise_wifi(void);
void smartconfig_example_task(void * parm);
void wifi_nsv_init();
void CommandDebug(char *Command_str,int len);
Gaode_api  cJSON_parse_task(char* text);
Gaode_api http_client_task(void);
void mqtt_app_start(void);

#endif