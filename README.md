# Fone

This project is an implementation of a cell phone while keeping true to the unix philosophy.

This project uses the following components:

- [Raspberry Pi 3B](https://www.raspberrypi.org/)
- [Adafruit FONA 808 - Mini Cellular GSM + GPS Breakout](https://www.adafruit.com/product/2542)

Fone consists of a server which will speak to the FONA baord using the serial interface. It accepts two types of transactions from clients:

1. Commands - These are transactions which require the FONA board to query or execute a task.
2. Subscriptions - These are transactions which require the server to notify a subscribing client about any subscribed notifications originating from the FONA board.

Every client in the server will have a specialized function and will need to communicate to the FONA board via the server.

Additional information may be found in the man pages in docs/man/

## Setup
1. Setup build environment

    ```
    sudo apt install build-essential screen
    ```

2. [Setup serial communication](https://learn.adafruit.com/adafruits-raspberry-pi-lesson-5-using-a-console-cable/enabling-serial-console)
3. Connect the GPIO pins on the FONA 808 to the pi. [Look here for more detail on pinouts](https://pinout.xyz/)
  - FONA GND -> Pi Ground
  - FONA Vio -> Pi 3v3 power
  - FONA RX  -> Pi TXD
  - FONA TX  -> Pi RXD
4. Configure ppp to connect to the network stack

    ```
    apt install ppp
    cp etc/ppp/peers/fona /etc/ppp/peers/fona
    chmod 644 /etc/ppp/peers/fona
    cp etc/chatscripts/gprs /etc/chatscripts/gprs
    chmod 644 /etc/chatscripts/gprs
    ```

5. Deploying (may be daemonized later)

    ```
    sudo pon fona
    cd /path/to/fone/code
    make all
    screen -fa -d -m -S foneserver ./build/foneserver
    # Wait until foneserver is running
    screen -fa -d -m -S call-receive ./build/call-receive
    screen -fa -d -m -S sms-receive ./build/sms-receive
    ```


## Interesting configuration

```
AT+CLIP=1      # Enable Caller ID
ATE 0          # Disable Echoing
AT+CHFA=0      # Main audio
AT+CLVL=20     # Volume

# Receiving SMS
AT+CMGL="ALL"  # List all messages in storage
AT+CPMS="SM"   # Prefer SM store
AT+CNMI=2,0    # do not emit +CMTI notifications. This will need to change if moved to subscription model
```


## What is complete:
- [x] Core
  - [x] Tether Cellular Data Connection
  - [x] Threaded named pipes
  - [x] Threaded UART interface
  - [x] Transaction message queue
  - [x] Subscriber message queue
- [x] Application Clients
  - [x] Test Network
  - [x] Sending SMS
  - [x] Receive SMS
  - [x] Initiate Phone Call
  - [x] Answer Phone Call
  - [ ] Something else?
