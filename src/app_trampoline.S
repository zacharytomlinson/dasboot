// Interrupt vector trampoline located at the start of APPCODE.
//
// On ATmega4808, after reset the interrupt vector base is the start of the APPCODE section.
// We place a vector table there that:
// - jumps to the bootloader reset handler at 0x0000
// - forwards all other interrupts to the application's relocated vector table
//   (linked to start at APP_START_ADDR).

#ifndef BOOT_SECTION_SIZE
#define BOOT_SECTION_SIZE 4096
#endif

#ifndef APP_TRAMPOLINE_SIZE
#define APP_TRAMPOLINE_SIZE 256
#endif

#define APP_START_ADDR (BOOT_SECTION_SIZE + APP_TRAMPOLINE_SIZE)
#define VECTOR_SIZE 4

    .section .appvectors, "ax", @progbits
    .global __app_trampoline_vectors

__app_trampoline_vectors:
    // Vector 0: Reset -> bootloader at 0x0000
    jmp 0x0000

    // Vectors 1..35: forward to application's vector table (relocated to APP_START_ADDR)
    jmp (APP_START_ADDR + (VECTOR_SIZE * 1))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 2))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 3))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 4))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 5))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 6))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 7))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 8))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 9))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 10))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 11))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 12))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 13))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 14))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 15))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 16))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 17))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 18))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 19))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 20))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 21))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 22))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 23))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 24))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 25))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 26))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 27))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 28))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 29))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 30))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 31))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 32))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 33))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 34))
    jmp (APP_START_ADDR + (VECTOR_SIZE * 35))
