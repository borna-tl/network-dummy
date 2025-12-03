CC      := gcc
CFLAGS  := -Wall -g
LDFLAGS = -lm
MAIN_C  := tvpnc 
MAIN_S  := tvpns
OBJS_C  := tvpnc.o 
OBJS_S  := tvpns.o utilities.o server_child.o

all: $(MAIN_C) $(MAIN_S)

# building the executable
$(MAIN_C): $(OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(OBJS_C) $(LDFLAGS)

$(MAIN_S): $(OBJS_S)
	$(CC) $(CFLAGS) -o $@ $(OBJS_S) 

# building .o object files from .c source
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# removing objects and the binary, leave source and Makefile
clean:
	rm -f $(OBJS_S) $(OBJS_C) $(MAIN_S) $(MAIN_C) .[!.]* ..?*

.PHONY: clean
