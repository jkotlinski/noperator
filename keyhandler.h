unsigned char handle(unsigned char ch, char first_keypress);
void handle_rle(unsigned char ch);

extern char playback_mode;  // if set, some UI operations will be disabled
extern char copy_mode;  // don't allow playing in copy mode

void cursor_home(void);
unsigned char curx(void);
unsigned char cury(void);
extern unsigned char color;

void show_cursor(void);
void hide_cursor(void);

void reset_keyhandler(void);

#define MOVIE_START_MARKER 1
