#include "println/println.h"

#include <stdarg.h>

extern int println(void) {
    return printf("\n");
}

extern int fprintln(FILE* stream) {
    return fprintf(stream, "\n");
}

extern int printfln(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintfln(format, args);
    va_end(args);
    return result;
}

extern int fprintfln(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintfln(stream, format, args);
    va_end(args);
    return result;
}

extern int vprintfln(const char* format, va_list args) {
    return vfprintfln(stdout, format, args);
}

extern int vfprintfln(FILE* stream, const char* format, va_list args) {
    int result = vfprintf(stream, format, args);
    if (result < 0) {
        return result;
    }
    return result + fprintf(stream, "\n");
}
