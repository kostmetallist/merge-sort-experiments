#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define  MAX_NUMBER   1000
#define  THREAD_MAGIC 80

// describes domains of array for the each thread
typedef struct
{
	int arr_beg;
	int arr_end;
} Thr_info;

void merge(int *, int *, int, int, int);
void merge_sort(int *, int *, int, int);

int main(int argc, char **argv) 
{   
	int array_len  = atoi(argv[1]);
	int *unsorted  = (int *) malloc(array_len * sizeof(int));
	int *tmp_array = (int *) malloc(array_len * sizeof(int));
	int *sorted;

	int i;

	srand(time(NULL));
	printf("Unsorted: ");

	for (i = 0; i < array_len; i++) 
	{   
		unsorted[i] = rand() % MAX_NUMBER;
		printf("%d ", unsorted[i]); 
	}   

	printf("\n");
	printf("\n");
	//clock_t timer_start = clock();
	double timer_start = omp_get_wtime();
	Thr_info thread_data[THREAD_MAGIC];

	#pragma omp parallel default(none) \
	shared(array_len, unsorted, tmp_array, thread_data, stderr)
	{
		int thr_id     = omp_get_thread_num();
		int thr_num    = omp_get_num_threads();
		int chunk_size = array_len / thr_num;

		int thr_begin  = thr_id * chunk_size;
		int thr_end    = (thr_id == thr_num-1)? 
						 (array_len-1): 
						 ((thr_id+1) * chunk_size - 1);

		// memorizing for future merging
		#pragma omp critical
		{
			thread_data[thr_id].arr_beg = thr_begin;
			thread_data[thr_id].arr_end = thr_end;
		}

		
		//fprintf(stderr, "Thread #%d, range %d-%d\n", 
		//		thr_id, thr_begin, thr_end);
		merge_sort(unsorted, tmp_array, thr_begin, thr_end);
		#pragma omp barrier

		int factor  = 2;
		int merging = thr_num/2 + thr_num%2;

		while (merging)
		{
			if (thr_id % factor == 0)
			{
				// here we have continious area of arrays, 
				// so thread_data[thr_id].arr_beg == 
				// thread_data[thr_id+1].arr_beg - 1

				int pair_id    = thr_id + factor/2;
				Thr_info first = thread_data[thr_id];

				// merge arrays for threads having
				// neighboud after them (i.e. here
				// don't need to perform merge() on
				// the last array in case of odd
				// number of active threads)

				if (pair_id < thr_num)
				{
					Thr_info second = thread_data[pair_id];

					merge(unsorted, tmp_array, 
						  first.arr_beg, 
						  first.arr_end, 
						  second.arr_end);
					
					// extending size of array related to
					// certain thread
					first.arr_end = second.arr_end;
				}

				// reassigning borders of arrays per thread
				#pragma omp critical
				{
					thread_data[thr_id] = first;
					//fprintf(stderr, "\nMerge in new thr_id %d, range %d-%d\n", 
					//	thr_id, first.arr_beg, first.arr_end);
				}
			}

			// for making sure that threads we removed
			// from considering won't interfere in merging
			else 
				thr_id = -1;

			factor *= 2;
			merging = (merging == 1)? 0: (merging/2 + merging%2);

			// now, we need to wait for the other 
			// threads to proceed for keeping memory consistency
			#pragma omp barrier
		}
	}

	sorted = unsorted;
	printf("Sorted: ");

	for (i = 0; i < array_len; i++)
		printf("%d ", sorted[i]);

	printf("\n");
	printf("\n");

	free(unsorted);
	free(tmp_array);

	//clock_t timer_end = clock();
	double timer_end  = omp_get_wtime();
	fprintf(stderr, "Execution time: %f\n", 
			(double) (timer_end-timer_start));

	return 0;
}

void merge(int *base_array, int *aux_array, int left, int mid, int right)
{
	int h, j, u;

	h = left;
	j = mid + 1;

	// for passing through auxiliary array
	u = left;

	while ((h <= mid) && (j <= right))
	{
		if (base_array[h] <= base_array[j])
		{
			aux_array[u] = base_array[h];
			h++;
		}

		else
		{
			aux_array[u] = base_array[j];
			j++;
		}

		u++;
	}

	int k;

	// if we have run the 1st half
	if (mid < h)
	{
		for (k = j; k <= right; k++)
		{
			aux_array[u] = base_array[k];
			u++;
		}
	}

	else
	{
		for (k = h; k <= mid; k++)
		{
			aux_array[u] = base_array[k];
			u++;
		}
	}

	// flushing data to base array
	for (k = left; k <= right; k++)
		base_array[k] = aux_array[k];
}

void merge_sort(int *base_array, int *aux_array, int left, int right)
{
	int mid;

	if (left < right)
	{
		mid = (left+right) / 2;

		merge_sort(base_array, aux_array, left, mid);
		merge_sort(base_array, aux_array, (mid + 1), right);
		merge(base_array, aux_array, left, mid, right);
	}
}
