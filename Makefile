CC=gcc
CFLAGS=-O2 -Wall -Wextra
MPICC=mpicc
MPICFLAGS=$(CFLAGS)
OPENMPFLAGS=-fopenmp
PTHREADSFLAGS=-pthread

# use slash after directory name!
BINDIR=bin/

TARGETS= \
	$(BINDIR)print-rank \
	$(BINDIR)prime-count \
	$(BINDIR)prime-count-openmp \
	$(BINDIR)prime-count2

.PHONY: all
all: $(TARGETS)

$(BINDIR)print-rank: print-rank.c
	$(MPICC) $(MPICFLAGS) -o $@ $^

$(BINDIR)prime-count: prime-count.c
	$(MPICC) $(MPICFLAGS) $(PTHREADSFLAGS) -o $@ $^

$(BINDIR)prime-count-openmp: prime-count-openmp.c
	$(CC) $(CFLAGS) $(OPENMPFLAGS) -o $@ $^

$(BINDIR)prime-count2: prime-count2.c
	$(MPICC) $(MPICFLAGS) $(PTHREADSFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(TARGETS)
