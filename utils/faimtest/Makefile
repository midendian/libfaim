include ../Makefile.dynamicrules
include $(LIBFAIM_LIB)/Makefile.rules

EXEC_NAME = faimtest
EXEC_OBJECTS = \
	faimtest.o

all: $(EXEC_OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC_NAME) $(EXEC_OBJECTS) $(LDFLAGS)

clean:
	rm -f $(EXEC_OBJECTS) $(EXEC_NAME)
