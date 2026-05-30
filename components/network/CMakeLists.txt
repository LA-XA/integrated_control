idf_component_register(SRCS "network.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_event nvs_flash esp_netif mqtt movement sensors) # 关键：在这里加上 movement