# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# プログラム内で処理を分けられるように、マクロ値としてコンパイラに渡す
if(PICO_BOARD STREQUAL "pico")
    set(PICO_BOARD_VALUE 1)
elseif(PICO_BOARD STREQUAL "pico_w")
    set(PICO_BOARD_VALUE 3)
elseif(PICO_BOARD STREQUAL "pico2")
    set(PICO_BOARD_VALUE 2)
elseif(PICO_BOARD STREQUAL "pico2_w")
    set(PICO_BOARD_VALUE 4)
else()
    set(PICO_BOARD_VALUE 0) # 未知のボード
endif()
add_compile_definitions(PICO_BOARD_VALUE=${PICO_BOARD_VALUE})

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(AS3935APP C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(AS3935APP AS3935APP.cpp
AS3935.cpp
I2CBase.cpp
FreqCounter.cpp
FlashMem.cpp
inetAction.cpp
DispClock.cpp
ScreenKeyboard.cpp
TouchCalibration.cpp
Settings.cpp
GUIEditbox.cpp
GUIMsgBox.cpp

lib-9341/misc/defines.cpp
lib-9341/Adafruit_GFX_Library/Adafruit_GFX.cpp
lib-9341/Adafruit_GFX_Library/Adafruit_SPITFT.cpp

lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.cpp
lib-9341/spi/spi.cpp
lib-9341/Kanji/KanjiHelper.cpp
lib-9341/core/cyw43_wrappers.cpp
lib-9341/core/delay.cpp
lib-9341/core/_freertos.cpp
lib-9341/core/CoreMutex.cpp
lib-9341/core/Print.cpp
lib-9341/core/stdlib_noniso.cpp
lib-9341/core/WString.cpp
lib-9341/core/wiring_private.cpp
lib-9341/core/wiring_digital.cpp
lib-9341/core/wiring_analog.cpp
lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.cpp
lib-9341/misc/pins.cpp

)

pico_set_program_name(AS3935APP "AS3935APP")
pico_set_program_version(AS3935APP "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(AS3935APP 0)
pico_enable_stdio_usb(AS3935APP 1)

# Add the standard library to the build
target_link_libraries(AS3935APP
        pico_stdlib
    #使用するライブラリを追加する        
    pico_rand
    pico_unique_id
    hardware_exception        
)

# Add the standard include files to the build
target_include_directories(AS3935APP PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
#インクルードディレクトリも追加する       
${CMAKE_CURRENT_LIST_DIR}
${CMAKE_CURRENT_LIST_DIR}/lib-9341
${CMAKE_CURRENT_LIST_DIR}/lib-9341/Adafruit_GFX_Library
${CMAKE_CURRENT_LIST_DIR}/lib-9341/Adafruit_GFX_Library/Fonts
${CMAKE_CURRENT_LIST_DIR}/lib-9341/Adafruit_BusIO
${CMAKE_CURRENT_LIST_DIR}/lib-9341/Adafruit_ILI9341
${CMAKE_CURRENT_LIST_DIR}/lib-9341/core
${CMAKE_CURRENT_LIST_DIR}/lib-9341/spi
${CMAKE_CURRENT_LIST_DIR}/lib-9341/variants
${CMAKE_CURRENT_LIST_DIR}/lib-9341/Kanji
${CMAKE_CURRENT_LIST_DIR}/lib-9341/misc
${CMAKE_CURRENT_LIST_DIR}/XPT2046_Touchscreen   
#PICOW以外の場合、これ以降は不要         
${PICO_SDK_PATH}/src/rp2_common/hardware_exception/include
${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include
${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_driver/include
${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_driver/include/pico
${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include/pico
${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include
${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include/pico
${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include/
${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include/
${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include/
${PICO_SDK_PATH}/lib/lwip/src/include
${PICO_SDK_PATH}/lib/lwip/src/include/iwip
${PICO_SDK_PATH}/lib/cyw43-driver/src/        

)

# Add any user requested libraries
target_link_libraries(AS3935APP 
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_flash
        hardware_sync                
        hardware_rtc
        pico_cyw43_arch_lwip_poll
        )

pico_add_extra_outputs(AS3935APP)

