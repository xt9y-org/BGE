#ifndef BGE_APP_H
#define BGE_APP_H

#include "util/types.h"

#define TITLE "opengl _f"
#define WIDTH 1270
#define HEIGHT 800
#define RENDER_BASE_W 200
#define PALETTE_LEVELS 32.0f

int app_run(void);
double app_get_fps(void);
void app_request_exit(void);

extern u32 g_fbo;
void fbo_resize(i32 w, i32 h);
void post_blit(i32 src_w, i32 src_h, i32 dst_w, i32 dst_h);

#endif
