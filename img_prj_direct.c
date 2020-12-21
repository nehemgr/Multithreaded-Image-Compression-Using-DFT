#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <complex.h>
#include <fftw3.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define WIDTH 16
#define HEIGHT 16
#define new_img_size img_width * img_height * img_channels
#define sq_index_column_xtreme ((img_width / WIDTH) + (img_width % WIDTH != 0)) 
#define sq_index_row_xtreme ((img_height / HEIGHT) + (img_height % HEIGHT != 0))
#define req_sq_row ((req_sq - 1) / sq_index_column_xtreme)
#define req_sq_column ((req_sq - 1) % sq_index_column_xtreme)
#define sq_column_excess (img_width - WIDTH * (sq_index_column_xtreme - 1))
#define sq_row_excess (img_height - HEIGHT * (sq_index_row_xtreme - 1))

unsigned char *img;
unsigned char *new_img;

int img_width, img_height, img_channels;

fftw_plan planner[8];

double tol;

int main(int argc, char *argv[]) {
if (argc != 4) {
	printf("img_prj: missing operands\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}

if (!(tol = abs(atoi(*(argv + 1))))) {
	printf("img_prj: cannot handle char input\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}
if ((tol < 1) || (tol > 10)) {
	printf("img_prj: compress_factor [1-10]\n");
	exit(0);
	}
tol *= 250;
	
if (((strstr(*(argv + 2), ".jpg") == NULL) && (strstr(*(argv + 2), ".jpeg") == NULL)) || ((strstr(*(argv + 3), ".jpg") == NULL) && (strstr(*(argv + 3), ".jpeg") == NULL))) {
	printf("img_prj: incorrect filename type\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}

img = stbi_load(*(argv + 2), &img_width, &img_height, &img_channels, 0);
if (img == NULL) {
	printf("Error in loading the image\n");
	exit(1);
	}
printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img_width, img_height, img_channels);

new_img = malloc(new_img_size);
if (new_img == NULL) {
	printf("Unable to allocate memory for the new image.\n");
	exit(1);
	}

unsigned char all_sq[sq_index_row_xtreme][sq_index_column_xtreme];

for (int i = 0; i < sq_index_row_xtreme; i++) {
	for (int j = 0; j < sq_index_column_xtreme; j++) {
		all_sq[i][j] = 0;
		}
	}

fftw_init_threads();
fftw_plan_with_nthreads(3);

planner[0] = fftw_plan_dft_2d(HEIGHT, WIDTH, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[1] = fftw_plan_dft_2d(HEIGHT, sq_column_excess, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[2] = fftw_plan_dft_2d(sq_row_excess, WIDTH, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);	
planner[3] = fftw_plan_dft_2d(sq_row_excess, sq_column_excess, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[4] = fftw_plan_dft_2d(HEIGHT, WIDTH, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);
planner[5] = fftw_plan_dft_2d(HEIGHT, sq_column_excess, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);
planner[6] = fftw_plan_dft_2d(sq_row_excess, WIDTH, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);	
planner[7] = fftw_plan_dft_2d(sq_row_excess, sq_column_excess, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);

clock_t t = clock();

int sq_row = 1, sq_column = 1;
unsigned char *p = img;
unsigned char *pg = new_img;
while (sq_row <= img_height) {
	for (; (sq_column <= img_width); p += img_channels, pg += img_channels) {
		*pg = *p;
		*(pg + 1) = *(p + 1);
		*(pg + 2) = *(p + 2);
		sq_column++;
		}
	sq_row++;
	sq_column = 1;
	}
sq_row = 1;
sq_column = 1;

fftw_complex *input;
int sq_row_size, sq_column_size;
int req_sq = 1;
while (req_sq <= sq_index_column_xtreme * sq_index_row_xtreme) {
	if ((req_sq_column + 1 == sq_index_column_xtreme) && (req_sq_row + 1 == sq_index_row_xtreme)) {
		sq_row_size = sq_row_excess;
		sq_column_size = sq_column_excess;
		}
	else if (req_sq_column + 1 == sq_index_column_xtreme) {
		sq_row_size = HEIGHT;
		sq_column_size = sq_column_excess;
		}
	else if (req_sq_row + 1 == sq_index_row_xtreme) {
		sq_row_size = sq_row_excess;
		sq_column_size = WIDTH;
		}
	else {
		sq_row_size = HEIGHT;
		sq_column_size = WIDTH;
		}
	input = (fftw_complex*) fftw_malloc(sq_row_size * sq_column_size * sizeof(fftw_complex));
	
	for (int i = 0; i < img_channels; i++) {
		p = img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + i;
		while (sq_row <= sq_row_size) {
			for (; sq_column <= sq_column_size; p += img_channels) {
				input[(sq_row - 1) * sq_column_size + (sq_column - 1)] = *p;
				sq_column++;
				}
			p = img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + (img_width * img_channels * sq_row) + i;
			sq_row++;
			sq_column = 1;
			}
		
		if ((req_sq_column + 1 == sq_index_column_xtreme) && (req_sq_row + 1 == sq_index_row_xtreme))
			fftw_execute_dft(planner[3], input, input);
		else if (req_sq_column + 1 == sq_index_column_xtreme)
			fftw_execute_dft(planner[1], input, input);
		else if (req_sq_row + 1 == sq_index_row_xtreme)
			fftw_execute_dft(planner[2], input, input);
		else
			fftw_execute_dft(planner[0], input, input);
			
		for (sq_row = 1; sq_row <= sq_row_size; sq_row++) {
			for (sq_column = 1; sq_column <= sq_column_size; sq_column++) {
				if (cabs(input[(sq_row - 1) * sq_column_size + (sq_column - 1)]) < tol)
					input[(sq_row - 1) * sq_column_size + (sq_column - 1)] = 0;
				}
			}
		
		if ((req_sq_column + 1 == sq_index_column_xtreme) && (req_sq_row + 1 == sq_index_row_xtreme))
			fftw_execute_dft(planner[7], input, input);
		else if (req_sq_column + 1 == sq_index_column_xtreme)
			fftw_execute_dft(planner[5], input, input);
		else if (req_sq_row + 1 == sq_index_row_xtreme)
			fftw_execute_dft(planner[6], input, input);
		else
			fftw_execute_dft(planner[4], input, input);
			
		sq_row = 1;
		sq_column = 1;
		pg = new_img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + i;
		while (sq_row <= sq_row_size) {
			for (; sq_column <= sq_column_size; pg += img_channels) {
					*pg = input[(sq_row - 1) * sq_column_size + (sq_column - 1)] / (sq_row_size * sq_column_size);
				sq_column++;
				}
			pg = new_img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + (img_width * img_channels * sq_row) + i;
			sq_row++;
			sq_column = 1;
			}
			
		sq_row = 1;
		sq_column = 1;
		}
	fftw_free(input);
	req_sq++;
	}

t = clock() - t;
double time_taken = ((double)t) / CLOCKS_PER_SEC;
printf("The program took %f seconds to execute \n", time_taken);

for (int i = 0; i < 8; i++)
	fftw_destroy_plan(planner[i]);
fftw_cleanup_threads();

stbi_write_jpg(*(argv + 3), img_width, img_height, img_channels, new_img, 65);
printf("Written image with a width of %dpx, a height of %dpx and %d channels\n", img_width, img_height, img_channels);

stbi_image_free(img);
stbi_image_free(new_img);
}
