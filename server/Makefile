ifeq ($(CROSS_COMPILE),)
	CC = gcc
else
	CC = $(CROSS_COMPILE)gcc
endif

CFLAGS = -Wall -g
SRC = aesdsocket.c
OBJ = aesdsocket.o
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	rm -f $(TARGET) $(OBJ)
