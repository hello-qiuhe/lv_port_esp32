set(uart0_srcs "src/uart0.c"
               "src/led.c"
               "src/key.c"
               "src/wifi.c"
               "src/Utf8ToGbk.c")
idf_component_register(SRCS "${uart0_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES nvs_flash 
                       REQUIRES esp_http_client
                       REQUIRES json
                       REQUIRES mqtt  )