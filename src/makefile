
CC = gcc

### for debugging
CFLAGS  = -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -marm -O0 -Wall -g
#CFLAGS  = -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -marm -O3 -Wall

LD = ld
LDFLAGS = -lrt -I/usr/local/include -L/usr/local/lib -lwiringPi


OBJ = rpiLcdDaemon.o libs/lcd.o libs/utility.o libs/server.o libs/command.o libs/timer.o libs/rpiHardware.o
BIN = rpiLcdDaemon

daemon: $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

client: $(OBJ)
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

libs/%.o: %.c
	$(CC) $(CFLAGS) -c $<

#.PHONY: clean
clean:
	rm -rf $(BIN) $(OBJ) client libs/$(OBJ)
