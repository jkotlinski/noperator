#define RLE_MARKER 0

unsigned char rle_char;

unsigned char rle_dec(unsigned char ch)
{
    static char rle_mode;
    switch (rle_mode) {
        case 0:
            if (ch == RLE_MARKER) {
                rle_mode = 1;
                return 0;
            }
            /* Not RLE. */
            rle_char = ch;
            return 1;
        case 1:
            rle_mode = 2;
            rle_char = ch;
            return 0;
        default:
            rle_mode = 0;
            return ch;
    }
}
