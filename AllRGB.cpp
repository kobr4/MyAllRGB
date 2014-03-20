#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
	apr_thread_mutex_t 	* palette_mutex;
} T_ALLRGBJOB;

T_ALLRGBJOB g_allrgbjob;

apr_status_t g_thr_status;
apr_pool_t * g_pool;
unsigned char * g_buffer_image;

typedef struct {
	unsigned char id_length;
	unsigned char color_map_type;
	unsigned char image_type;
	unsigned char color_map_spec[5];
	unsigned char image_spec[10];
} t_tgaheader;

typedef struct {
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned char used;
} t_color;
void timer_stop();
void refresh_image();

void writeTGAoutput(unsigned char * map, int width, int height, char * filename) {
	FILE * pFile;
	fopen_s (&pFile, filename , "wb");
	t_tgaheader header;

	header.id_length = 0;
	header.color_map_type = 0;
	header.image_type = 2;
	header.color_map_spec[0] = 0;
	header.color_map_spec[1] = 0;
	header.color_map_spec[2] = 0;
	header.color_map_spec[3] = 0;
	header.color_map_spec[4] = 0;	
	header.image_spec[0] = 0;
	header.image_spec[1] = 0;
	header.image_spec[2] = 0;
	header.image_spec[3] = 0;
	header.image_spec[4] = width & 0xff;
	header.image_spec[5] = (width & 0xff00) >> 8;
	header.image_spec[6] = height & 0xff;
	header.image_spec[7] = (height & 0xff00) >> 8;
	header.image_spec[8] = 24;
	header.image_spec[9] = 0;

	fwrite(&header,sizeof(t_tgaheader),1,pFile);
	fwrite(map,sizeof(unsigned char)*width*height*3,1,pFile);
	fclose(pFile);
}

t_color * generatePalette(int r_range, int g_range, int b_range) {
	t_color * palette = (t_color*)malloc(sizeof(t_color) * r_range * g_range * b_range);
	int count = 0;
	for (int r = 0;r < r_range;r++) {
		for (int g = 0;g < g_range;g++) {
			for (int b = 0;b < b_range;b++) {
				palette[count].r = r;
				palette[count].g = g;
				palette[count].b = b;
				palette[count].used = 0;
				count++;
			}
		}
	}
	return palette;
}

void putpixel(unsigned char * bufferImage, int x,int y, t_color color) {
	int offset = (x + y * g_allrgbjob.width) * 3;
	bufferImage[offset] = (unsigned char)color.r;
	bufferImage[offset + 1] = (unsigned char)color.g;
	bufferImage[offset + 2] = (unsigned char)color.b;
}

int getpixel(unsigned char * bufferImage, int x,int y) {
	return bufferImage[(x + y * g_allrgbjob.width) * 3] + bufferImage[(x + y * g_allrgbjob.width) * 3 + 1] + bufferImage[(x + y * g_allrgbjob.width) * 3 + 2];
}

void getpixel(unsigned char * bufferImage, int x,int y,int * r,int * g, int * b) {
	*r = bufferImage[(x + y * g_allrgbjob.width) * 3]; 
	*g = bufferImage[(x + y * g_allrgbjob.width) * 3 + 1]; 
	*b = bufferImage[(x + y * g_allrgbjob.width) * 3 + 2];
}

void create_seeds(t_color * palette, unsigned char * bufferImage, int nb_element) {
	for (int i = 0;i < nb_element;i++) {
		int x = rand()%g_allrgbjob.width;
		int y = rand()%g_allrgbjob.height;
		int pos = rand()%g_allrgbjob.fullrange;
		while (palette[pos].used != 0) {
			pos = rand()%g_allrgbjob.fullrange;
		}
		putpixel(bufferImage,x,y,palette[pos]);
		palette[pos].used = 1;
	}
}

int palette_contains_unused(t_color * palette,int size) {
	for (int i = 0;i < size;i++) {
		if (palette[i].used == 0) {
			return 1;
		}
	}
	return 0;
}

int palette_count_used(t_color * palette,int size) {
	int count = 0;
	for (int i = 0;i < size;i++) {
		if (palette[i].used == 1) {
			count++;
		}
	}
	return count;
}

int compute_score(int r1,int g1,int b1, int r2,int g2,int b2) {
	//return (r1 - r2) * (r1 - r2) + (b1 - b2) * (b1 - b2) + (g1 - g2) * (g1 - g2);
	
	return abs(r1 - r2) + abs(b1 - b2) + abs(g1 - g2);
	/*
	int r = abs(r1 - r2);
	int b = abs(b1 - b2);
	int g = abs(g1 - g2);
	if (r > b && r > g) 
		return r;
	if (b > r && b > g)
		return b;
	return g;
	*/
	/*
	int score = (r1 - r2) * (r1 - r2) + (b1 - b2) * (b1 - b2) + (g1 - g2) * (g1 - g2);
	if ((r1 - r2) * (r1 - r2) > 8*8)
		score += (r1 - r2) * (r1 - r2);
	if ((g1 - g2) * (g1 - g2) > 8*8)
		score += (g1 - g2) * (g1 - g2);
	if ((b1 - b2) * (b1 - b2) > 8*8)
		score += (b1 - b2) * (b1 - b2);
	return score;
	*/
}

int find_closest_color_match(unsigned char * bufferImage,t_color * palette, int x, int y) {
	int r, g, b;
	getpixel(bufferImage, x,y,&r,&g, &b);
	int best_score = 0;
	int pos = 0;
	for (int i = 0;i < g_allrgbjob.fullrange;i++) {
		if (palette[i].used == 0) {
			int current_score = compute_score(palette[i].r,palette[i].g,palette[i].b, r, g, b);
			/*
			for (int u = x-1;u <= x+1;u++) {
				for (int v = y-1;v <= y+1;v++) {
					if (u < 0 || u >= g_width || v < 0 || v >= g_height) {
						getpixel(bufferImage, u,v,&r,&g, &b);
						current_score += compute_score(palette[i].r,palette[i].g,palette[i].b, r, g, b);
					}
				}
			}*/
			if ((best_score == 0) || (current_score  < best_score)) {
				best_score = current_score;
				pos = i;
			}
		}
	}
	return pos;
}

void fillpixel(unsigned char * bufferImage,t_color * palette) {
	int x = rand()%g_allrgbjob.width;
	int y = rand()%g_allrgbjob.height;
	while (getpixel(bufferImage,x,y) == 0) {
		x = rand()%g_allrgbjob.width;
		y = rand()%g_allrgbjob.height;
	}

	int sign = rand()%2;
	int incr = rand()%2; 
	int incrx,incry;
	if (incr == 0) {
		incrx = (sign==0? 1 : -1 );
		incry = 0;
	} else {
		incry = (sign==0? 1 : -1 );
		incrx = 0;
	}


	int new_x = x + incrx;
	int new_y = y + incry;
	if (new_x < 0 || new_x >= g_allrgbjob.width || new_y < 0 || new_y >= g_allrgbjob.height) {
		return;
	}

	while (getpixel(bufferImage,new_x,new_y) != 0) {
		x = new_x;
		y = new_y;
		new_x = x + incrx;
		new_y = y + incry;	
		if (new_x < 0 || new_x >= g_allrgbjob.width || new_y < 0 || new_y >= g_allrgbjob.height) {
			return;
		}
	}

	int pos = find_closest_color_match(bufferImage, palette,x,y);
	apr_thread_mutex_lock (g_allrgbjob.palette_mutex);
	if ((palette[pos].used == 0)&&(getpixel(bufferImage,new_x,new_y)==0)) {
		putpixel(bufferImage,new_x,new_y,palette[pos]);
		palette[pos].used = 1;
	} 
	apr_thread_mutex_unlock (g_allrgbjob.palette_mutex);
}

void copy_to_buffer_image(unsigned char * bufferImage) {
	for (int i = 0;i < g_allrgbjob.width * g_allrgbjob.height;i++) {
		g_buffer_image[i*3] = bufferImage[i*3] * 256 / g_allrgbjob.r_range; 
		g_buffer_image[i*3+1] = bufferImage[i*3+1] * 256 / g_allrgbjob.g_range; 
		g_buffer_image[i*3+2] = bufferImage[i*3+2] * 256 / g_allrgbjob.b_range; 
	}
	refresh_image();
}

void unit_process(t_color * palette, unsigned char * bufferImage) {
	int count = 0;
	while ((palette_count_used(palette,g_allrgbjob.fullrange) < g_allrgbjob.height * g_allrgbjob.width) 
		&& (palette_contains_unused(palette,g_allrgbjob.fullrange) == 1)) {
		fillpixel(bufferImage,palette);
		if (count%1000 == 0) {
			int count_unused=0;
			for (int i = 0;i < g_allrgbjob.fullrange;i++) {
				if (palette[i].used == 0) count_unused++;
			}

			copy_to_buffer_image(bufferImage);
		}
		
		count++;
	}
}

static void * APR_THREAD_FUNC thread_function( apr_thread_t * thread, void *data )
{
  const void ** datac =(const void **)data;
  unit_process((t_color *)datac[0], (unsigned char *)datac[1]);
  return NULL;
}



void fillBuffer(unsigned char * bufferImage) {
	t_color * palette = generatePalette(g_allrgbjob.r_range, g_allrgbjob.g_range, g_allrgbjob.b_range);
	create_seeds(palette, bufferImage, g_allrgbjob.nb_seeds);

	void * data[2];
	data[0] = palette;
	data[1] = bufferImage;

	for (int i = 0;i < g_allrgbjob.nb_thread;i++) {
		if( apr_thread_create(&g_allrgbjob.worker_threads[i], NULL, thread_function, data,  g_pool) != APR_SUCCESS ) 
		{
			printf( "Could not create the thread\n");
			exit( -1);
		}
	}
  
	for (int i = 0;i < g_allrgbjob.nb_thread;i++) {
		apr_thread_join( &g_thr_status, g_allrgbjob.worker_threads[i]);
	}

	delete palette;
	timer_stop();
}

void allrgb_generate() {
	srand (time(NULL));

	g_allrgbjob.worker_threads = (apr_thread_t **)malloc(sizeof(apr_thread_t *)*g_allrgbjob.nb_thread);
	g_allrgbjob.fullrange = g_allrgbjob.r_range * g_allrgbjob.g_range * g_allrgbjob.b_range;
	apr_thread_mutex_create (&g_allrgbjob.palette_mutex, APR_THREAD_MUTEX_DEFAULT, g_pool);
	unsigned char * bufferImage = (unsigned char *)calloc(3 * g_allrgbjob.width *g_allrgbjob.height,sizeof(char));
	fillBuffer(bufferImage);
	
	for (int i = 0;i < g_allrgbjob.width*g_allrgbjob.height;i++) {
		/*
		bufferImage[i*3] = bufferImage[i*3] * 256 / g_allrgbjob.r_range; 
		bufferImage[i*3+1] = bufferImage[i*3+1] * 256 / g_allrgbjob.g_range;
		bufferImage[i*3+2] = bufferImage[i*3+2] * 256 / g_allrgbjob.b_range;
		*/
		int r = bufferImage[i*3] * 256 / g_allrgbjob.r_range; 
		int g = bufferImage[i*3+1] * 256 / g_allrgbjob.g_range;
		int b = bufferImage[i*3+2] * 256 / g_allrgbjob.b_range;
		bufferImage[i*3] = b;
		bufferImage[i*3+1] = g;
		bufferImage[i*3+2] = r;
	}

	writeTGAoutput(bufferImage, g_allrgbjob.width, g_allrgbjob.height, "test.tga");

	delete bufferImage;
	g_allrgbjob.status = 0;
	delete g_allrgbjob.worker_threads;
}
/*
int main(int argc, char **argv) {

	if (apr_initialize() != APR_SUCCESS) 
	{
		printf( "Could not initialize\n");
		exit(-1);
	}

	if (apr_pool_create(&g_pool, NULL) != APR_SUCCESS) 
	{
		printf( "Could not allocate pool\n");
		exit( -1);
	}

	allrgb_generate();
	return 0;
}
*/
