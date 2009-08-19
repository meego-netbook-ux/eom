bin_PROGRAMS=eom test
eom_CFLAGS = $(DEPS_CFLAGS) $(DEFAULT_COMPILE_FLAGS) -I.. 

test_CFLAGS = $(DEPS_CFLAGS) $(DEFAULT_COMPILE_FLAGS) -I.. 
test_LDADD = $(DEPS_LIBS)
test_SOURCES=test.c

eom_SOURCES=main.c
eom_LDADD=$(DEPS_LIBS) -L../tidy -ltidy 

# End of Makefile.am
