#pragma once

#include <apr.h>
#include <apr_thread_proc.h>

typedef struct {
	int status;
	int width;
	int height;
	int r_range;
	int g_range;
	int b_range;
	int nb_thread;
	int fullrange;
	int nb_seeds;
	apr_thread_t ** worker_threads;
} T_ALLRGBJOB;

extern apr_pool_t * g_pool;
extern unsigned char * g_buffer_image;
extern T_ALLRGBJOB g_allrgbjob;
void allrgb_generate();