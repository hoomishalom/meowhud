CC := gcc
CFLAGS := -ggdb -O0 -Wall -Wextra -I./include -DWLR_USE_UNSTABLE $(shell pkg-config --cflags fcft) $(shell pkg-config --cflags --libs pixman-1)
LDFLAGS := -lwayland-client  -lfcft
EXEC := meowhud 
SRCDIR := ./src/
BUILDDIR := ./build/
SRCFILES := $(wildcard $(SRCDIR)*.c)
OBJFILES := $(patsubst $(SRCDIR)%.c,$(BUILDDIR)%.o,$(SRCFILES)) # generates obj files

#.SILENT:

all: $(BUILDDIR) $(EXEC)

$(EXEC): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJFILES): $(BUILDDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

test: test_utils
	./test_utils

test_utils: $(BUILDDIR)tests/test_utils.o $(BUILDDIR)utils.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)tests/test_utils.o: tests/test_utils.c
	mkdir -p $(BUILDDIR)tests
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(EXEC) test_utils 
