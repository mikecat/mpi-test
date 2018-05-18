#include <mpi.h>
#include <stdio.h>

void print_array(int rank, const char* name,
const int* array, int size) {
	int i;
	printf("rank %d: %s = {", rank, name);
	for (i = 0; i < size; i++) {
		printf("%d", array[i]);
		if (i + 1 < size) printf(", ");
	}
	printf("}\n");
}

int main(int argc, char* argv[]) {
	int send[4] = {1, 2, 3, 4};
	int recv[4] = {0, 0, 0, 0};
	int rank;
	MPI_Datatype dt, dt2, dt3;
	int asize = 2, asubsize = 1, astart = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Type_vector(1, 1, 2, MPI_INT, &dt);
	MPI_Type_commit(&dt);
	MPI_Type_vector(2, 1, 2, MPI_INT, &dt2);
	MPI_Type_commit(&dt2);
	MPI_Type_create_subarray(1, &asize, &asubsize, &astart,
		MPI_ORDER_C, MPI_INT, &dt3);
	MPI_Type_commit(&dt3);

	print_array(rank, "send", send, 4);
	print_array(rank, "recv", recv, 4);

	if (rank == 0) {
		MPI_Send(send, 2, dt, 1, 0, MPI_COMM_WORLD);
	} else if (rank == 1) {
		MPI_Status status;
		int count;
		MPI_Recv(recv, 2, dt, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, dt, &count);
		printf("rank %d: received %d elements\n", rank, count);
	}

	print_array(rank, "send", send, 4);
	print_array(rank, "recv", recv, 4);

	recv[0] = recv[1] = recv[2] = recv[3] = 0;

	if (rank == 0) {
		MPI_Send(send, 1, dt2, 1, 0, MPI_COMM_WORLD);
	} else if (rank == 1) {
		MPI_Status status;
		int count;
		MPI_Recv(recv, 1, dt2, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, dt2, &count);
		printf("rank %d: received %d elements\n", rank, count);
	}

	print_array(rank, "send", send, 4);
	print_array(rank, "recv", recv, 4);

	recv[0] = recv[1] = recv[2] = recv[3] = 0;

	if (rank == 0) {
		MPI_Send(send, 2, dt3, 1, 0, MPI_COMM_WORLD);
	} else if (rank == 1) {
		MPI_Status status;
		int count;
		MPI_Recv(recv, 2, dt3, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, dt3, &count);
		printf("rank %d: received %d elements\n", rank, count);
	}

	print_array(rank, "send", send, 4);
	print_array(rank, "recv", recv, 4);

	MPI_Finalize();
}
