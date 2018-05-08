#include "mpi.h"
#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int main(int argc, char* argv[]) {
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp critical
		printf("rank = %d, size = %d, thread = %d\n", rank, size, omp_get_thread_num());
	}
#else
	printf("rank = %d, size = %d (OpenMP disabled)\n", rank, size);
#endif
	MPI_Finalize();
	return 0;
}
