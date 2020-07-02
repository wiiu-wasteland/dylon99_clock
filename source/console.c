/*
 * console.c
 * Console output
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>

#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/cache.h>
#include <coreinit/time.h>
#include <proc_ui/procui.h>

#define CONSOLE_CALLBACK_PRIORITY	(100)
#define CONSOLE_FRAME_HEAP_TAG		(0x46545055)

typedef struct
{
	int rows, cols, offset;
	char *buffer;
	OSScreenID screen;
	void *screen_buf;
	size_t screen_buf_size;
} console_t;

static console_t cons[2] = {
	{.screen = SCREEN_TV, .rows = 27, .cols = 80, .offset = 0},
	{.screen = SCREEN_DRC, .rows = 18, .cols = 80, .offset = 0},
};

static MEMHeapHandle mem1_heap;
static bool in_foreground = false;

static void console_refresh()
{
	if (!in_foreground)
		return;

	for (int d = 0; d < 2; d++) {
		console_t *con = &cons[d];
		OSScreenClearBufferEx(con->screen, 0);

		char *buf = malloc(con->cols + 1);
		buf[con->cols] = '\0';
		for(int i = 0; i < con->rows; i++) {
			memcpy(buf, &con->buffer[con->cols  * i], con->cols);
			OSScreenPutFontEx(con->screen, 0, i, buf);
		}
		free(buf);

		DCFlushRange(con->screen_buf, con->screen_buf_size);
		OSScreenFlipBuffersEx(con->screen);
	}
}

uint32_t console_acquire_foreground(void *context)
{
	if (in_foreground)
		return 0;

	mem1_heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
	MEMRecordStateForFrmHeap(mem1_heap, CONSOLE_FRAME_HEAP_TAG);

	OSScreenInit();
	for (int d = 0; d < 2; d++) {
		console_t *con = &cons[d];
		con->screen_buf_size = OSScreenGetBufferSizeEx(con->screen);		
		con->screen_buf = MEMAllocFromFrmHeapEx(mem1_heap, con->screen_buf_size, 4);
		OSScreenSetBufferEx(con->screen, con->screen_buf);
		OSScreenEnableEx(con->screen, 1);		
	}

	in_foreground = true;
	console_refresh();

	return 0;
}

uint32_t console_release_foreground(void *context)
{
	if (!in_foreground)
		return 0;

	MEMFreeByStateToFrmHeap(mem1_heap, CONSOLE_FRAME_HEAP_TAG);
	in_foreground = false;

	return 0;
}

static void console_write_char(char c)
{
	for (int d = 0; d < 2; d++) {
		console_t *con = &cons[d];

		if (con->offset >= (con->rows * con->cols)) {
			unsigned shift = (con->rows - 1) * con->cols;
			memcpy(con->buffer, &con->buffer[con->cols], shift);
			memset(&con->buffer[shift], ' ', con->cols);
			con->offset -= con->cols;
		}

		switch(c) {
			case '\n':	con->offset += con->cols - (con->offset % con->cols); break;
			case '\t':	con->offset += 4 - (con->offset % 4); break;
			case '\b':	con->buffer[--con->offset] = ' '; break;
			default:	con->buffer[con->offset++] = c; break;
		}
	}
}

void console_clear()
{
	for (int d = 0; d < 2; d++) {
		console_t *con = &cons[d];
		memset(con->buffer, ' ', con->cols * con->rows);
	}

	console_refresh();
}

void console_printf(const char *format, ...)
{
	char * tmp = NULL;
	va_list va;

	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
		for (int i = 0; tmp[i]; i++)
			console_write_char(tmp[i]);
		free(tmp);
	}
	va_end(va);

	console_refresh();
}

void console_init()
{
	cons[0].buffer = malloc(cons[0].rows * cons[0].cols);
	cons[1].buffer = malloc(cons[1].rows * cons[1].cols);
	console_clear();

	console_acquire_foreground(NULL);

	ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, console_acquire_foreground,
						   NULL, CONSOLE_CALLBACK_PRIORITY);
	ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, console_release_foreground,
						   NULL, CONSOLE_CALLBACK_PRIORITY);
}

void console_deinit()
{
	ProcUIClearCallbacks();

	console_release_foreground(NULL);
	
	free(cons[0].buffer);
	free(cons[1].buffer);
}
