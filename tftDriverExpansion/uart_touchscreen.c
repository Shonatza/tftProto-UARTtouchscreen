#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // File control
#include <unistd.h>     // UNIX standard function definitions
#include <errno.h>
#include "include/uart_touchscreen.h"

long last_touch = 0;

void init_uart() {

    // Open the UART device file in non-blocking mode
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd < 0) perror("Failed to open UART");

    // Configure the serial port
    tcgetattr(serial_fd, &options);
    options.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(serial_fd, TCIFLUSH);
    tcsetattr(serial_fd, TCSANOW, &options);
}

void close_uart() {
    close(serial_fd);
}

Pair getxy() {

    // Clear any existing data in the UART buffer first
    if(calibrating) tcflush(serial_fd, TCIFLUSH);

    int buf_pos = 0;

    while (1) {
        usleep(1000); // small delay to avoid busy loop
        int bytes_read = read(serial_fd, read_buf, sizeof(read_buf));
        if (bytes_read > 0) {
            for (int i = 0; i < bytes_read; ++i) {
                // Slide buffer
                if (buf_pos < MSG_LENGTH) {
                    msg_buf[buf_pos++] = read_buf[i];
                } else {
                    memmove(msg_buf, msg_buf + 1, MSG_LENGTH - 1);
                    msg_buf[MSG_LENGTH - 1] = read_buf[i];
                }

                // Check for valid message
                if (buf_pos == MSG_LENGTH && msg_buf[0] == 'X' && msg_buf[1] == 'Y' && msg_buf[MSG_LENGTH -1] == '\n') {
                    checksum = msg_buf[2] ^ msg_buf[4];
                    if(checksum == msg_buf[MSG_LENGTH - 2]) {
                        unsigned int val1 = (msg_buf[2] << 8) | msg_buf[3];
                        unsigned int val2 = (msg_buf[4] << 8) | msg_buf[5];
                        Pair p = {val1, val2};
                        clock_gettime(CLOCK_REALTIME, &ts);
                        long current_touch = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
                        pressing = (current_touch - last_touch <= TOUCH_TIMEOUT_MS);;
                        last_touch = current_touch;
                        return p;
                    }
                }
            }
        } else if (bytes_read < 0 && errno != EAGAIN) perror("Error reading from UART");
    }
}
