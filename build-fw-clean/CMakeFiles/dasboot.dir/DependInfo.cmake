
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/home/arch/CLionProjects/dasboot/firmware/src/app_trampoline.S" "/home/arch/CLionProjects/dasboot/build-fw-clean/CMakeFiles/dasboot.dir/src/app_trampoline.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "APP_TRAMPOLINE_SIZE=0x0100"
  "BOOT_SECTION_SIZE=0x1000"
  "F_CPU=20000000UL"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "/home/arch/CLionProjects/dasboot/firmware/src"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/home/arch/CLionProjects/dasboot/firmware/src/boot_eeprom.c" "CMakeFiles/dasboot.dir/src/boot_eeprom.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/boot_eeprom.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/drivers/atecc608a_bl.c" "CMakeFiles/dasboot.dir/src/drivers/atecc608a_bl.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/drivers/atecc608a_bl.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/drivers/w25q.c" "CMakeFiles/dasboot.dir/src/drivers/w25q.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/drivers/w25q.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/main.c" "CMakeFiles/dasboot.dir/src/main.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/main.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/mcu.c" "CMakeFiles/dasboot.dir/src/mcu.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/mcu.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/nvm_flash.c" "CMakeFiles/dasboot.dir/src/nvm_flash.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/nvm_flash.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/slots.c" "CMakeFiles/dasboot.dir/src/slots.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/slots.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/spi.c" "CMakeFiles/dasboot.dir/src/spi.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/spi.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/status_led.c" "CMakeFiles/dasboot.dir/src/status_led.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/status_led.c.obj.d"
  "/home/arch/CLionProjects/dasboot/firmware/src/twi0.c" "CMakeFiles/dasboot.dir/src/twi0.c.obj" "gcc" "CMakeFiles/dasboot.dir/src/twi0.c.obj.d"
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_LINKED_INFO_FILES
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_FORWARD_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
