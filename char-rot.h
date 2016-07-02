#ifndef CHAR_ROT_H
#define CHAR_ROT_H

/* dir: 0 = Off, 1 = Up, 2 = Right, 3 = Down, 4 = Left */
void rotate_char(unsigned char screencode, unsigned char dir);

void stop_char_rotations(void);

#endif
