include Makefile.dynamicrules

UTIL_DIRS = faimtest #aimpasswd

utils_all: 
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  cd $$i; \
	  make; \
	  cd ..; \
	done; 

clean:
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  cd $$i; \
	  make clean; \
	  cd ..; \
	done;
	@rm -f Makefile.dynamicrules
