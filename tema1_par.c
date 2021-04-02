/*
	Duracoska Keti 332CB
	TEMA1 APD
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int P;
int width;
int height;
int **result;
pthread_barrier_t barrier;


// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;
params *par;


// citeste argumentele programului
void get_args(int argc, char **argv) {
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	// numarul de thread-uri
	P = atoi(argv[5]);
}
// citeste fisierul de intrare
void read_input_file(char *in_filename, params *par) {
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}
// aloca memorie pentru rezultat
int **allocate_memory(int width, int height) {
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height) {
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
	result = NULL;
}

// returneaza valoarea minima
int min(int a, int b) {
    if(a < b) {
        return a;
    }
    return b;
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename) {
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}
	fprintf(file, "P2\n%d %d\n255\n", width, height);

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}
	fclose(file);
}

void calculate_julia(long id) {
	int w, h, step;
	// calculate the start and end for every thread
	int start = id * width / P;
    int end = min((id + 1) * width / P, width);

    for (w = start; w < end; w++) {
    	for (h = 0; h < height; h++) {
    		step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };
				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}
			result[h][w] = step % 256;
    	}
    }
}

void calculate_coordinates(long id) {
	// calculate the start and end for every thread
	int start_coord = id * (height/2) / P;
    int end_coord = min((id + 1) * (height /2) / P, (height/2));
    int i, *aux;

    for (i = start_coord; i < end_coord; i++) {
		aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
}

void calculate_mandelbrot(long id) {
	// calculate the start and end for every thread
	int start = id * width / P;
    int end = min((id + 1) * width / P, width);
    int w, h, step;
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}
}

void* thread_function(void *arg) {
	long id = *(long*) arg;
	// just one thread reads the input
    if (id == 1) {
    	read_input_file(in_filename_julia, par);
    	width = (par->x_max - par->x_min) / par->resolution;
		height = (par->y_max - par->y_min) / par->resolution;
		result = allocate_memory(width, height);
    }
    // wait while whole file is read
    pthread_barrier_wait(&barrier);
    // start with calculation for julia set
    calculate_julia(id);
    // wait while every thread had finished
    pthread_barrier_wait(&barrier); 
    // transform the result coordinates
    calculate_coordinates(id);
    // wait
	pthread_barrier_wait(&barrier);
	// write the output only once
	if (id == 1) {
		write_output_file(out_filename_julia);
	}
	pthread_barrier_wait(&barrier);
	// free memory only once
	if (id == 1) {
		free_memory(result, height);
	}
	pthread_barrier_wait(&barrier);
	// read the input for mandelbrot set
	if (id == 1) {
		read_input_file(in_filename_mandelbrot, par);
		// alloc result
		width = (par->x_max - par->x_min) / par->resolution;
		height = (par->y_max - par->y_min) / par->resolution;
		result = allocate_memory(width, height);
	}
	pthread_barrier_wait(&barrier);
	// wait while every thread calculate the mandelbrot set
	calculate_mandelbrot(id);
	pthread_barrier_wait(&barrier);
	// transform the result
	calculate_coordinates(id);
	pthread_barrier_wait(&barrier);
	// write the output
	if (id == 1) {
		write_output_file(out_filename_mandelbrot);
	}
	pthread_barrier_wait(&barrier);
	if (id == 1) {
		free_memory(result, height);
	}
	pthread_barrier_wait(&barrier);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	// se citesc argumentele programului
	par = malloc(sizeof(params));
	get_args(argc, argv);
	int r;
	// init the barrier
	r = pthread_barrier_init(&barrier, NULL, P);
	if(r) {
		printf("Eroare la pthread_barrier_init\n");
		exit(-1);
	}
	// create the threads
	pthread_t threads[P];
	long id;
	void *status;
	long arguments[P];

	for (id = 0; id < P; id++) {
		arguments[id] = id;

		r = pthread_create(&threads[id], NULL, thread_function, &arguments[id]);
		if(r) {
			printf("Eroare la crearea thread-ului %ld\n",id);
			exit(-1);
		}
	}
	// join the threads
	for (id = 0; id < P; id++) {

		r = pthread_join(threads[id], &status);
		if(r) {
			printf("Eroare la asteptarea thread-ului %ld\n",id);
			exit(-1);
		}
	}

	// destroy the barrier
	r = pthread_barrier_destroy(&barrier);
	if(r) {
		printf("Eroare la pthread_barrier_destroy\n");
		exit(-1);
	}

	pthread_exit(NULL);
}
