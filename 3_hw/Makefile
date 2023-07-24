CC = gcc
GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant
TARGET = a.out

PREF_SRC = ./src/
PREF_OBJ = ./obj/

SRC = $(shell find . -name "*.c")
OBJ = $(patsubst $(PREF_SRC)%.c, $(PREF_OBJ)%.o, $(SRC))

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(PREF_OBJ)%.o : $(PREF_SRC)%.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

clean :
	rm $(TARGET) $(OBJ)


# all: test.o userfs.o heap_help.o
# 	$(CC) $(GCC_FLAGS) test.o userfs.o heap_help.o

# test.o: test.c
# 	$(CC) $(GCC_FLAGS) -c test.c -o test.o -I ./utils

# userfs.o: userfs.c
# 	$(CC) $(GCC_FLAGS) -c userfs.c -o userfs.o

# heap_help.o: ./utils/heap_help.c
# 	$(CC) $(GCC_FLAGS) -c ./utils/heap_help.c -o ./utils/heap_help.o -I ./utils