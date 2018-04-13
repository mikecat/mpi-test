#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int check_prime(int num) {
	int i;
	if (num < 2) return 0;
	for (i = 2; i < num; i++) {
		if (num % i == 0) return 0;
	}
	return 1;
}

int main(int argc, char* argv[]) {
	int work_from = 1, work_to = 100000;
	int invalid_argument = 0;
	int help = 0;
	int i;
	int prime_count = 0;

	/* コマンドライン引数を処理する */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--from") == 0 || strcmp(argv[i], "-f") == 0) {
			if (++i < argc) work_from = atoi(argv[i]); else invalid_argument = 1;
		} else if (strcmp(argv[i], "--to") == 0 || strcmp(argv[i], "-t") == 0) {
			if (++i < argc) work_to = atoi(argv[i]); else invalid_argument = 1;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			help = 1;
		} else {
			invalid_argument = 1;
		}
	}
	if (work_from <= 0 || work_from > work_to) {
		invalid_argument = 1;
	}
	if (invalid_argument || help) {
		if (invalid_argument) fputs("invalid argument.\n", stderr);
		fprintf(stderr, "Usage: %s [options]\n", argc > 0 ? argv[0] : "prime-count-openmp");
		fputs("\noptions:\n", stderr);
		fputs("--from num / -f num : set number to begin with\n", stderr);
		fputs("--to num / -t num : set number to end with\n", stderr);
		fputs("--help / -h : show this help\n", stderr);
		return 1;
	}

	/* 計算を行う */
	#pragma omp parallel for schedule(dynamic) reduction(+:prime_count)
	for (i = work_from - 1; i < work_to; i++) {
		prime_count += check_prime(i + 1);
	}

	/* 結果を出力する */
	printf("# of primes in [%d, %d] = %d\n", work_from, work_to, prime_count);
	return 0;
}
