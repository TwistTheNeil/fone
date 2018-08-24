.DEFAULT_GOAL := server
.PHONY: mkbuild server client clean-pipe clean-out client_status client_sms client_call
CC := gcc
BUILD_DIR := build
LIBS := -lpthread
CFLAGS := -g -Wall
CLIENTS := client_ccid.c 

mkbuild:
	mkdir -p $(BUILD_DIR)

server: mkbuild foneserver.c fone_protocol.c fone_serial.c fone_subscribers.h messagequeue.c shared_pipes.h
	$(CC) foneserver.c -o $(BUILD_DIR)/foneserver.out $(LIBS) $(CFLAGS)

clients: mkbuild shared_pipes.h client_protocol.h client_status client_sms client_call
	
client_status: client_status.c
	$(CC) client_status.c -o $(BUILD_DIR)/status.out $(CFLAGS)

client_sms: client_sms_receive.c client_sms_send.c
	$(CC) client_sms_receive.c -o $(BUILD_DIR)/client_sms_receive.out $(CFLAGS)
	$(CC) client_sms_send.c -o $(BUILD_DIR)/client_sms_send.out $(CFLAGS)

client_call: client_call_initiate.c client_call_receive.c
	$(CC) client_call_initiate.c -o $(BUILD_DIR)/client_call_initiate.out $(LIBS) $(CFLAGS)
	$(CC) client_call_receive.c -o $(BUILD_DIR)/client_call_receive.out $(LIBS) $(CFLAGS)

all: clean-out server clients

clean-pipe:
	find . -type p -exec rm -f {} \;

clean-out:
	rm -f $(BUILD_DIR)/*out

clean: clean-pipe clean-out
