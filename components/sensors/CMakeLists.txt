idf_component_register(SRCS "sensors.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_adc) # 关键：在这里加上 movement