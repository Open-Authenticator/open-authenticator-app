# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main_lvgl.c" "ntp.c" "rtc.c" "sync.c" "totp.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

register_component()

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_LVGL_H_INCLUDE_SIMPLE")

