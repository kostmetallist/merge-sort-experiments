#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>  // only for time processing

#define  MAX_NUMBER 1000

void merge(int *, int *, int, int, int);
void merge_sort(int *, int *, int, int);

int main(int argc, char **argv) 
{   
	int array_len  = atoi(argv[1]);
	int *unsorted  = (int *) malloc(array_len * sizeof(int));
	int *tmp_array = (int *) malloc(array_len * sizeof(int));
	int *sorted;

	srand(time(NULL));
	printf("Unsorted: ");

	for (int i = 0; i < array_len; i++) 
	{   
		unsorted[i] = rand() % MAX_NUMBER;
		printf("%d ", unsorted[i]); 
	}   

	printf("\n");
	printf("\n");
	//clock_t timer_start = clock();
	double timer_start = omp_get_wtime();

	merge_sort(unsorted, tmp_array, 0, (array_len - 1));
	sorted = unsorted;

	printf("Sorted: ");

	for (int i = 0; i < array_len; i++)
		printf("%d ", sorted[i]);

	printf("\n");
	printf("\n");

	free(unsorted);
	free(tmp_array);

	//clock_t timer_end = clock();
	double timer_end = omp_get_wtime();
	//fprintf(stderr, "Execution time: %f\n", 
	//		(double) (timer_end-timer_start) / CLOCKS_PER_SEC);
	fprintf(stderr, "Execution time: %f\n", 
			(double) (timer_end-timer_start));

	return 0;
}

void merge(int *base_array, int *aux_array, int left, int mid, int right)
{
	int h, i, j, k;

	h = left;
	i = left;
	j = mid + 1;

	while ((h <= mid) && (j <= right))
	{
		if (base_array[h] <= base_array[j])
		{
			aux_array[i] = base_array[h];
			h++;
		}

		else
		{
			aux_array[i] = base_array[j];
			j++;
		}

		i++;
	}

	if (mid < h)
	{
		for (k = j; k <= right; k++)
		{
			aux_array[i] = base_array[k];
			i++;
		}
	}

	else
	{
		for (k = h; k <= mid; k++)
		{
			aux_array[i] = base_array[k];
			i++;
		}
	}

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
