CC      := gcc
CFLAGS  := -Wall -g
MAIN_S  := suft
OBJS_S  := main.o sender.o file_io.o alarm_handler.o
all: $(MAIN_S)

# building the executable
$(MAIN_S): $(OBJS_S)
	$(CC) $(CFLAGS) -o $@ $(OBJS_S)

# building .o object files from .c source
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# removing objects and the binary, leave source and Makefile
clean:
	rm -f $(OBJS_S) $(MAIN_S) .[!.]* ..?*

.PHONY: clean
