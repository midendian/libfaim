include ../Makefile.dynamicrules
include $(LIBFAIM_LIB)/Makefile.rules

LDFLAGS += -lpcap -lfaim

EXEC_NAME = aimdump 
EXEC_OBJECTS = \
	main.o \
	util.o \
	icbmparse.o

all: $(EXEC_OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC_NAME) $(EXEC_OBJECTS) $(LDFLAGS)

clean:
	rm -f $(EXEC_OBJECTS) $(EXEC_NAME)
