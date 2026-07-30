#ifndef SHADOWBOX_SERIAL_H
#define SHADOWBOX_SERIAL_H

/*
 * serial_init - Initialize serial port for I/O
 */
void serial_init(void);

/*
 * serial_write_char - Write a character to serial port
 * @a: Character to write
 */
void serial_write_char(char a);

#endif
