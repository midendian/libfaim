include ../Makefile.dynamicrules
include $(LIBFAIM_LIB)/Makefile.rules

EXEC_NAME = aimdebugd
EXEC_OBJECTS = \
	aimdebugd.o \
	network.o \
	file.o \
	icbm.o

all: $(EXEC_OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC_NAME) $(EXEC_OBJECTS) $(LDFLAGS)

clean:
	rm -f *~ $(EXEC_OBJECTS) $(EXEC_NAME)
