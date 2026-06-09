CC     = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
TARGET = mysgf
TEST   = tests/test_basic

SRCS = src/main.c src/fs.c src/inode.c src/block.c \
       src/file_ops.c src/dir_ops.c src/shell.c src/save.c
OBJS = $(SRCS:.c=.o)

TEST_SRCS = tests/test_basic.c src/fs.c src/inode.c src/block.c \
            src/file_ops.c src/dir_ops.c src/save.c

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "  --> Compilation OK : ./$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST)
	./$(TEST)

$(TEST): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET) $(TEST) disk.img

.PHONY: all test clean
