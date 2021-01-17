# top level makefile

.DEFAULT:default
default:
	cd src && $(MAKE) all
    
.PHONY: install
install:
	cd src && $(MAKE) install

.PHONY: uninstall
uninstall:
	cd src && $(MAKE) uninstall

.PHONY: sample
sample: 
	cd sample && $(MAKE) all

.PHONY: clean
clean: 
	cd src    && $(MAKE) clean
	cd sample && $(MAKE) clean
