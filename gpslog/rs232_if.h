/*
 * simple rs232 lib
 *
 * Copyright (C) 2014 Benedikt Heinz, <hunz@firlefa.nz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#ifndef RS232_IF_H
#define RS232_IF_H

int rs232_setbaud(int fd, int baud);
int rs232_init(const char *dev, int baud, int parity, int time);
int rs232_close(int fd);
int rs232_flush(int fd);
int rs232_read(int fd, uint8_t * d, int len);
int rs232_send(int fd, uint8_t * d, int len);
int rs232_set_rts(int fd, int rts);

#endif
