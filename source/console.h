/*
 * console.h
 * Console output
 */

#pragma once

void console_clear();
void console_printf(const char *format, ...);

void console_init();
void console_deinit();
