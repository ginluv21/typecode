CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
LIBS    = -lncurses
TARGET  = typecode
SRCDIR  = src
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))

.PHONY: all clean run debug

all: $(BUILDDIR) $(if $(SRCS), $(TARGET))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(TARGET)

debug: CFLAGS += -g -fsanitize=address -fsanitize=undefined
debug: all

clean:
	rm -rf $(BUILDDIR) $(TARGET)
