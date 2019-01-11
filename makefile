.DEFAULT_GOAL := server
.PHONY: mkbuild server client clean-pipe clean-out client_status client_sms client_call configuration
CC := gcc
BUILD_DIR := build
LIBS := -lpthread
CFLAGS := -g -Wall
CLIENTS := client_ccid.c 

mkbuild:
	mkdir -p $(BUILD_DIR)

server: mkbuild foneserver.c fone_protocol.c fone_serial.c fone_subscribers.h messagequeue.c shared_pipes.h
	$(CC) foneserver.c -o $(BUILD_DIR)/foneserver $(LIBS) $(CFLAGS)

clients: mkbuild shared_pipes.h client_protocol.h client_status client_sms client_call configuration
	
client_status: client_status.c
	$(CC) client_status.c -o $(BUILD_DIR)/status $(CFLAGS)

configuration: client_configuration.c
	$(CC) client_configuration.c -o $(BUILD_DIR)/fone_configure $(CFLAGS)

client_sms: client_sms_receive.c client_sms_send.c
	$(CC) client_sms_receive.c -o $(BUILD_DIR)/sms-receive $(CFLAGS)
	$(CC) client_sms_send.c -o $(BUILD_DIR)/sms-send $(CFLAGS)

client_call: client_call_initiate.c client_call_receive.c
	$(CC) client_call_initiate.c -o $(BUILD_DIR)/call-send $(LIBS) $(CFLAGS)
	$(CC) client_call_receive.c -o $(BUILD_DIR)/call-receive $(LIBS) $(CFLAGS)

all: clean-out server clients

clean-pipe:
	find /tmp/ -maxdepth 1 -user $${USER} -type p \( -name "fa2s*" -o -name "fs2a*" \) -exec rm -f {} \;

clean-out:
	rm -f $(BUILD_DIR)/*

clean: clean-pipe clean-out
