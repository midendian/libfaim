include Makefile.dynamicrules

UTIL_DIRS = faimtest #aimdump aimpasswd

utils_all: 
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  cd $$i; \
	  $(MAKE); \
	  cd ..; \
	done; 

clean:
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  cd $$i; \
	  $(MAKE) clean; \
	  cd ..; \
	done;
	@rm -f Makefile.dynamicrules
