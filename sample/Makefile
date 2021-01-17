CC   := gcc
FLAG := -g -W -Wall -Werror -llivewatcher -lpthread

MULCORE    = sample
MULCOREOBJ = sample.o
MULCORESRC = sample.c


$(MULCORE): $(MULCOREOBJ)
	$(CC) -o $@ $^  $(FLAG)
$(MULCOREOBJ):$(MULCORESRC)
	$(CC) -c $^ -o $@ $(FLAG)

.PHONY: all
all: $(MULCORE)

.PHONY: clean
clean:
	rm -f  $(MULCORE) $(MULCOREOBJ)
