
bin_PROGRAMS = faimtest
faimtest_SOURCES = faimtest.h faimtest.c commands.c
INCLUDES = -I$(top_srcdir)/include
faimtest_LDADD = -L$(top_srcdir)/src/ -lfaim @READLINELIB@
faimtest_DEPENDENCIES = $(top_srcdir)/src/.libs/libfaim.a

