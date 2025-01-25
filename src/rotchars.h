#ifndef CHAR_ROT_H
#define CHAR_ROT_H

/* dir: 145=up, 17=down, 157=left, 29=right
 * speed: 0-9, where 0=off, 1=fastest, 9=slowest */
void rotate_char(unsigned char screencode, unsigned char dir, unsigned char period);

void stop_char_rotations(void);

void tick_rotate_chars(void);

#endif
