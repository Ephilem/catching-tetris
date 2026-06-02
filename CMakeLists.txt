cmake_minimum_required(VERSION 3.20)

# --- Toolchain ARM ----------------------------------------------------------
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(Project C ASM)

# --- Flags CPU/MCU ----------------------------------------------------------
set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb")

set(CMAKE_C_FLAGS   "${CPU_FLAGS} -ffunction-sections -fdata-sections -Wall -fno-common -std=gnu90")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp -DRAM_MODE=0 -g")

# Defines issus du .uvprojx (section Aads)
add_compile_definitions(
    __BUILD_WITH_EXAMPLE__=1
    __RAM_MODE__=0
)

set(CMAKE_C_FLAGS_DEBUG   "-Og -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# --- Linker -----------------------------------------------------------------
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/LPC1768.ld")

set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} \
     -T${LINKER_SCRIPT} \
     -Wl,--gc-sections \
     -Wl,-Map=${PROJECT_NAME}.map \
     -Wl,--print-memory-usage \
     --specs=nano.specs \
     --specs=nosys.specs")

# --- Includes ---------------------------------------------------------------
include_directories(
    compat
    Libraries/LCP1768_StdPeriph_Driver/include
    Libraries/CM3/CoreSupport
    Libraries/CM3/DeviceSupport/NXP/LPC17xx
    User
    User/touch
)

# --- Sources (exactement celles référencées dans Project.uvprojx) -----------
set(SOURCES
    # Startup + CMSIS device
    Libraries/CM3/DeviceSupport/NXP/LPC17xx/startup/gcc/startup_LPC17xx.s
    Libraries/CM3/DeviceSupport/NXP/LPC17xx/system_LPC17xx.c
    Libraries/CM3/CoreSupport/core_cm3.c

    # StdPeriph — uniquement les drivers utilisés dans le projet
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_clkpwr.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_gpio.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_pinsel.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_timer.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_exti.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_nvic.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_ssp.c
    Libraries/LCP1768_StdPeriph_Driver/source/lpc17xx_i2c.c

    # Touch / LCD
    User/touch/ili_lcd_general.c
    User/touch/lcd_api.c
    User/touch/english_16x8.c
    User/touch/touch_panel.c

    # Application
    User/affichagelcd.c
    User/memory.c
    User/timer.c
    User/main.c
    User/hardfault_handler.c
        User/joystick.c
        User/joystick.h
        User/math.h
        User/game.c
        User/game.h
        User/render.c
        User/render.h
        User/sprites_data.c
        User/sprites_data.h
)

# --- Executable -------------------------------------------------------------
add_executable(${PROJECT_NAME}.elf ${SOURCES})

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE}    $<TARGET_FILE:${PROJECT_NAME}.elf>
    VERBATIM
)