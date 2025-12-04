CC      := gcc
CFLAGS  := -Wall -g
MAIN_R  := suftd
OBJS_R  := main.o receiver.o file_io.o
all: $(MAIN_R)

# building the executable
$(MAIN_R): $(OBJS_R)
	$(CC) $(CFLAGS) -o $@ $(OBJS_R)

# building .o object files from .c source
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# removing objects and the binary, leave source and Makefile
clean:
	rm -f $(OBJS_R) $(MAIN_R) .[!.]* ..?*

.PHONY: clean
