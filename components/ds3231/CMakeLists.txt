idf_component_register(
    SRCS "ds3231.c" "i2cdev/i2cdev.c"
    INCLUDE_DIRS "i2cdev" "esp_idf_lib_helpers" 
    REQUIRES log
)
