#ifndef ASM_H
#define ASM_H


#ifdef _WIN32

#define aim_asm(asm) __asm { asm }

#else

#define aim_asm(asm) "__asm__(\"\n" #asm "\" : : )"

#endif


#endif // ASM_H