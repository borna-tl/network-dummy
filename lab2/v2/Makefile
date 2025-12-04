CC      := gcc
CFLAGS  := -Wall -g
MAIN_C  := tcpcmdc 
MAIN_S  := tcpcmds
OBJS_C  := tcpcmdc.o alarm_handler.o install_alarm_handler.o
OBJS_S  := tcpcmds.o setup_server.o process_client_loop.o handle_client.o execute_command.o install_sigint_handler.o sigint_handler.o
all: $(MAIN_C) $(MAIN_S)

# building the executable
$(MAIN_C): $(OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(OBJS_C)

$(MAIN_S): $(OBJS_S)
	$(CC) $(CFLAGS) -o $@ $(OBJS_S)

# building .o object files from .c source
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# removing objects and the binary, leave source and Makefile
clean:
	rm -f $(OBJS_S) $(OBJS_C) $(MAIN_S) $(MAIN_C) .[!.]* ..?*

.PHONY: clean
