#ifndef UART_TOUCHSCREEN_H
#define UART_TOUCHSCREEN_H

#include <termios.h>    // Terminal control
#include <time.h>

#define SERIAL_PORT "/dev/serial0"
#define BAUDRATE B9600
#define MSG_LENGTH 8
#define READ_BUFFER_SIZE 256
#define TOUCH_TIMEOUT_MS 100

struct timespec ts;
extern long last_touch;

typedef struct {
    unsigned int x;
    unsigned int y;
} Pair;

Pair pos, last_pos;
int serial_fd;
struct termios options;

unsigned char read_buf[READ_BUFFER_SIZE];
unsigned char msg_buf[MSG_LENGTH];
unsigned char checksum;
int calibrating;
int pressing;

void init_uart();
void close_uart();
Pair getxy();

#endif