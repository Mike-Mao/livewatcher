CC   := gcc
FLAG := -g -W -Wall -Werror -lpthread

INSTALL_H   := /usr/local/include
INSTALL_LIB := /usr/local/lib

LWLIB:=liblivewatcher.a
LWOBJ:=livewatcher.o

$(LWOBJ):livewatcher.c
	$(CC) -c $^ -o $@ $(FLAG)
$(LWLIB):$(LWOBJ)
	ar cr $@ $^

.PHONY: all
all:$(LWLIB)

.PHONY: install
install: all
	cp $(LWLIB)  $(INSTALL_LIB)
	cp ./livewatcher.h  $(INSTALL_H)
    
.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_LIB)/$(LWLIB)
	rm -f $(INSTALL_H)/livewatcher.h
    
.PHONY: clean
clean:
	rm -f $(LWOBJ) $(LWLIB)
