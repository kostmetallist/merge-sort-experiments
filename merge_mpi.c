#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define  MAX_NUMBER 1000

void merge(int *, int *, int, int, int);
void merge_sort(int *, int *, int, int);

int main(int argc, char **argv) 
{   
	int array_len = atoi(argv[1]);
	int *unsorted;
	int rank, comm_size;
	double timer_start, timer_end;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

	if (rank == 0)
	{
		fprintf(stderr, "Number of processes: %d\n", comm_size);
		unsorted = malloc(array_len * sizeof(int));
	
		srand(time(NULL));
		printf("Unsorted: ");
	
		int i;
	
		for (i = 0; i < array_len; i++) 
		{   
			unsorted[i] = rand() % MAX_NUMBER;
			printf("%d ", unsorted[i]); 
		}   
	
		printf("\n");
		printf("\n");
		timer_start = MPI_Wtime();
	}

	MPI_Barrier(MPI_COMM_WORLD);

	// Implementing size management:
	// dividing all the work to equal chunks
	// excluding last case - 
	// process #(comm_size-1) receives
	// an extra remainder.
	int local_size = (rank == comm_size-1)?
					 array_len/comm_size + array_len%comm_size:
					 array_len/comm_size;
	int *sub_array = malloc(local_size * sizeof(int));
	int *tmp_array = malloc(local_size * sizeof(int));
	int *distribution, *offsets;

	if (rank == 0)
	{
		int iter, offset = 0;

		distribution = (int *) malloc(comm_size * sizeof(int));
		offsets = (int *) malloc(comm_size * sizeof(int));

		for (iter = 0; iter < comm_size; iter++)
		{
			distribution[iter] = (iter == comm_size-1)?
								 array_len/comm_size + 
								 array_len%comm_size:
								 array_len/comm_size;
		}

		for (iter = 0; iter < comm_size; iter++)
		{
			offsets[iter] = offset;
			offset += distribution[iter];
		}
	}

	MPI_Scatterv(unsorted, distribution, offsets, 
				 MPI_INT, sub_array, local_size, MPI_INT, 
				 0, MPI_COMM_WORLD);

	merge_sort(sub_array, tmp_array, 0, (local_size - 1));
	MPI_Barrier(MPI_COMM_WORLD);

	int factor  = 2;
	int merging = comm_size/2 + comm_size%2;

	while (merging)
	{
		// i.e. merging case
		if (rank % factor == 0)
		{
			// just for MPI_Recv implementing
			MPI_Status status;
			int add_size, pair_id;

			pair_id = rank + factor/2;

			// check for the case when we have odd
			// number of processes and don't need to
			// perform merge in the last process which have 
			// no pair (a priori fault situation)
			if (pair_id < comm_size)
			{
				MPI_Recv(&add_size, 1, MPI_INT, pair_id, 
						 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	
				// according to the size we've received, 
				// allocating appropriate array for placing 
				// extra data
				int *arrived = 
					(int *) malloc(add_size * sizeof(int));
	
				// receiving sorted subarray from collegue
				MPI_Recv(arrived, add_size, MPI_INT, pair_id, 
						 MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				sub_array = 
					(int *) realloc(sub_array, 
									(local_size + add_size) * 
									sizeof(int));
				tmp_array = 
					(int *) realloc(tmp_array, 
									(local_size + add_size) * 
									sizeof(int));

				// filling extended final array
				int j;

				for (j = local_size; 
					 j < local_size + add_size; j++)

					sub_array[j] = arrived[j-local_size];

				// performing merge
				merge(sub_array, tmp_array, 
					  0, local_size-1, local_size+add_size-1);

				local_size += add_size;
				free(arrived);
			}
		}

		else if (rank % factor == factor/2)
		{
			// Sending info about array size and 
			// array itself to the merging pair-process.
			// We don't check at all the correctness of 
			// sender id (cause we have a verification
			// mechanism in the receiver processes) - 
			// it may be a situation when multiple processes
			// sends their data to one dedicated.
			// But it doesn't impact on main performance - 
			// true time consuming will be in the receiver 
			// process.

			MPI_Send(&local_size, 1, MPI_INT, 
					 rank - factor/2, 
					 0, MPI_COMM_WORLD);
			MPI_Send(sub_array, local_size, MPI_INT, 
					 rank - factor/2, 
					 0, MPI_COMM_WORLD);
		}

		factor *= 2;
		merging = (merging == 1)? 0: (merging/2 + merging%2);
	}

	if (rank == 0)
	{
		int *sorted = sub_array;

		printf("Sorted: ");

		int k;

		for (k = 0; k < local_size; k++)
			printf("%d ", sorted[k]);

		printf("\n\n");

		free(distribution);
		free(offsets);
		free(unsorted);

		timer_end = MPI_Wtime();
		fprintf(stderr, "Execution time: %f\n", 
				timer_end - timer_start);
	}
	
	free(sub_array);
	free(tmp_array);

	// waiting until all the memory is free'd
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
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
