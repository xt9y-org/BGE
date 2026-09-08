#ifndef CAM_H
#define CAM_H

#include "util/types.h"

typedef struct {
    vec3s pos, front, up;
    f32 yaw, pitch;
} camera_t;

void update_camera_vectors(camera_t* cam);
void camera_apply_mouse_delta(camera_t* cam, f32 dx, f32 dy);

#endif
