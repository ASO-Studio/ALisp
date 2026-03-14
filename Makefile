CC = gcc
CFLAGS = -Wall -Wextra -g -rdynamic #-DDEBUG
M_TARGET = alisp
L_TARGET = libepl.so
TARGETS = $(M_TARGET) $(L_TARGET)

M_SOURCES = alisp.c main.c alisp_utils.c xalloc.c
L_SOURCES = libepl.c

M_OBJECTS = $(M_SOURCES:.c=.o)
L_OBJECTS = $(L_SOURCES:.c=.o)
OBJECTS = $(M_OBJECTS) $(L_OBJECTS)

$(M_TARGET): $(M_OBJECTS)
	$(CC) $(CFLAGS) -o $(M_TARGET) $(M_OBJECTS)

%.o: %.c alisp.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

example_lib: $(L_TARGET)
$(L_TARGET): $(L_OBJECTS)
	$(CC) -o $@ $< -shared

clean:
	rm -f $(OBJECTS) $(TARGETS)

.PHONY: clean test example_lib
