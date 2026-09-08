#include "portal.h"
#include "util/math.h"

#include <math.h>
#include <stddef.h>

static vec3s portal_center(const level_quad_t* quad, const portal_basis_t basis)
{
    return vec3_add(quad->pos, vec3_add(vec3_scale(basis.right, quad->size.x * 0.5f), vec3_scale(basis.up, quad->size.y * 0.5f)));
}

static vec3s portal_from_local(const vec3s local, const vec3s origin, const portal_basis_t basis)
{
    return vec3_add(origin, vec3_add(vec3_scale(basis.right, local.x), vec3_add(vec3_scale(basis.up, local.y), vec3_scale(basis.normal, local.z))));
}

static portal_basis_t portal_basis_from_quad(const level_quad_t* quad)
{
    f32 rot_y[16], rot_x[16], rot_z[16], model[16], temp[16];
    mat4_rotate_y(rot_y, -DEG2RAD(quad->rot.y));
    mat4_rotate_x(rot_x, -DEG2RAD(quad->rot.x));
    mat4_rotate_z(rot_z, -DEG2RAD(quad->rot.z));
    mat4_multiply(temp, rot_y, rot_x);
    mat4_multiply(model, temp, rot_z);

    return (portal_basis_t) {
        .right = vec3_normalize((vec3s){ model[0], model[1], model[2] }),
        .up = vec3_normalize((vec3s){ model[4], model[5], model[6] }),
        .normal = vec3_normalize((vec3s){ model[8], model[9], model[10] })
    };
}

static portal_basis_t portal_basis_for_side(portal_basis_t basis, const vec3s center, const vec3s point)
{
    if (vec3_dot(vec3_sub(point, center), basis.normal) > 0.0f) {
        basis.right = vec3_scale(basis.right, -1.0f);
        basis.normal = vec3_scale(basis.normal, -1.0f);
    }
    return basis;
}

bool portal_find_link(const level_data_t* level, const level_quad_t* src, portal_link_t* out_link)
{
    if (!level || !src || src->portal_id <= 0) return false;

    const level_quad_t* dst = NULL;
    for (i32 s = 0; s < level->sector_count; s++) {
        const level_sector_data_t* sector = &level->sectors[s];
        for (i32 q = 0; q < sector->quad_count; q++) {
            const level_quad_t* candidate = &sector->quads[q];
            if (candidate == src) continue;
            if (candidate->portal_id != src->portal_id) continue;
            dst = candidate;
            break;
        }
        if (dst) break;
    }

    if (!dst) return false;
    if (out_link) {
        out_link->portal_id = src->portal_id;
        out_link->src = src;
        out_link->dst = dst;
    }
    return true;
}

void vec3_flip_no_y(vec3s* local) {
    local->x = -local->x;
    local->z = -local->z;
}

bool portal_build_camera(const level_quad_t* src, const level_quad_t* dst, const camera_t* in_cam, camera_t* out_cam)
{
    if (!src || !dst || !in_cam || !out_cam) return false;

    portal_basis_t src_basis = portal_basis_from_quad(src);
    portal_basis_t dst_basis = portal_basis_from_quad(dst);
    const vec3s src_center = portal_center(src, src_basis);
    const vec3s dst_center = portal_center(dst, dst_basis);

    src_basis = portal_basis_for_side(src_basis, src_center, in_cam->pos);

    if (vec3_dot(src_basis.normal, portal_basis_from_quad(src).normal) < 0.0f) {
        dst_basis.right = vec3_scale(dst_basis.right, -1.0f);
        dst_basis.normal = vec3_scale(dst_basis.normal, -1.0f);
    }
    if (src->portal_side_flip) {
        dst_basis.right = vec3_scale(dst_basis.right, -1.0f);
        dst_basis.normal = vec3_scale(dst_basis.normal, -1.0f);
    }

    *out_cam = *in_cam;

    vec3s local_pos = {
        vec3_dot(vec3_sub(in_cam->pos, src_center), src_basis.right),
        vec3_dot(vec3_sub(in_cam->pos, src_center), src_basis.up),
        vec3_dot(vec3_sub(in_cam->pos, src_center), src_basis.normal)
    };

    vec3s local_front = {
        vec3_dot(in_cam->front, src_basis.right),
        vec3_dot(in_cam->front, src_basis.up),
        vec3_dot(in_cam->front, src_basis.normal)
    };

    vec3s local_up = {
        vec3_dot(in_cam->up, src_basis.right),
        vec3_dot(in_cam->up, src_basis.up),
        vec3_dot(in_cam->up, src_basis.normal)
    };

    vec3_flip_no_y(&local_pos);
    vec3_flip_no_y(&local_front);
    vec3_flip_no_y(&local_up);

    out_cam->pos = portal_from_local(local_pos, dst_center, dst_basis);
    out_cam->front = vec3_normalize(portal_from_local(local_front, (vec3s){0.0f, 0.0f, 0.0f}, dst_basis));
    out_cam->up = vec3_normalize(portal_from_local(local_up, (vec3s){0.0f, 0.0f, 0.0f}, dst_basis));
    out_cam->pos = vec3_add(out_cam->pos, vec3_scale(dst_basis.normal, -0.05f));

    if (vec3_magnitude(out_cam->front) < 0.0001f || vec3_magnitude(out_cam->up) < 0.0001f) return false;

    out_cam->pitch = RAD2DEG(asinf(fmaxf(-1.0f, fminf(1.0f, out_cam->front.y))));
    out_cam->yaw = RAD2DEG(atan2f(out_cam->front.z, out_cam->front.x));
    return true;
}

bool portal_try_teleport(const level_data_t* level, const vec3s prev_pos, camera_t* cam)
{
    const vec3s move = vec3_sub(cam->pos, prev_pos);
    const vec3s ray_dir = vec3_scale(move, 1.0f / vec3_magnitude(move));
    if (vec3_magnitude(move) < 0.0001f) return false;
 
    for (i32 s = 0; s < level->sector_count; s++) 
    {
        const level_sector_data_t* sector = &level->sectors[s];
        for (i32 q = 0; q < sector->quad_count; q++) 
        {
            const level_quad_t* quad = &sector->quads[q];
            if (quad->portal_id <= 0) continue;
 
            portal_link_t link;
            if (!portal_find_link(level, quad, &link)) continue;
            if (link.src != quad) continue;
 
            f32 t;
            vec3s hit, local_hit;
            if (!level_ray_intersects_quad(prev_pos, ray_dir, quad, &t, &hit, &local_hit)) continue;
            if (t > vec3_magnitude(move) + 0.05f) continue;
 
            camera_t new_cam;
            if (!portal_build_camera(link.src, link.dst, cam, &new_cam)) continue;
 
            *cam = new_cam;
            return true;
        }
    }
    return false;
}
