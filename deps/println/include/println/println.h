#ifndef PRINTLN_H_
#define PRINTLN_H_

#include <hedley/hedley.h>
#include <stdio.h>

extern int println(void);
extern int fprintln(FILE* stream);
extern int printfln(const char* format, ...) HEDLEY_PRINTF_FORMAT(1, 2);
extern int fprintfln(FILE* stream, const char* format, ...) HEDLEY_PRINTF_FORMAT(2, 3);
extern int vprintfln(const char* format, va_list args);
extern int vfprintfln(FILE* stream, const char* format, va_list args);

#endif  // PRINTLN_H_
