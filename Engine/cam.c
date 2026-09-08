#include "cam.h"
#include "util/math.h"

#include <math.h>

void update_camera_vectors(camera_t* cam)
{
    vec3s front;
    front.x = cosf(DEG2RAD(cam->yaw)) * cosf(DEG2RAD(cam->pitch));
    front.y = sinf(DEG2RAD(cam->pitch));
    front.z = sinf(DEG2RAD(cam->yaw)) * cosf(DEG2RAD(cam->pitch));
    cam->front = vec3_normalize(front);
}

void camera_apply_mouse_delta(camera_t* cam, const f32 dx, const f32 dy)
{
    if (!cam) return;

    cam->yaw += dx * 0.1f;
    cam->pitch += dy * 0.1f;

    if (cam->pitch > 89.0f) cam->pitch = 89.0f;
    if (cam->pitch < -89.0f) cam->pitch = -89.0f;

    update_camera_vectors(cam);
}
