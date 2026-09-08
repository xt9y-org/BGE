#ifndef BGE_GAME_H
#define BGE_GAME_H

#include <stdbool.h>

bool game_init(void);
void game_handle_input(void);
void game_update(void);
void game_render(void);
void game_shutdown(void);

#endif
