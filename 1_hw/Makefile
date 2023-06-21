GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant

all: libcoro.c solution.c
	gcc $(GCC_FLAGS) heap_help.c libcoro.c solution.c

clean:
	rm a.out
