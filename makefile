CC ?= clang
CFLAGS := -Wall -Werror
ifdef DEBUG
CFLAGS += -g
endif
LIBS := -lmenu -lform -lncurses

BIN := drtron

HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJDIR := obj/
OBJECTS := $(SOURCES:%.c=$(OBJDIR)%.o)

.PHONY: all
all: $(BIN)

$(BIN): $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJECTS) $(LIBS)

$(OBJDIR)%.o: %.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR):
	@mkdir $(OBJDIR)

.PHONY: clean
clean:
	-rm -rf $(OBJDIR)
	-rm -f $(BIN)
