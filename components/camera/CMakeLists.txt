idf_component_register(SRCS "camera.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver network esp32-camera esp_psram)
