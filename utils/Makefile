include Makefile.dynamicrules

UTIL_DIRS = faimtest #aimdebugd aimdump aimpasswd

utils_all: 
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  $(MAKE) -C $$i; \
	done; 

clean:
	@dirs='$(UTIL_DIRS)'; \
	for i in $$dirs; do \
	  $(MAKE) -C $$i clean; \
	done;
	@rm -f Makefile.dynamicrules
