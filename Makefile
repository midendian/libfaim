
include Makefile.rules

SONAME = libfaim.so
SOFILENAME = libfaim.so.0.90 # used for installation

LIBFAIM_OBJECTS = \
	aim_rxhandlers.o \
	aim_auth.o \
	aim_info.o \
	aim_rxqueue.o \
	aim_txqueue.o \
	aim_im.o \
	aim_login.o \
	aim_logoff.o \
	aim_misc.o \
	aim_buddylist.o \
	aim_search.o \
	aim_snac.o \
	aim_tlv.o \
	aim_conn.o \
	aim_chat.o \
	aim_chatnav.o \
	aim_util.o \
	aim_meta.o \
	aim_msgcookie.o \
	aim_ft.o \
	aim_ads.o \
	md5.o

all: libfaim allutils

mkbuildinfo:
	sh $$PWD/mkbuildinfo.sh

libfaim: mkbuildinfo $(LIBFAIM_OBJECTS)
	$(AR) cru libfaim.a $(LIBFAIM_OBJECTS)
	$(RANLIB) libfaim.a
	ld -o $(SONAME) $(LIBFAIM_OBJECTS) -shared -soname $(SONAME)

allutils: libfaim
	@echo "LIBFAIM_INC = $$PWD" > utils/Makefile.dynamicrules; \
	echo "LIBFAIM_LIB = $$PWD" >> utils/Makefile.dynamicrules; \
	cd utils; \
	$(MAKE)	

install: libfaim
	cp -r faim /usr/include
	cp libfaim.so /usr/lib/$(SOFILENAME)
	@echo YOU MUST UPDATE YOUR DYNAMIC LOADER CACHE NOW

cleanutils:
	@echo "LIBFAIM_INC = $$PWD" > utils/Makefile.dynamicrules; \
	echo "LIBFAIM_LIB = $$PWD" >> utils/Makefile.dynamicrules; \
	cd utils; \
	$(MAKE) clean

clean: cleanutils
	rm -f $(LIBFAIM_OBJECTS) $(SONAME) libfaim.a *~ core