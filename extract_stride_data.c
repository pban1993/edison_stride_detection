/* for file and terminal I/O */
#include <stdio.h>
/* for string manip */
#include <string.h>
/* for exit() */
#include <stdlib.h>
/* for fabsf() */
#include <math.h>

#define BUFF_SIZE 1024

/*
 * sets first <n> values in <*arr> to <val>
 */
void clear_buffer(float *arr, float val, int n) 
{
	int i;
	for (i = 0; i < n; i++) {
		arr[i] = val;
	}
}

/*
 * Caculates mean of first <n> samples in <*arr>
 */
float calculate_mean(float *arr, int n)
{
	float total;
	int i;

	total = 0.0f;
	for (i = 0; i < n; i++) {
		total += arr[i];
	}

	return total/((float) n);
}

/*
 * Performs unsupervised learning on *arr to find two mean clusters.
 * Use the higher cluster value found in either thresholds[0] or thresholds[1]
 * to perform peak detection.
 */
int 
find_thresholds(
		float *arr, 	// signal
		int n_samples, 	// number of samples in the signal
		float e, 	// stop conidition for unsupervised learning
		// thresholds[2]
		// must intitialized before passing to this function
		// and used to store the return values of this function.
		float *thresholds
		)
{
	float c1, c2;
	float last_c1, last_c2;
	float *class1, *class2;
	int i, n_count1, n_count2;

	if (n_samples < 2) {
		fprintf(stderr, 
				"Not enough samples in array"
			        " to detect thresholds.\n"
				);
		return -1;
	}

	c1 = arr[0];
	c2 = arr[1];
	last_c1 = c1;
	last_c2 = c2;
	class1 = (float *) malloc(sizeof(float) * n_samples);
	class2 = (float *) malloc(sizeof(float) * n_samples);
	while (1) {
		n_count1 = 0;
		n_count2 = 0;
		clear_buffer(class1, 0.0f, n_samples);
		clear_buffer(class2, 0.0f, n_samples);
		for (i = 0; i < n_samples; i++) {
			if (fabsf(c1 - arr[i]) < fabsf(c2 - arr[i])) {
				class1[n_count1] = arr[i];
				n_count1++;
			} else {
				class2[n_count2] = arr[i];
				n_count2++;
			}
		}
		if (n_count1 > 0) {
			c1 = calculate_mean(class1, n_count1);
		} else {
			c1 = 0.0f;
		}
		if (n_count2 > 0) {
			c2 = calculate_mean(class2, n_count2);
		} else {
			c2 = 0.0f;
		}
		if ( (fabsf(last_c2 - c2) < e) && (fabsf(last_c1 - c1) < e) ) {
			thresholds[0] = c1;
			thresholds[1] = c2;
			free(class1);
			free(class2);
			return 0;
		}
		last_c1 = c1;
		last_c2 = c2;
	}
}

int 
find_peaks_and_troughs(
		float *arr, 	// signal 
		int n_samples, 	// number of samples present in the signal
		float E, 	// threshold for peak detection
		// arrays that will store the indicies of the located
		// peaks and troughs
		float *P, float *T,
		// number of peaks (n_P) and number of troughs (n_T)
		// found in the data set *arr
		int *n_P, int *n_T
		)
{
	int a, b, i, d, _n_P, _n_T;

	i = -1; d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

	clear_buffer(P, 0.0f, n_samples);
	clear_buffer(T, 0.0f, n_samples);

	while (i != n_samples) {
		i++;
		if (d == 0) {
			if (arr[a] >= (arr[i] + E)) {
				d = 2;
			} else if (arr[i] >= (arr[b] + E)) {
				d = 1;
			}
			if (arr[a] <= arr[i]) {
				a = i;
			} else if (arr[i] <= arr[b]) {
				b = i;
			}
		} else if (d == 1) {
			if (arr[a] <= arr[i]) {
				a = i;
			} else if (arr[a] >= (arr[i] + E)) {
				/*
				 * Peak has been detected.
				 * Add index at detected peak
				 * to array of peak indicies
				 * increment count of peak indicies
				 */
				P[_n_P] = a;
				_n_P++;
				b = i;
				d = 2;
			}
		} else if (d == 2) {
			if (arr[i] <= arr[b]) {
				b = i;
			} else if (arr[i] >= (arr[b] + E)) {
				/*
				 * Trough has been detected.
				 * Add index at detected trough
				 * to array of trough indicies
				 * increment count of trough indicies
				 */
				T[_n_T] = b;
				_n_T++;
				a = i;
				d = 1;
			}
		}
	}

	(*n_P) = _n_P;
	(*n_T) = _n_T;
	return 0;
}

int main(int argc, char **argv)
{
	/* Generic variables */
	int i, idx;
	int rv;
	/* Variables for reading file line by line */
	char *ifile_name, *ofile_pt_name, *ofile_st_name;
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int N_SAMPLES;

	/* Variables for storing the data and storing the return values */
	float *t, *x, *y, *z; // variables for data collected from input file
	/* Variables for threshold calculation */
	float thresholds[2], pk_threshold;
       	/* Variables for peak-trough detection */	
	float *P_i; 	// indicies of each peak found by peak detection
	float *T_i; 	// indicies of each trough found by trough detection
	float *S_i; 	// indicies of the start of each stride
	int n_P; 	// number of peaks
	int n_T; 	// number of troughs
	int n_S; 	// number of strides

	/*
	 * set this to 0 so that the code can function without
	 * having to actually performing stride detection
	 * from peaks and troughs
	 */
	n_S = 0; 
	
	/*
	 * Check if the user entered the correct command line arguments
	 * Usage: 
	 * ./extract_stride_data <ifile_name> <output_peaks> <output_strides>
	 * Or 
	 * ./extract_stride_data
	 */
	if (argc != 4) {
		ifile_name = (char *) malloc(sizeof(char) * BUFF_SIZE);
		memset(ifile_name, 0, BUFF_SIZE);
		snprintf(ifile_name, 
				BUFF_SIZE, 
				"Acceleration_Walk_Dataset.csv"
			);
		ofile_pt_name = (char *) malloc(sizeof(char) * BUFF_SIZE);
		memset(ofile_pt_name, 0, BUFF_SIZE);
		snprintf(ofile_pt_name, BUFF_SIZE, "acceleration_output.csv");
		ofile_st_name = (char *) malloc(sizeof(char) * BUFF_SIZE);
		memset(ofile_st_name, 0, BUFF_SIZE);
		snprintf(ofile_st_name, BUFF_SIZE, "acceleration_strides.csv");
	} else {
		ifile_name = argv[1];
		ofile_pt_name = argv[2];
		ofile_st_name = argv[3];
	}

	/* open the input file */
	printf("Attempting to read from file \'%s\'.\n", ifile_name);
	fp = fopen(ifile_name, "r");
	if (fp == NULL) {
		fprintf(stderr, 
				"Failed to read from file \'%s\'.\n", 
				ifile_name
		       );
		exit(EXIT_FAILURE);
	}

	/* count the number of lines in the file */
	read = getline(&line, &len, fp); //discard header of file
	N_SAMPLES = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		N_SAMPLES++;
	}

	/* go back to the start of the file so that the data can be read */
	rewind(fp);
	read = getline(&line, &len, fp); //discard header of file

	/* start reading the data from the file into the data structures */
	i = 0;
	t = (float *) malloc(sizeof(float) * N_SAMPLES);
	x = (float *) malloc(sizeof(float) * N_SAMPLES);
	y = (float *) malloc(sizeof(float) * N_SAMPLES);
	z = (float *) malloc(sizeof(float) * N_SAMPLES);
	while ((read = getline(&line, &len, fp)) != -1) {
		/* parse the data */
		rv = sscanf(line, "%f,%f,%f,%f\n", &t[i], &x[i], &y[i], &z[i]);
		if (rv != 4) {
			fprintf(stderr,
					"%s %d \'%s\'. %s.\n",
					"Failed to read line",
					i,
					line,
					"Exiting"
			       );
			exit(EXIT_FAILURE);
		}
		i++;
	}
	fclose(fp);


	/* Find peak thresholds */
	rv = find_thresholds(x, N_SAMPLES, 1e-5, thresholds);
	if (rv < 0) {
		fprintf(stderr, "find_thresholds failed\n");
		exit(EXIT_FAILURE);
	}

	printf("THRESHOLDS: %f %f\n", thresholds[0], thresholds[1]);

	/* From detected thresholds, find the max(abs(thresholds)) */
	if (fabsf(thresholds[0]) > fabsf(thresholds[1])) {
		pk_threshold = fabsf(thresholds[0]);
	} else {
		pk_threshold = fabsf(thresholds[1]);
	}

	/* 
	 * From detected thresholds, 
	 * find indicies of peaks
	 * find indicies of troughs
	 */
	printf("THRESHOLD SELECTED: %f\n", pk_threshold);
	P_i = (float *) malloc(sizeof(float) * N_SAMPLES);
	T_i = (float *) malloc(sizeof(float) * N_SAMPLES);
	rv = find_peaks_and_troughs(
			x, 
			N_SAMPLES, 
			pk_threshold, 
			P_i, T_i, 
			&n_P, &n_T);
	if (rv < 0) {
		fprintf(stderr, "find_peaks_and_troughs failed\n");
		exit(EXIT_FAILURE);
	}

	/* DO NOT MODIFY ANYTHING BEFORE THIS LINE */

	/* 
	 * Insert your algorithm to convert from a series of peak-trough
	 * indicies, to a series of indicies that indicate the start
	 * of a stride.
	 */



	/* DO NOT MODIFY ANYTHING AFTER THIS LINE */

	/* open the output file to write the peak and trough data */
	printf("Attempting to write to file \'%s\'.\n", ofile_pt_name);
	fp = fopen(ofile_pt_name, "w");
	if (fp == NULL) {
		fprintf(stderr, 
				"Failed to write to file \'%s\'.\n", 
				ofile_pt_name
		       );
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "P_i,P_t,P_x,T_i,T_t,T_p\n");
	for (i = 0; i < n_P || i < n_T; i++) {
		/* Only peak data if there is peak data to write */
		if (i < n_P) {
			idx = (int) P_i[i];
			fprintf(fp, "%d,%20.10lf,%lf,",
					idx,
					t[idx],
					x[idx]
			       );
		} else {
			fprintf(fp, ",,,");
		}
		/* Only trough data if there is trough data to write */
		if (i < n_T) {
			idx = (int) T_i[i];
			fprintf(fp, "%d,%20.10lf,%lf\n",
					idx,
					t[idx],
					x[idx]
			       );
		} else {
			fprintf(fp, ",,\n");
		}
	}
	fclose(fp);

	/* open the output file to write the stride data */
	printf("Attempting to write to file \'%s\'.\n", ofile_st_name);
	fp = fopen(ofile_st_name, "w");
	if (fp == NULL) {
		fprintf(stderr, 
				"Failed to write to file \'%s\'.\n", 
				ofile_st_name
		       );
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "S_i,S_t,S_x\n");
	for (i = 0; i < n_S; i++) {
		idx = (int) S_i[i];
		fprintf(fp, "%d,%20.10lf,%lf\n",
				idx,
				t[idx],
				x[idx]
		       );
	}
	fclose(fp);

	return 0;
}