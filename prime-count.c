#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

enum {
	TAG_WORK_REQUEST,
	TAG_WORK,
	TAG_RESULT
};

struct work_data {
	int start, end;
};

int work_from = 1, work_to = 100000;
int chunk_size = 10;

void* send_work(void* size_ptr) {
	int node_num = *(int*)size_ptr - 1;
	int eof_sent_count = 0;
	int next_work = work_from, work_eof = 0;
	struct work_data work;
	MPI_Request recv_request, send_request;
	int sent = 0;
	while (eof_sent_count < node_num) {
		MPI_Status status;
		unsigned char dummy;
		/* タスクのリクエストを受け付ける */
		MPI_Irecv(&dummy, 1, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TAG_WORK_REQUEST,
			MPI_COMM_WORLD, &recv_request);
		/* 次のタスクを生成する */
		if (work_eof) {
			work.start = work.end = 0;
			eof_sent_count++;
		} else {
			work.start = next_work;
			/* if (chunk_size > INT_MAX - next_work || next_work + chunk_size > work_to) { */
			if (chunk_size > work_to - next_work) {
				work.end = work_to;
				work_eof = 1;
			} else {
				work.end = next_work + chunk_size - 1;
				next_work += chunk_size;
			}
		}
		MPI_Wait(&recv_request, &status);
		if (sent) MPI_Wait(&send_request, MPI_STATUS_IGNORE);
		/* 生成したタスクを送信する */
		MPI_Isend(&work, 1, MPI_2INT, status.MPI_SOURCE, TAG_WORK,
			MPI_COMM_WORLD, &send_request);
		sent = 1;
	}
	if (sent) MPI_Wait(&send_request, MPI_STATUS_IGNORE);
	return NULL;
}

pthread_mutex_t queue_lock;
pthread_cond_t queue_ready_to_dequeue, queue_ready_to_enqueue;

int queue_size = 5;
int queue_begin = 0, queue_end = 0, queue_count = 0;
struct work_data *queue;

int queue_empty(void) {
	return queue_count == 0;
}

int queue_full(void) {
	return queue_count == queue_size;
}

void enqueue(struct work_data work) {
	queue[queue_end++] = work;
	if (queue_end >= queue_size) queue_end = 0;
	queue_count++;
}

struct work_data dequeue(void) {
	struct work_data ret = queue[queue_begin++];
	if (queue_begin >= queue_size) queue_begin = 0;
	queue_count--;
	return ret;
}

void* fetch_work(void* size_ptr) {
	int sender_rank = *(int*)size_ptr - 1;
	unsigned char dummy = 0;
	for (;;) {
		struct work_data work;
		MPI_Request requests[2];
		MPI_Status statuses[2];
		int count;
		/* タスクを取ってくる */
		MPI_Isend(&dummy, 1, MPI_UNSIGNED_CHAR, sender_rank, TAG_WORK_REQUEST,
			MPI_COMM_WORLD, &requests[0]);
		MPI_Irecv(&work, 1, MPI_2INT, sender_rank, TAG_WORK,
			MPI_COMM_WORLD, &requests[1]);
		MPI_Waitall(2, requests, statuses);
		MPI_Get_count(&statuses[1], MPI_2INT, &count);
		if (count != 1) continue;
		/* キューにタスクを入れる */
		pthread_mutex_lock(&queue_lock);
		while (queue_full()) {
			pthread_cond_wait(&queue_ready_to_enqueue, &queue_lock);
		}
		enqueue(work);
		pthread_cond_signal(&queue_ready_to_dequeue);
		pthread_mutex_unlock(&queue_lock);
		/* 終了条件のチェック */
		if (work.start == 0 || work.end == 0) break;
	}
	return NULL;
}

int prime_count = 0;

/* MPIで殴る実験なので、あえて遅いアルゴリズムを使う */
void check_prime_and_count(int num) {
	int i;
	if (num < 2) return;
	for (i = 2; i < num; i++) {
		if (num % i == 0) return;
	}
	prime_count++;
}

void* crunch_work(void* placeholder) {
	(void)placeholder;
	for (;;) {
		struct work_data work;
		int i;
		/* キューからタスクを取り出す */
		pthread_mutex_lock(&queue_lock);
		while (queue_empty()) {
			pthread_cond_wait(&queue_ready_to_dequeue, &queue_lock);
		}
		work = dequeue();
		pthread_cond_signal(&queue_ready_to_enqueue);
		pthread_mutex_unlock(&queue_lock);
		/* 終了条件のチェック */
		if (work.start == 0 || work.end == 0) break;

		/* タスクの処理を行う */
		for (i = work.start; i < work.end; i++) {
			check_prime_and_count(i);
		}
		/* 分けることで、 work.end == INT_MAXでも大丈夫にする */
		check_prime_and_count(work.end);
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	int rank, size;
	int is_sender;
	int work_valid = 1;
	pthread_t send_thread, fetch_thread, crunch_thread;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	is_sender = (rank == size - 1);

	if (is_sender) {
		int invalid_argument = 0;
		int help = 0;
		int i;
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "--from") == 0 || strcmp(argv[i], "-f") == 0) {
				if (++i < argc) work_from = atoi(argv[i]); else invalid_argument = 1;
			} else if (strcmp(argv[i], "--to") == 0 || strcmp(argv[i], "-t") == 0) {
				if (++i < argc) work_to = atoi(argv[i]); else invalid_argument = 1;
			} else if (strcmp(argv[i], "--chunk") == 0 || strcmp(argv[i], "-c") == 0) {
				if (++i < argc) chunk_size = atoi(argv[i]); else invalid_argument = 1;
			} else if (strcmp(argv[i], "--queue") == 0 || strcmp(argv[i], "-q") == 0) {
				if (++i < argc) queue_size = atoi(argv[i]); else invalid_argument = 1;
			} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
				help = 1;
			} else {
				invalid_argument = 1;
			}
		}
		if (work_from <= 0 || work_from > work_to || chunk_size <= 0 ||
		queue_size <= 0 || sizeof(*queue) > SIZE_MAX / queue_size) {
			invalid_argument = 1;
		}
		if (invalid_argument || help) {
			if (invalid_argument) fputs("invalid argument.\n", stderr);
			fprintf(stderr, "Usage: %s [options]\n", argc > 0 ? argv[0] : "prime-count");
			fputs("\noptions:\n", stderr);
			fputs("--from num / -f num : set number to begin with\n", stderr);
			fputs("--to num / -t num : set number to end with\n", stderr);
			fputs("--chunk num / -c num : set chunk size\n", stderr);
			fputs("--queue num / -q num : set queue1 size\n", stderr);
			fputs("--help / -h : show this help\n", stderr);
			work_valid = 0;
		} else if (size <= 1) {
			fputs("sorry, this program requires at least 2 nodes to do calculation\n", stderr);
			work_valid = 0;
		}
	}
	MPI_Bcast(&work_valid, 1, MPI_INT, size - 1, MPI_COMM_WORLD);
	if (!work_valid) {
		MPI_Finalize();
		return 1;
	}

	MPI_Bcast(&queue_size, 1, MPI_INT, size - 1, MPI_COMM_WORLD);

	if (is_sender) {
		int final_result = 0, recv_count = 0;
		MPI_Request request;
		MPI_Status recv_result;
		/* 仕事を配る */
		pthread_create(&send_thread, NULL, send_work, &size);
		pthread_join(send_thread, NULL);

		/* マージされた結果を受け取る */
		MPI_Irecv(&final_result, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD, &request);
		MPI_Wait(&request, &recv_result);
		MPI_Get_count(&recv_result, MPI_INT, &recv_count);
		if (recv_count != 1) {
			fputs("failed to receive final result\n", stderr);
			MPI_Abort(MPI_COMM_WORLD, 1);
			MPI_Finalize();
			return 1;
		}
		printf("# of primes in [%d, %d] = %d\n", work_from, work_to, final_result);
	} else {
		int i;
		int calc_result;
		queue = malloc(sizeof(*queue) * queue_size);
		if (queue == NULL) {
			fprintf(stderr, "node %d failed to allocate memory for queue!\n", rank);
			MPI_Abort(MPI_COMM_WORLD, 1);
			MPI_Finalize();
			return 1;
		}

		pthread_mutex_init(&queue_lock, NULL);
		pthread_cond_init(&queue_ready_to_dequeue, NULL);
		pthread_cond_init(&queue_ready_to_enqueue, NULL);
		pthread_create(&fetch_thread, NULL, fetch_work, &size);
		pthread_create(&crunch_thread, NULL, crunch_work, NULL);

		pthread_join(fetch_thread, NULL);
		pthread_join(crunch_thread, NULL);
		pthread_cond_destroy(&queue_ready_to_dequeue);
		pthread_cond_destroy(&queue_ready_to_enqueue);
		free(queue);

		calc_result = prime_count;
		for (i = 1; ; i <<= 1) {
			MPI_Request request;
			int opposite = rank ^ i;
			if (rank & i) {
				/* 送信 */
				MPI_Isend(&calc_result, 1, MPI_INT, opposite, TAG_RESULT,
					MPI_COMM_WORLD, &request);
				MPI_Wait(&request, MPI_STATUS_IGNORE);
				break;
			} else {
				if (opposite < size - 1) {
					/* 受信の相手がいる */
					int opposite_result = 0;
					MPI_Status recv_result;
					int recv_count = 0;
					/* 受信 */
					MPI_Irecv(&opposite_result, 1, MPI_INT, opposite, TAG_RESULT,
						MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &recv_result);
					MPI_Get_count(&recv_result, MPI_INT, &recv_count);
					if (recv_count != 1) {
						fprintf(stderr, "result receive error on node %d\n", rank);
						MPI_Abort(MPI_COMM_WORLD, 1);
						MPI_Finalize();
						return 1;
					}
					/* 現在の結果と受信した結果をマージ */
					calc_result += opposite_result;
				} else {
					/* 受信の相手がいない */
					if (rank <= i) {
						/* 最高ビットよりiが上になったので、この先受信の機会が無い */
						break;
					}
					/* この先また受信の機会があるなら、このラウンドは何もせず待つ */
				}
			}
			/* オーバーフローする前に抜ける */
			if (i > (INT_MAX >> 1)) break;
		}

		if (rank == 0) {
			MPI_Request request;
			/* 全体の結果が集まった先頭のノードの結果を渡す */
			MPI_Isend(&calc_result, 1, MPI_INT, size - 1, TAG_RESULT,
				MPI_COMM_WORLD, &request);
			MPI_Wait(&request, MPI_STATUS_IGNORE);
		}
	}

	MPI_Finalize();
	return 0;
}
