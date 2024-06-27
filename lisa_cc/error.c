// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lisacc.h"

bool enable_warning = true;
bool warning_is_error = false;

#ifdef STD_P16CC
static void print_error(char *line, char *pos, char *label, char *fmt, va_list args) {
    fprintf(stderr, isatty(fileno(stderr)) ? "%s:%s: \e[1;31m[%s]\e[0m " : "%s:%s: [%s] ",
        line, pos, label);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void errorf(char *line, char *pos, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_error(line, pos, "ERROR", fmt, args);
    va_end(args);
    exit(1);
}

void warnf(char *line, char *pos, char *fmt, ...) {
    if (!enable_warning)
        return;
    char *label = warning_is_error ? "ERROR" : "WARN";
    va_list args;
    va_start(args, fmt);
    print_error(line, pos, label, fmt, args);
    va_end(args);
    if (warning_is_error)
        exit(1);
}
#else
static void print_error(char *file, int line, char *pos, char *label, char *fmt, va_list args) {
    fprintf(stderr, isatty(fileno(stderr)) ? "\e[1;31m[%s]\e[0m " : "[%s] ", label);
    if (pos == NULL || strlen(pos) == 0)
      fprintf(stderr, "%s:%d: ", file, line);
    else
      fprintf(stderr, "%s: ", pos);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void errorf(char *file, int line, char *pos, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_error(file, line, pos, "ERROR", fmt, args);
    va_end(args);
    exit(1);
}

void warnf(char *file, int line, char *pos, char *fmt, ...) {
    if (!enable_warning)
        return;
    char *label = warning_is_error ? "ERROR" : "WARN";
    va_list args;
    va_start(args, fmt);
    print_error(file, line, pos, label, fmt, args);
    va_end(args);
    if (warning_is_error)
        exit(1);
}
#endif

char *token_pos(Token *tok) {
    File *f = tok->file;
    if (!f)
        return "(unknown)";
    char *name = f->name ? f->name : "(unknown)";
    return format("%s:%d:%d", name, tok->line, tok->column);
}
