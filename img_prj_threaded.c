//This program is used to compress an image given a compression factor and uses safe multithreading to implement this

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

pthread_mutex_t mutex;

fftw_plan planner[8];

double tol;

void *copy_runner(void *param);
void *calc_runner(void *param);

int main(int argc, char *argv[]) {
// Get the neccessary data
if (argc != 5) {
	printf("img_prj: missing operands\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}

int num_threads;
if (!(num_threads = abs(atoi(*(argv + 1)))) || !(tol = abs(atoi(*(argv + 2))))) {
	printf("img_prj: cannot handle char input\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}
if ((tol < 1) || (tol > 10)) {
	printf("img_prj: compress_factor [1-10]\n");
	exit(0);
	}
tol *= 250; // Scale up tolerance factor
	
if (((strstr(*(argv + 3), ".jpg") == NULL) && (strstr(*(argv + 3), ".jpeg") == NULL)) || ((strstr(*(argv + 4), ".jpg") == NULL) && (strstr(*(argv + 4), ".jpeg") == NULL))) {
	printf("img_prj: incorrect filename type\n");
	printf("syntax: <num_threads> <compress_factor [1-10]> <input_img.jpg> <output_img.jpg>\n");
	exit(0);
	}
	
// Loading image
img = stbi_load(*(argv + 3), &img_width, &img_height, &img_channels, 0);
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

// Setup the sub-images status array
unsigned char all_sq[sq_index_row_xtreme][sq_index_column_xtreme];

for (int i = 0; i < sq_index_row_xtreme; i++) {
	for (int j = 0; j < sq_index_column_xtreme; j++) {
		all_sq[i][j] = 0;
		}
	}

// Setup the FFTW multithreading
fftw_init_threads();
fftw_plan_with_nthreads(3);

// Setup the DFT plans of required size
planner[0] = fftw_plan_dft_2d(HEIGHT, WIDTH, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[1] = fftw_plan_dft_2d(HEIGHT, sq_column_excess, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[2] = fftw_plan_dft_2d(sq_row_excess, WIDTH, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);	
planner[3] = fftw_plan_dft_2d(sq_row_excess, sq_column_excess, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
planner[4] = fftw_plan_dft_2d(HEIGHT, WIDTH, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);
planner[5] = fftw_plan_dft_2d(HEIGHT, sq_column_excess, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);
planner[6] = fftw_plan_dft_2d(sq_row_excess, WIDTH, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);	
planner[7] = fftw_plan_dft_2d(sq_row_excess, sq_column_excess, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE);

clock_t t = clock();

// Setup the necessary threads
pthread_t thread_id[num_threads + 1];
pthread_attr_t attr;
pthread_mutex_init(&mutex, NULL);
pthread_attr_init(&attr);

pthread_create(&thread_id[0], &attr, copy_runner, (int *)all_sq);

for (int i = 1; i < num_threads + 1; i++)
	pthread_create(&thread_id[i], &attr, calc_runner, (int *)all_sq);

for (int i = 0; i < num_threads + 1; i++)
	pthread_join(thread_id[i], NULL);
	
t = clock() - t;
double time_taken = ((double)t) / CLOCKS_PER_SEC;
printf("The program took %f seconds to execute \n", time_taken);

// Destroy FFTW threads and plans
for (int i = 0; i < 8; i++)
	fftw_destroy_plan(planner[i]);
fftw_cleanup_threads();

// Writing image
stbi_write_jpg(*(argv + 4), img_width, img_height, img_channels, new_img, 65);
printf("Written image with a width of %dpx, a height of %dpx and %d channels\n", img_width, img_height, img_channels);

// Free up allocated memory for images
stbi_image_free(img);
stbi_image_free(new_img);
}

void *copy_runner(void *param) {
unsigned char *all_sq_ptr = param;
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
		
	// After one row of sub-images is completed - update the sub-images status array
	if ((sq_row % HEIGHT == 0) || (sq_row == img_height)) {
		int cur_row = (sq_row / HEIGHT) + (sq_row % HEIGHT != 0);
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < sq_index_column_xtreme; i++)
			*((all_sq_ptr + (cur_row - 1) * sq_index_column_xtreme) + i) = 1;
		pthread_mutex_unlock(&mutex);
		}
	sq_row++;
	sq_column = 1;
	}
pthread_exit(0);
}

void *calc_runner(void *param) {
unsigned char *all_sq_ptr = param;
int sq_row = 1, sq_column = 1, req_sq = -1, empty_flg = 1;
while (1) {
	// Scan the sub-images status array for available sub-images in new image
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < sq_index_row_xtreme; i++) {
		for (int j = 0; j < sq_index_column_xtreme; j++) {
			if (*((all_sq_ptr + i * sq_index_column_xtreme) + j) == 1) {
				req_sq = (i * sq_index_column_xtreme) + j + 1;
				*((all_sq_ptr + i * sq_index_column_xtreme) + j) = 2;
				i = sq_index_row_xtreme;
				break;
				}
			if (*((all_sq_ptr + i * sq_index_column_xtreme) + j) == 0)
				empty_flg = 0;
			}
		}
	pthread_mutex_unlock(&mutex);
	
	// Die gracefully if no more sub-images' DFT calculations are left
	if ((req_sq == -1) && (empty_flg == 1))
		break;
		
	// Start calculating DFT of acquired sub-image
	else if (req_sq > 0) {
		// Set appropriate dimensions of sub-image
		int sq_row_size, sq_column_size;
		fftw_complex *input;
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
		
		// Repeat for as many image channels
		for (int i = 0; i < img_channels; i++) {
			// Copy pixels of sub-image from new image into input
			unsigned char *p = img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + i;
			while (sq_row <= sq_row_size) {
				for (; sq_column <= sq_column_size; p += img_channels) {
					input[(sq_row - 1) * sq_column_size + (sq_column - 1)] = *p;
					sq_column++;
					}
				p = img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + (img_width * img_channels * sq_row) + i;
				sq_row++;
				sq_column = 1;
				}
			
			// Calculate DFT of sub-image in input
			if ((req_sq_column + 1 == sq_index_column_xtreme) && (req_sq_row + 1 == sq_index_row_xtreme))
				fftw_execute_dft(planner[3], input, input);
			else if (req_sq_column + 1 == sq_index_column_xtreme)
				fftw_execute_dft(planner[1], input, input);
			else if (req_sq_row + 1 == sq_index_row_xtreme)
				fftw_execute_dft(planner[2], input, input);
			else
				fftw_execute_dft(planner[0], input, input);
			
			// Drop Fourier coefficients below the threshold
			for (sq_row = 1; sq_row <= sq_row_size; sq_row++) {
				for (sq_column = 1; sq_column <= sq_column_size; sq_column++) {
					if (cabs(input[(sq_row - 1) * sq_column_size + (sq_column - 1)]) < tol)
						input[(sq_row - 1) * sq_column_size + (sq_column - 1)] = 0;
				}
			}
			
			// Calculate inverse DFT of frequencies in input
			if ((req_sq_column + 1 == sq_index_column_xtreme) && (req_sq_row + 1 == sq_index_row_xtreme))
				fftw_execute_dft(planner[7], input, input);
			else if (req_sq_column + 1 == sq_index_column_xtreme)
				fftw_execute_dft(planner[5], input, input);
			else if (req_sq_row + 1 == sq_index_row_xtreme)
				fftw_execute_dft(planner[6], input, input);
			else
				fftw_execute_dft(planner[4], input, input);
			
			// Copy pixels of sub-image from input into new image
			sq_row = 1;
			sq_column = 1;
			unsigned char *pg = new_img + (img_width * img_channels * HEIGHT * req_sq_row) + (img_channels * WIDTH * req_sq_column) + i;
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
			
		// Free up allocated memory for sub-image
		fftw_free(input);
		}
		
	// Reset flags
	req_sq = -1;
	empty_flg = 1;
	}
pthread_exit(0);
}
