#include "wifi.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
//#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <sys/socket.h>
#include "esp_http_client.h"
#include "cJSON.h"
#include "mqtt_client.h"
//#include "Utf8ToGbk.h"
//******************************************************************
#define PORT                        8266//端口
#define KEEPALIVE_IDLE              7200//TCP链接在多少秒之后没有数据报文传输启动探测报文
#define KEEPALIVE_INTERVAL          75//前一个探测报文和后一个探测报文之间的时间间隔
#define KEEPALIVE_COUNT             9//心跳探测的次数
#define TCP_Server                  0//tcp_servre条件编译 

//******************************************************************
EventGroupHandle_t s_wifi_event_group;//创建wifi事件句柄
const int CONNECTED_BIT = BIT0;//wifi连接�?
const int ESPTOUCH_DONE_BIT = BIT1;//wifi连接完成�?
const char *TAG = "wifi_set";//打印
const char *HTTP_TAG="HTTP";
extern nvs_handle_t WifiConfig_Set_Handler;//创建wifi密码保存句柄
extern SemaphoreHandle_t key_SemaphoreHandler;//按键二值信号句�?
//******************************************************************
void smartconfig_example_task(void * parm);
void tcp_server_task(void *pvParameters);
void do_retransmit(const int sock);
//******************************************************************
uint8_t temp_wififalg=0;//wifi标志�?
uint8_t temp_wifista=0;//wifi密码正确�?
size_t nvs_ssid_size;//wifi名称长度
size_t nvs_pw_size;//wiif密码长度
uint8_t wifi_connect_num=0,connect_sta=0;//wifi自动连接计数�?

//****************************/高德天气api/**************************************
#define  GDhost       "restapi.amap.com"//高德api网站
#define  httpPort      80//端口
#define  ApiKey       "8ac9d07aaf8b66109771b33773473030"
#define  City         "510117"//城市编码
#define  OutPut       "JSON"
#define  extensions   "all"//all：查询天气预报，base：查询当前天�?
//**************************/mqtt配置信息/*****************************************
#define mqtt_host       "aaywnvq.iot.gz.baidubce.com"        //host
#define mqtt_port        1883                                //端口
#define mqtt_username   "thingidp@aaywnvq|app|0|MD5"         //用户名
#define mqtt_password   "280a5fdf30d7eebb5ff104887b8310d7"   //密码
//*******************************************************************
//wifi״状态机
//*******************************************************************
void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)//系统事件循环处理函数 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) //如果系统循环事件时wifi事件和wifi处于正常状态时
    {
        ESP_LOGI(TAG, "wifi start!");//打印wifi扫描完成 
        esp_wifi_connect(); //wifi连接   
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)//smartonfig扫描完成
     {
        ESP_LOGI(TAG, "Scan done");
     }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) //如果系统循环事件时wifi事件和wifi未连接事件时
    {
        ESP_LOGI(TAG, "ip_wifi_connet");
        wifi_connect_num++;//连接次数
        esp_wifi_connect();//进行wifi连接
        if((wifi_connect_num>20)&&(connect_sta==0))//如果wifi连接次数超过规定值，则启动自动配�?
        {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);//创建smartconfig任务 
        }
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);//清除wifi连接成功事件标志�?
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)//获取到ip
     {
        
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);//设置wifi连接事件标志位
        xSemaphoreGive(key_SemaphoreHandler);//改变led状态
        #if TCP_Server
            xTaskCreate(tcp_server_task, "tcp_server", 4096,NULL, 5, NULL);//����tcp�ͻ���
        #endif 
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);//创建tcp客户事件位
        mqtt_app_start();
        connect_sta=1;//wifi配网完成
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)//如果smartconfig事件产生和smartconfig的wifi扫描完成�?
    {
        ESP_LOGI(TAG, "Scan done");//打印wifi扫描完成
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)//如果smartconfig事件产生和smartconfig的通道扫描完成�?
     {
        ESP_LOGI(TAG, "Found channel");
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) //如果smartconfig事件产生和获取到wifi名称和密码时
    {
        ESP_LOGI(TAG, "Got SSID and password");//打印wifi的名称和密码获取成功

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;//初始化wifi名称和密�?
        wifi_config_t wifi_config;//初始化wifi
        char ssid[33] = { 0 };//创建ssid数组
        char password[65] = { 0 };//创建password数组
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));//清除wifi_config参数
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));//拷贝wifi名称（用于wifi配置�?
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));//拷贝wifi密码（用于wifi配置�?
        wifi_config.sta.bssid_set = evt->bssid_set;//设置mac地址
        if (wifi_config.sta.bssid_set == true) //判断mac地址是否有效
        {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));//拷贝mac地址
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));//拷贝wifi名称（用于打印）
        memcpy(password, evt->password, sizeof(evt->password));//拷贝wifi密码（用于打印）
        ESP_LOGI(TAG, "SSID:%s", ssid);//打印wifi名称
        ESP_LOGI(TAG, "PASSWORD:%s", password);//打印wifi密码
        //保存wifi名字和密�?
        ESP_ERROR_CHECK(nvs_open("WifiConfig",NVS_READWRITE,&WifiConfig_Set_Handler));//创建或打开wifi保存文件�?
        ESP_ERROR_CHECK(nvs_set_u8(WifiConfig_Set_Handler,"WifiConfigFlag",pdTRUE));//定义wifi是否存在�?
        ESP_ERROR_CHECK(nvs_set_u8(WifiConfig_Set_Handler,"WifiConfigSta",pdTRUE));//定义wifi密码是否有效�?
        ESP_ERROR_CHECK(nvs_set_str(WifiConfig_Set_Handler,"SSID",ssid));//保存wifi名称
        ESP_ERROR_CHECK(nvs_set_str(WifiConfig_Set_Handler,"PASSWORD",password));//保存wifi密码
        ESP_ERROR_CHECK(nvs_commit(WifiConfig_Set_Handler));//提交wifi密码�?
        nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�?

        if (evt->type == SC_TYPE_ESPTOUCH_V2) //判断是否为ESPTouchv2协议（app上的选择模式�?
        {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );//获取 ESPTouch v2 的保留数据�?
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++)
             {
                printf("%02x ", rvd_data[i]);
             }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );//断开wifi连接
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );//配置wifi设置
        esp_wifi_connect();//开始wifi连接
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) //如果smartconfig事件产生和smartconfig已向手机发�? ACK
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);//智能配网完成
    }
    else if (event_id ==SYSTEM_EVENT_STA_CONNECTED) //wifi连接成功
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);//wifi配网完成
    }
}
//*******************************************************************
//wifi初始�?
//*******************************************************************
void initialise_wifi(void)
{ 
   
    ESP_ERROR_CHECK(esp_netif_init());//初始化tcp/ip
    s_wifi_event_group = xEventGroupCreate();//创建wifi事件标志�?
    ESP_ERROR_CHECK(esp_event_loop_create_default());//创建系统循环事件
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();//初始化wifi的sta模式
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();//创建wifi默认参数
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );//初始化wifi
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );//将wifi事件添加到系统事件循环中
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );//将ip事件添加到系统事件循环中
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );//smartconfig事件添加到系统事件循环中
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );//设置wifi为STA模式
    ESP_ERROR_CHECK(nvs_open("WifiConfig",NVS_READWRITE,&WifiConfig_Set_Handler));//创建或打开wifi保存文件�?
    ESP_ERROR_CHECK(nvs_get_u8(WifiConfig_Set_Handler,"WifiConfigFlag",&temp_wififalg));//获取wifi标志�?
    ESP_ERROR_CHECK(nvs_get_u8(WifiConfig_Set_Handler,"WifiConfigSta",&temp_wifista));//获取wifi密码是否正确标志�?
    //用于获取保存的wifi密码，并尝试连接
    if(temp_wififalg==1&&temp_wifista==1)
    {
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler, "SSID", NULL, &nvs_ssid_size));//获取ssid的字�?
        char nvs_ssid[32] = {0};//创建wifi名称保存空间
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler,"SSID",nvs_ssid,&nvs_ssid_size));//获取wifi名称
        ESP_LOGI(TAG, "NVS_wifi start!");//打印wifi扫描完成
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler, "PASSWORD", NULL, &nvs_pw_size));//获取wifi密码字长
        char nvs_password[64] = {0};//创建wifi密码空间
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler,"PASSWORD",nvs_password,&nvs_pw_size));//获取wifi密码
        nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�?
        wifi_config_t wifi_config;//创建wifi设置参数
        bzero(&wifi_config, sizeof(wifi_config_t));//清除wifi_config参数
        memcpy(wifi_config.sta.ssid, nvs_ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, nvs_password, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );//配置wifi设置
        ESP_ERROR_CHECK( esp_wifi_start() );//开始wifi连接
    }  
    nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�?
    ESP_ERROR_CHECK( esp_wifi_start() );//开始wifi连接

}
//*******************************************************************
//用于打开smartconfig，并判断wifi是否在连接中或连接完成，并关闭smartconfig
//*******************************************************************
void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;//创建事件标志�?
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );//设置可以微信配网模式
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();//默认初始化smartconfig
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );//默认初始化smartconfig
    while (1) 
    {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);//等待wifi事件连接或者连接完成事�?
        if(uxBits & CONNECTED_BIT)//wifi连接事件产生
        {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT)//wifi连接完成事件产生
         {
            ESP_ERROR_CHECK(nvs_open("WifiConfig",NVS_READWRITE,&WifiConfig_Set_Handler));//创建或打开wifi保存文件�?
            ESP_ERROR_CHECK(nvs_set_u8(WifiConfig_Set_Handler,"WifiConfigSta",pdTRUE));//定义wifi密码是否有效�?
            ESP_ERROR_CHECK(nvs_commit(WifiConfig_Set_Handler));//提交wifi密码�?
            nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�?
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();//关闭smartconfig
            xSemaphoreGive(key_SemaphoreHandler);//改变led灯状�?
            vTaskDelete(NULL);//删除自身任务
        }
    }
}
//*******************************************************************
//初始化wifi名字和密�?
//*******************************************************************
void wifi_nsv_init()

{
    char ssid_init[33] = {'H','e','l','l','o','\0'};//创建初始化ssid数组
    char password_init[65] ={'U','N','D','F','o','o','o','o','\0'};//创建初始化password数组
    ESP_ERROR_CHECK(nvs_open("WifiConfig",NVS_READWRITE,&WifiConfig_Set_Handler));//创建或打开wifi保存文件�?
    ESP_ERROR_CHECK(nvs_set_u8(WifiConfig_Set_Handler,"WifiConfigFlag",pdTRUE));//定义wifi是否存在�?
    ESP_ERROR_CHECK(nvs_set_u8(WifiConfig_Set_Handler,"WifiConfigSta",pdTRUE));//定义wifi密码是否有效�?
    ESP_ERROR_CHECK(nvs_set_str(WifiConfig_Set_Handler,"SSID",ssid_init));//保存wifi名称
    ESP_ERROR_CHECK(nvs_set_str(WifiConfig_Set_Handler,"PASSWORD",password_init));//保存wifi密码
    ESP_ERROR_CHECK(nvs_commit(WifiConfig_Set_Handler));//提交wifi密码�?
    nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�?
 }
//*******************************************************************
//tcp客户端通讯任务
//*******************************************************************
/*void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(0x00000000);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);//创建socket，第一项为ip类型，第二项为数据传输方式，第三项udp还是tcp
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//设置套接口的选项
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));//设置服务器的ip和端�?
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);//开始监听创建的listen_sock
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; 
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);//监听任何客户端的连接，并返回客服端地址并创建新的一个socket
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        //  设置tcp的心�?
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        //返回客户端ip地址
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);//数据接收与转�?
        shutdown(sock, 0);//关闭sock
        close(sock);//关闭sock
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}*/
//*******************************************************************
//tcp接收数据并发回数�?
//*******************************************************************
/*void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[1024];
    do 
    {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);//读取tcp数据
        CommandDebug(rx_buffer,len);
       // ESP_LOGW(TAG, "byteLength:%d",len);
        if (len < 0) 
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } 
        else if (len == 0) 
        {
            ESP_LOGW(TAG, "Connection closed");
        } 
        else 
        {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            int to_write = len;//获取字符长度
            while (to_write > 0)
            {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);//发送数�?
                if (written < 0) 
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }
        }
    } while (len > 0);
}*/
//*******************************************************************
//网络&串口命令控制
//Command_str:获取的字符串
//len：字符长�?
//*******************************************************************
void CommandDebug(char *Command_str,int len)
{
    char *help="$help";//led灯控�?
    char *led_change="$led_change";//led灯控�?
    char *ip_query  ="$ip";        //ip查询
    char *wifi_query="$wifi";      //wifi名称密码查询
    char *wifi_init ="$wifi_init"; //wifi重置
    char *restart ="$restart";     //重启系统
    char temp_str[128]={0};       //数据临时缓存

    strncpy(temp_str,Command_str,len);//用于截取指定字符串，以消除串口的无用数据
    printf("temp_str:%s,strlen:%d,len:%d\r\n",temp_str,strlen(temp_str),len);
    if((len==strlen(led_change))&&(strcmp(temp_str,led_change)==0))
    {
        xSemaphoreGive(key_SemaphoreHandler);//改变led灯状�?
    }
    else if((len==strlen(ip_query))&&(strcmp(temp_str,ip_query)==0))//查询ip地址
    {    
        tcpip_adapter_ip_info_t local_ip;
        esp_err_t res_ap_get = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
        printf("IP地址ַ:"IPSTR"\n",IP2STR(&local_ip.ip));
        printf("MAC地址:"IPSTR"\n",IP2STR(&local_ip.netmask));
        printf("主机地址:"IPSTR"\n",IP2STR(&local_ip.gw));
    }
    else if((len==strlen(wifi_query))&&(strcmp(temp_str,wifi_query)==0))//查询wifi
    {

        ESP_ERROR_CHECK(nvs_open("WifiConfig",NVS_READWRITE,&WifiConfig_Set_Handler));//创建或打开wifi保存文件�?
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler, "SSID", NULL, &nvs_ssid_size));//获取wifi名称长度
        char ssid[32] = {0};//创建wifi名称保存空间
        //char gbkssid[32]={0};
       // int len=32;
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler,"SSID",ssid,&nvs_ssid_size));//获取wifi名称
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler, "PASSWORD", NULL, &nvs_pw_size));//获取wifi密码字长
        char password[64] = {0};//创建wifi密码空间
        ESP_ERROR_CHECK(nvs_get_str(WifiConfig_Set_Handler,"PASSWORD",password,&nvs_pw_size));//获取wifi密码
        nvs_close(WifiConfig_Set_Handler);//关闭wifi密码�? 
        //SwitchToGbk((unsigned char*)ssid,strlen(ssid),(unsigned char*)gbkssid,&len);
        printf("WIFI名称:%s\r\n",ssid);
        printf("WIFI密码:%s\r\n",password);  
    }
    else if((len==strlen(wifi_init))&&(strcmp(temp_str,wifi_init)==0))//重置wifi
    {
        printf("重置wifi�?.............\r\n");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        wifi_nsv_init();
        printf("重置成功，正在重�?.......\r\n");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        esp_restart();
    }
     else if((len==strlen(restart))&&(strcmp(temp_str,restart)==0))//重启
    {
        esp_restart();
    }
    else if((len==strlen(help))&&(strcmp(temp_str,help)==0))//帮助
    {
        printf("\r\n//****************************************************//\r\n");
        printf("$led_change:led灯控制\r\n");
        printf("$ip:ip查询\r\n");
        printf("$wifi:wifi名称密码查询\r\n");
        printf("$wifi_init:wifi名称密码重置\r\n");
        printf("$restart:重启系统\r\n");
        printf("//****************************************************//\r\n");
    }


}
//*******************************************************************
//HTTP服务
//*******************************************************************
Gaode_api http_client_task(void)
{   
	Gaode_api GdWeather;
    uint16_t  gbk=1200;
    char *output_buffer;
   // gbk_buffer=(char*)malloc(gbk*sizeof(char));
    output_buffer=(char*)malloc(gbk*sizeof(char));
    uint16_t content_length = 0;
	static const char *URL = "http://"GDhost"/v3/weather/weatherInfo?city="City\
							 "&key="ApiKey"&output="OutPut		\
							 "&extensions="extensions;
    while(1)
    {
        esp_http_client_config_t config = {
        .url = URL,
        };
    esp_http_client_handle_t client = esp_http_client_init(&config);//连接网站初始�?

    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);//get请求
    esp_err_t err = esp_http_client_open(client, 0);//开始请�?
    if (err != ESP_OK) 
    {
        ESP_LOGE(HTTP_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } 
    else 
    {
        content_length = esp_http_client_fetch_headers(client);//查找响应�?
        if (content_length < 0)
         {
            ESP_LOGE(HTTP_TAG, "HTTP client fetch headers failed");
         }
         else
         {
            int data_read = esp_http_client_read_response(client, output_buffer, 1300);//读响应头
            if (data_read >= 0) 
            {
               // ESP_LOGI(HTTP_TAG, "HTTP GET Status = %d, content_length = %d",
               // esp_http_client_get_status_code(client),//获取状�?
               // esp_http_client_get_content_length(client));//获取内容长度
               
       
               // printf("output_buffer:%s\r\n", (char*)output_buffer);
               // SwitchToGbk((unsigned char*)output_buffer,strlen(output_buffer),(unsigned char*)gbk_buffer,&gbk);
               // printf("\r\ndata:%s\r\n", (char*)gbk_buffer);
				GdWeather=cJSON_parse_task(output_buffer);//json解析
                 
            } else {
                ESP_LOGE(HTTP_TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
    //free(gbk_buffer);
    free(output_buffer);
    return GdWeather; 
    }
    
}
//*******************************************************************
//json解析
//*******************************************************************
Gaode_api  cJSON_parse_task(char* text)
 {
    Gaode_api gaode_data;
	cJSON *root=NULL,*status=NULL,*forecasts=NULL,*name1_json=NULL,*name3_json=NULL,*name4_json=NULL;
    cJSON *city=NULL,*province=NULL,*reporttime=NULL,*name2_json=NULL,*casts=NULL,*name5_json=NULL;
    cJSON *date0=NULL,*dayweather0=NULL,*nightweather0=NULL,*daytemp0=NULL,*nighttemp0=NULL,*daywind0=NULL,*nightwind0=NULL,*daypower0=NULL,*nightpower0=NULL;
    cJSON *date1=NULL,*dayweather1=NULL,*nightweather1=NULL,*daytemp1=NULL,*nighttemp1=NULL,*daywind1=NULL,*nightwind1=NULL,*daypower1=NULL,*nightpower1=NULL;
    cJSON *date2=NULL,*dayweather2=NULL,*nightweather2=NULL,*daytemp2=NULL,*nighttemp2=NULL,*daywind2=NULL,*nightwind2=NULL,*daypower2=NULL,*nightpower2=NULL;
    cJSON *date3=NULL,*dayweather3=NULL,*nightweather3=NULL,*daytemp3=NULL,*nighttemp3=NULL,*daywind3=NULL,*nightwind3=NULL,*daypower3=NULL,*nightpower3=NULL;
    
    root=cJSON_Parse(text);//创建JSON解析对象，返回JSON格式是否正确
    if(!root)
    {
        printf("JSON解析错误!\n\n"); //输出json格式错误信息
        gaode_data.state=0;
        return gaode_data;
    }
    else
    {
        status=cJSON_GetObjectItem(root,"status");//解析status
        gaode_data.state=1;
    }

    forecasts=cJSON_GetObjectItem(root,"forecasts");//获取forecasts内容
    name1_json = cJSON_GetArrayItem(forecasts,0);        //forecasts的数组第0个元�?
    city = cJSON_GetObjectItem(name1_json, "city")->valuestring;//city键对应的�?
    province = cJSON_GetObjectItem(name1_json, "province")->valuestring;//province键对应的�?
    reporttime = cJSON_GetObjectItem(name1_json, "reporttime")->valuestring;//reporttime键对应的�?
    strcpy(gaode_data.city,city); 
    strcpy(gaode_data.province,province);
    strcpy(gaode_data.reporttime,reporttime);  
    casts = cJSON_GetObjectItem(name1_json,"casts");        //获取casts内容
    name2_json = cJSON_GetArrayItem(casts,0);        //casts的数组第0个元素，当天天气

    date0= cJSON_GetObjectItem(name2_json, "date")->valuestring;
    dayweather0= cJSON_GetObjectItem(name2_json, "dayweather")->valuestring;
    nightweather0= cJSON_GetObjectItem(name2_json, "nightweather")->valuestring;
    daytemp0= cJSON_GetObjectItem(name2_json, "daytemp")->valuestring;
    nighttemp0= cJSON_GetObjectItem(name2_json, "nighttemp")->valuestring;  
    daywind0= cJSON_GetObjectItem(name2_json, "daywind")->valuestring;
    nightwind0=cJSON_GetObjectItem(name2_json, "nightwind")->valuestring;  
    daypower0=cJSON_GetObjectItem(name2_json, "daypower")->valuestring;
    nightpower0=cJSON_GetObjectItem(name2_json, "nightpower")->valuestring;

    strcpy((char*)gaode_data.today.data,date0); 
    strcpy((char*)gaode_data.today.dayweather,dayweather0); 
    strcpy((char*)gaode_data.today.nightweather,nightweather0); 
    strcpy((char*)gaode_data.today.daytemp,daytemp0); 
    strcpy((char*)gaode_data.today.nighttemp,nighttemp0); 
    strcpy((char*)gaode_data.today.daypower,daypower0); 
    strcpy((char*)gaode_data.today.nightpower,nightpower0); 


    name3_json = cJSON_GetArrayItem(casts,1);        //casts的数组第1个元素，明天天气
    date1= cJSON_GetObjectItem(name3_json, "date")->valuestring;
    dayweather1= cJSON_GetObjectItem(name3_json, "dayweather")->valuestring;
    nightweather1= cJSON_GetObjectItem(name3_json, "nightweather")->valuestring;
    daytemp1= cJSON_GetObjectItem(name3_json, "daytemp")->valuestring;
    nighttemp1= cJSON_GetObjectItem(name3_json, "nighttemp")->valuestring;  
    daywind1= cJSON_GetObjectItem(name3_json, "daywind")->valuestring;
    nightwind1=cJSON_GetObjectItem(name3_json, "nightwind")->valuestring;  
    daypower1=cJSON_GetObjectItem(name3_json, "daypower")->valuestring;
    nightpower1=cJSON_GetObjectItem(name3_json, "nightpower")->valuestring;

    strcpy((char*)gaode_data.tomorrow.data,date1); 
    strcpy((char*)gaode_data.tomorrow.dayweather,dayweather1); 
    strcpy((char*)gaode_data.tomorrow.nightweather,nightweather1); 
    strcpy((char*)gaode_data.tomorrow.daytemp,daytemp1); 
    strcpy((char*)gaode_data.tomorrow.nighttemp,nighttemp1); 
    strcpy((char*)gaode_data.tomorrow.daypower,daypower1); 
    strcpy((char*)gaode_data.tomorrow.nightpower,nightpower1); 


    name4_json = cJSON_GetArrayItem(casts,2);        //casts的数组第2个元素，后天天气
    date2= cJSON_GetObjectItem(name4_json, "date")->valuestring;
    dayweather2= cJSON_GetObjectItem(name4_json, "dayweather")->valuestring;
    nightweather2= cJSON_GetObjectItem(name4_json, "nightweather")->valuestring;
    daytemp2= cJSON_GetObjectItem(name4_json, "daytemp")->valuestring;
    nighttemp2= cJSON_GetObjectItem(name4_json, "nighttemp")->valuestring;  
    daywind2= cJSON_GetObjectItem(name4_json, "daywind")->valuestring;
    nightwind2=cJSON_GetObjectItem(name4_json, "nightwind")->valuestring;  
    daypower2=cJSON_GetObjectItem(name4_json, "daypower")->valuestring;
    nightpower2=cJSON_GetObjectItem(name4_json, "nightpower")->valuestring;

    strcpy((char*)gaode_data.next_tomorrow.data,date2); 
    strcpy((char*)gaode_data.next_tomorrow.dayweather,dayweather2); 
    strcpy((char*)gaode_data.next_tomorrow.nightweather,nightweather2); 
    strcpy((char*)gaode_data.next_tomorrow.daytemp,daytemp2); 
    strcpy((char*)gaode_data.next_tomorrow.nighttemp,nighttemp2); 
    strcpy((char*)gaode_data.next_tomorrow.daypower,daypower2); 
    strcpy((char*)gaode_data.next_tomorrow.nightpower,nightpower2); 


    name5_json = cJSON_GetArrayItem(casts,3);        //casts的数组第3个元素，大后天天�?
    date3= cJSON_GetObjectItem(name5_json, "date")->valuestring;
    dayweather3= cJSON_GetObjectItem(name5_json, "dayweather")->valuestring;
    nightweather3= cJSON_GetObjectItem(name5_json, "nightweather")->valuestring;
    daytemp3= cJSON_GetObjectItem(name5_json, "daytemp")->valuestring;
    nighttemp3= cJSON_GetObjectItem(name5_json, "nighttemp")->valuestring;  
    daywind3= cJSON_GetObjectItem(name5_json, "daywind")->valuestring;
    nightwind3=cJSON_GetObjectItem(name5_json, "nightwind")->valuestring;  
    daypower3=cJSON_GetObjectItem(name5_json, "daypower")->valuestring;
    nightpower3=cJSON_GetObjectItem(name5_json, "nightpower")->valuestring;

    strcpy((char*)gaode_data.last_day.data,date3); 
    strcpy((char*)gaode_data.last_day.dayweather,dayweather3); 
    strcpy((char*)gaode_data.last_day.nightweather,nightweather3); 
    strcpy((char*)gaode_data.last_day.daytemp,daytemp3); 
    strcpy((char*)gaode_data.last_day.nighttemp,nighttemp3); 
    strcpy((char*)gaode_data.last_day.daypower,daypower3); 
    strcpy((char*)gaode_data.last_day.nightpower,nightpower3);  
	cJSON_Delete(root);//释放json
    return gaode_data;
}

//*******************************************************************
//mqtt事件句柄函数
//*******************************************************************
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    
    esp_mqtt_event_handle_t event = event_data;// 获取MQTT客户端结构体指针
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) //事件处理
    {
        
        case MQTT_EVENT_CONNECTED:// 建立连接成功
            printf("MQTT_client cnnnect to EMQ ok. \n");
            esp_mqtt_client_publish(client, "SW_LED", "I am ESP32.", 0, 0, 0);    
            esp_mqtt_client_subscribe(client, "SW_LED", 0); // 订阅主题，qos=0          
            break;
        
        case MQTT_EVENT_DISCONNECTED:// 客户端断开连接
            printf("MQTT_client have disconnected. \n");
            break;
        
        case MQTT_EVENT_SUBSCRIBED:// 主题订阅成功
            printf("mqtt subscribe ok. msg_id = %d \n",event->msg_id);
            break;
        
        case MQTT_EVENT_UNSUBSCRIBED:// 取消订阅
            printf("mqtt unsubscribe ok. msg_id = %d \n",event->msg_id);
            break;
        
        case MQTT_EVENT_PUBLISHED://  主题发布成功
            printf("mqtt published ok. msg_id = %d \n",event->msg_id);
            break;
        
        case MQTT_EVENT_DATA:// 已收到订阅的主题消息
            printf("mqtt received topic: %.*s \n",event->topic_len, event->topic);
            printf("topic data: %.*s\r\n", event->data_len, event->data);
            CommandDebug(event->data,event->data_len);
            break;
        
        case MQTT_EVENT_ERROR:// 客户端遇到错误
            printf("MQTT_EVENT_ERROR \n");
            break;
        default:
            printf("Other event id:%d \n", event->event_id);
            break;
    }
}

//*******************************************************************
//mqtt通讯
//*******************************************************************
void mqtt_app_start(void)
{

    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .host = mqtt_host,                      //MQTT服务器IP
        .event_handle = mqtt_event_handler,    //MQTT事件
        .port=mqtt_port,                       //端口
        .username = mqtt_username,            //用户名
        .password = mqtt_password,            //密码
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);//通过esp_mqtt_client_init获取一个MQTT客户端结构体指针，参数是MQTT客户端配置结构体
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);//注册MQTT事件
    esp_mqtt_client_start(client);//开始mqtt
}