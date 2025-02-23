# parts of the Makefile copied from Johannes Nonnast: https://gitlab.lrz.de/00000000014AB37C/basic-structure-for-gra/-/blob/main/Makefile?ref_type=heads

EXEC_NAME = BrightnessAndContrast.out

CC=gcc

HEADERS = -Iinclude
SOURCES = $(wildcard src/*.c src/*.S tests/*.c)

OPTIMIZE_OPTIONS = -O3 -fno-unroll-loops

 # -msse4.1 needed, because internally the gcc implementation makes use of AVX (even with -mno-avx) without setting this flag
CFLAGS = -std=gnu17 -lm -fopenmp -msse4.1
#	Change to your needs if you use other extensions or delete if only sse used.
# 	Options defined here: https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
#	Look for a flag wall like this:
#	-mmmx
#	-msse
#	-msse2
#	-msse3
#   ...
ifeq ($(shell lscpu | grep -c avx), 0)
	CFLAGS += -mno-avx
else
	CFLAGS += -mavx
endif

CFLAGS_DEBUG = -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion \
	-Wformat=2 -Wformat-truncation -Wundef -fno-common -Wconversion \
	-Wmisleading-indentation -fsanitize=address -g3

#	-fno-common In C code, this option controls the placement of global variables defined without an initializer, known as tentative definitions in the C standard.
#		Tentative definitions are distinct from declarations of a variable with the extern keyword, which do not allocate storage.
#		The default is -fno-common, which specifies that the compiler places uninitialized global variables in the BSS section of the object file. This inhibits the merging of tentative definitions by the linker so you get a multiple-definition error if the same variable is accidentally defined in more than one compilation unit.

.PHONY: all debug release clean

all: clean release

debug:
	$(CC) -o $(EXEC_NAME) $(HEADERS) $(SOURCES) $(CFLAGS) $(CFLAGS_DEBUG)

release:
	$(CC) -o $(EXEC_NAME) $(HEADERS) $(SOURCES) $(CFLAGS) $(OPTIMIZE_OPTIONS)

clean:
	rm -f $(EXEC_NAME)
