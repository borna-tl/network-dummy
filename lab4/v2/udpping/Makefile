CC      := gcc
CFLAGS  := -Wall -g
LDFLAGS = -lm
MAIN_C  := udppingc 
MAIN_S  := udppings
OBJS_C  := udppingc.o setup_udp_client.o send_ping_requests.o calculate_statistics.o alarm_handler_udp.o install_alarm_handler_udp.o
OBJS_S  := udppings.o setup_udp_server.o process_ping_requests.o

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
