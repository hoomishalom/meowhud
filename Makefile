CC := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := 
EXEC := meowline
SRCDIR := ./src/
BUILDDIR := ./build/
SRCFILES := $(shell ls $(SRCDIR)*.c)
OBJFILES := $(patsubst $(SRCDIR)%.c,$(BUILDDIR)%.o,$(SRCFILES)) # generates obj files

.SILENT:

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

all: $(EXEC)

$(EXEC): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJFILES): $(SRCFILES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJFILES) $(EXEC)
