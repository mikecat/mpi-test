CC=gcc
CFLAGS=-O2 -Wall -Wextra
MPICC=mpicc
MPICFLAGS=$(CFLAGS)
OPENMPFLAGS=-fopenmp
PTHREADSFLAGS=-pthread

# use slash after directory name!
BINDIR=bin/

TARGETS= \
	$(BINDIR)mpi-test \
	$(BINDIR)mpi-test2

.PHONY: all
all: $(TARGETS)

$(BINDIR)mpi-test: mpi-test.c
	$(MPICC) $(MPICFLAGS) -o $@ $^

$(BINDIR)mpi-test2: mpi-test2.c
	$(MPICC) $(MPICFLAGS) $(PTHREADSFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(TARGETS)
