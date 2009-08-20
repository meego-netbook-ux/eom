bin_PROGRAMS=eom gesture-demo
eom_CFLAGS = $(DEPS_CFLAGS) $(DEFAULT_COMPILE_FLAGS) -I.. 

gesture_demo_CFLAGS = $(DEPS_CFLAGS) $(DEFAULT_COMPILE_FLAGS) -I.. 
gesture_demo_LDADD = $(DEPS_LIBS)
gesture_demo_SOURCES=gesture-demo.c

eom_SOURCES=main.c
eom_LDADD=$(DEPS_LIBS) -L../tidy -ltidy 

# End of Makefile.am
