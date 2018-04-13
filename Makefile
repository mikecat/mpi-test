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
	$(BINDIR)prime-count

.PHONY: all
all: $(TARGETS)

$(BINDIR)print-rank: print-rank.c
	$(MPICC) $(MPICFLAGS) -o $@ $^

$(BINDIR)prime-count: prime-count.c
	$(MPICC) $(MPICFLAGS) $(PTHREADSFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(TARGETS)
