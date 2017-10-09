all: CFLAGS += "-Ofast"
all: base
debug: CFLAGS += "-ggdb3"
debug: base
base:
	gcc -o dotter dotter.c -lm ${CFLAGS}
clean:
	rm dotter
.PHONY: all clean
