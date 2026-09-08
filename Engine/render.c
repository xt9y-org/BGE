#include "render.h"
#include "state.h"
#include "App.h"
#include "portal.h"
#include "editor.h"
#include "gun.h"
#include <lwcgl/glmodern.h>
#include <lwcgl/lwcgl.h>
#include <stdlib.h>

static vec3s quad_world_normal(const level_quad_t* q);

#define MAX_DEBUG_SHOTS 64
#define MAX_PATH_POINTS 8

typedef struct {
    vec3s points[MAX_PATH_POINTS];
    i32 point_count;
    vec3s normal;
    f32 time;
    f32 r, g, b;
    bool active;
} debug_shot_t;

static debug_shot_t g_debug_shots[MAX_DEBUG_SHOTS];
static u32 g_debug_vao = 0;
static u32 g_debug_vbo = 0;

static void debug_init(void)
{
    GL30.glGenVertexArrays(1, &g_debug_vao);
    GL15.glGenBuffers(1, &g_debug_vbo);
    GL30.glBindVertexArray(g_debug_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_debug_vbo);
    GL15.glBufferData(GL_ARRAY_BUFFER, MAX_DEBUG_SHOTS * 32 * 8 * (i32)sizeof(f32), NULL, GL_DYNAMIC_DRAW);

    GL20.glEnableVertexAttribArray(0);
    GL20.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)0);
    GL20.glEnableVertexAttribArray(1);
    GL20.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(3 * sizeof(f32)));
    GL20.glEnableVertexAttribArray(2);
    GL20.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(6 * sizeof(f32)));
    GL30.glBindVertexArray(0);
}

void render_init(void)
{
    debug_init();
}

void render_shutdown(void)
{
    if (g_debug_vbo) GL15.glDeleteBuffers(1, &g_debug_vbo);
    if (g_debug_vao) GL30.glDeleteVertexArrays(1, &g_debug_vao);
}

void shoot_bullet(void)
{
    if (state.id != STATE_PLAYING) return;

    vec3s origin = state.cam->pos;
    vec3s dir = state.cam->front;
    bool used_portals[256] = {false};
    vec3s path[MAX_PATH_POINTS];
    i32 path_count = 0;
    path[path_count++] = origin;
    vec3s final_normal = {0};
    bool hit_wall = false;

    for (i32 depth = 0; depth <= MAX_PORTAL_DEPTH && !hit_wall && path_count < MAX_PATH_POINTS; depth++)
    {
        f32 nearest_t = 1e10f;
        vec3s nearest_hit = {0};
        const level_quad_t* nearest_quad = NULL;

        for (i32 s = 0; s < state.editor->level->sector_count; s++)
        {
            const level_sector_data_t* sector = &state.editor->level->sectors[s];
            for (i32 q = 0; q < sector->quad_count; q++)
            {
                const level_quad_t* quad = &sector->quads[q];
                if (quad->is_invisible) continue;
                if (quad->portal_id > 0 && used_portals[quad->portal_id]) continue;

                f32 t; vec3s hit;
                if (level_ray_intersects_quad(origin, dir, quad, &t, &hit, NULL) && t > 0.001f && t < nearest_t) {
                    nearest_t = t;
                    nearest_hit = hit;
                    nearest_quad = quad;
                }
            }
        }

        if (!nearest_quad) break;

        path[path_count++] = nearest_hit;

        if (nearest_quad->portal_id <= 0)
        {
            final_normal = quad_world_normal(nearest_quad);
            hit_wall = true;
            break;
        }

        portal_link_t link;
        if (!portal_find_link(state.editor->level, nearest_quad, &link) || link.src != nearest_quad) break;

        used_portals[nearest_quad->portal_id] = true;

        camera_t in = { .pos = origin, .front = dir, .up = {0, 1, 0} };
        camera_t out;
        if (!portal_build_camera(link.src, link.dst, &in, &out)) break;

        origin = out.pos;
        dir = out.front;
        if (path_count < MAX_PATH_POINTS) path[path_count++] = origin;
    }

    if (!hit_wall) return;

    i32 slot = 0;
    f32 oldest = g_debug_shots[0].time;
    for (i32 i = 0; i < MAX_DEBUG_SHOTS; i++) {
        if (!g_debug_shots[i].active) { slot = i; break; }
        if (g_debug_shots[i].time < oldest) { oldest = g_debug_shots[i].time; slot = i; }
    }

    for (i32 i = 0; i < path_count; i++) g_debug_shots[slot].points[i] = path[i];
    g_debug_shots[slot].point_count = path_count;
    g_debug_shots[slot].normal = final_normal;
    const double timer_resolution = (double)Sys.getTimerResolution();
    g_debug_shots[slot].time = timer_resolution > 0.0
        ? (f32)((double)Sys.getTime() / timer_resolution)
        : 0.0f;
    g_debug_shots[slot].r = (f32)(rand() % 256) / 255.0f;
    g_debug_shots[slot].g = (f32)(rand() % 256) / 255.0f;
    g_debug_shots[slot].b = (f32)(rand() % 256) / 255.0f;
    g_debug_shots[slot].active = true;
}

static void render_debug_shots(void)
{
    const double timer_resolution = (double)Sys.getTimerResolution();
    const f32 now = timer_resolution > 0.0
        ? (f32)((double)Sys.getTime() / timer_resolution)
        : 0.0f;
    f32 verts[MAX_DEBUG_SHOTS * 32 * 8];
    i32 line_verts = 0, tri_verts = 0;

    for (i32 i = 0; i < MAX_DEBUG_SHOTS; i++)
    {
        if (!g_debug_shots[i].active) continue;
        if (now - g_debug_shots[i].time > 4.0f) { g_debug_shots[i].active = false; continue; }

        f32 r = g_debug_shots[i].r, gr = g_debug_shots[i].g, b = g_debug_shots[i].b;

        if (state.debug_visible) {
            for (i32 j = 0; j < g_debug_shots[i].point_count - 1; j += 2)
            {
                vec3s a = g_debug_shots[i].points[j];
                vec3s pb = g_debug_shots[i].points[j+1];
                i32 n = line_verts;
                verts[n*8+0]=a.x; verts[n*8+1]=a.y; verts[n*8+2]=a.z;
                verts[n*8+3]=r; verts[n*8+4]=gr; verts[n*8+5]=b;
                verts[n*8+6]=0; verts[n*8+7]=0; line_verts++;
                verts[n*8+8]=pb.x; verts[n*8+9]=pb.y; verts[n*8+10]=pb.z;
                verts[n*8+11]=r; verts[n*8+12]=gr; verts[n*8+13]=b;
                verts[n*8+14]=0; verts[n*8+15]=0; line_verts++;
            }
        }
    }

    for (i32 i = 0; i < MAX_DEBUG_SHOTS; i++)
    {
        if (!g_debug_shots[i].active) continue;
        if (now - g_debug_shots[i].time > 4.0f) continue;

        vec3s n = g_debug_shots[i].normal;
        vec3s h = g_debug_shots[i].points[g_debug_shots[i].point_count - 1];

        vec3s up = {0,1,0};
        if (fabsf(vec3_dot(n, up)) > 0.99f) up = (vec3s){0,0,1};
        vec3s right = vec3_normalize(vec3_cross(n, up));
        vec3s bt = vec3_cross(n, right);
        f32 s = 0.12f;
        vec3s corners[4] = {
            vec3_add(vec3_add(h, vec3_scale(right,-s)), vec3_scale(bt,-s)),
            vec3_add(vec3_add(h, vec3_scale(right, s)), vec3_scale(bt,-s)),
            vec3_add(vec3_add(h, vec3_scale(right, s)), vec3_scale(bt, s)),
            vec3_add(vec3_add(h, vec3_scale(right,-s)), vec3_scale(bt, s)),
        };
        f32 r = 0, gr = 0, b = 0;
        if (state.debug_visible) { r = g_debug_shots[i].r; gr = g_debug_shots[i].g; b = g_debug_shots[i].b; }
        i32 idx[] = {0,1,2, 0,2,3};
        for (i32 ti = 0; ti < 6; ti++) {
            vec3s c = corners[idx[ti]];
            i32 n = line_verts + tri_verts;
            verts[n*8+0]=c.x; verts[n*8+1]=c.y; verts[n*8+2]=c.z;
            verts[n*8+3]=r; verts[n*8+4]=gr; verts[n*8+5]=b;
            verts[n*8+6]=0; verts[n*8+7]=0; tri_verts++;
        }
    }

    i32 total = line_verts + tri_verts;
    if (!total) return;

    glDepthFunc(GL_LEQUAL);

    GL20.glUseProgram(state.data->program);
    f32 identity[16];
    mat4_identity(identity);
    GL20.glUniformMatrix4fv(state.data->u_model, 1, GL_FALSE, identity);
    texture_bind(&state.text->textures[3], 0);

    GL30.glBindVertexArray(g_debug_vao);
    GL15.glBindBuffer(GL_ARRAY_BUFFER, g_debug_vbo);
    GL15.glBufferSubData(GL_ARRAY_BUFFER, 0, total * 8 * sizeof(f32), verts);
    if (line_verts) glDrawArrays(GL_LINES, 0, line_verts);
    if (tri_verts) glDrawArrays(GL_TRIANGLES, line_verts, tri_verts);
    GL30.glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

void apply_level_camera(camera_t *cam, level_data_t *level)
{
    cam->pos = level->cam.pos;
    cam->yaw = level->cam.yaw;
    cam->pitch = level->cam.pitch;
    update_camera_vectors(cam);
}

void player_collide_quads(const level_data_t* level, camera_t* cam)
{
    const f32 radius = 0.5f;

    for (i32 s = 0; s < level->sector_count; s++)
    {
        const level_sector_data_t* sector = &level->sectors[s];
        for (i32 q = 0; q < sector->quad_count; q++)
        {
            const level_quad_t* quad = &sector->quads[q];
            if (quad->portal_id > 0 || !quad->is_solid || quad->is_billboard) continue;

            f32 ry[16], rx[16], rz[16], m[16], t[16];
            mat4_rotate_y(ry, -DEG2RAD(quad->rot.y));
            mat4_rotate_x(rx, -DEG2RAD(quad->rot.x));
            mat4_rotate_z(rz, -DEG2RAD(quad->rot.z));
            mat4_multiply(t, ry, rx);
            mat4_multiply(m, t, rz);

            const vec3s normal = { m[8], m[9], m[10] };
            const vec3s right  = { m[0], m[1], m[2] };
            const vec3s up     = { m[4], m[5], m[6] };
            const vec3s to_cam = vec3_sub(cam->pos, quad->pos);

            const f32 dist = vec3_dot(to_cam, normal);
            if (fabsf(dist) > radius) continue;

            const f32 lx = vec3_dot(to_cam, right);
            const f32 ly = vec3_dot(to_cam, up);
            const f32 mg = radius * 0.5f;

            if (lx < -mg || lx > quad->size.x + mg) continue;
            if (ly < -mg || ly > quad->size.y + mg)  continue;

            const f32 push = dist >= 0.0f ? (radius - dist) : -(radius + dist);
            cam->pos = vec3_add(cam->pos, vec3_scale(normal, push));
        }
    }
}

static vec3s quad_world_normal(const level_quad_t* q)
{
    f32 ry[16], rx[16], rz[16], m[16], t[16];
    mat4_rotate_y(ry, -DEG2RAD(q->rot.y));
    mat4_rotate_x(rx, -DEG2RAD(q->rot.x));
    mat4_rotate_z(rz, -DEG2RAD(q->rot.z));
    mat4_multiply(t, ry, rx);
    mat4_multiply(m, t, rz);
    return vec3_normalize((vec3s){m[8], m[9], m[10]});
}

static void oblique_near_clip(f32* proj, const f32* view, vec3s plane_pos, vec3s plane_normal)
{
    vec3s pos_v = {
        view[0]*plane_pos.x + view[4]*plane_pos.y + view[8]*plane_pos.z  + view[12],
        view[1]*plane_pos.x + view[5]*plane_pos.y + view[9]*plane_pos.z  + view[13],
        view[2]*plane_pos.x + view[6]*plane_pos.y + view[10]*plane_pos.z + view[14]
    };

    vec3s norm_v = {
        view[0]*plane_normal.x + view[4]*plane_normal.y + view[8]*plane_normal.z,
        view[1]*plane_normal.x + view[5]*plane_normal.y + view[9]*plane_normal.z,
        view[2]*plane_normal.x + view[6]*plane_normal.y + view[10]*plane_normal.z
    };

    f32 d = -vec3_dot(norm_v, pos_v);
    if (norm_v.z > 0.0f) { norm_v = vec3_scale(norm_v, -1.0f); d = -d; }
    if (fabsf(d) < 0.005f) return;

    vec4s cp = { norm_v.x, norm_v.y, norm_v.z, d };

    f32 sx = cp.x > 0.0f ? 1.0f : (cp.x < 0.0f ? -1.0f : 0.0f);
    f32 sy = cp.y > 0.0f ? 1.0f : (cp.y < 0.0f ? -1.0f : 0.0f);

    vec4s q = { (sx + proj[8]) / proj[0], (sy + proj[9]) / proj[5], -1.0f, (1.0f + proj[10]) / proj[14] };
    f32 dot = cp.x*q.x + cp.y*q.y + cp.z*q.z + cp.w*q.w;
    if (fabsf(dot) < 0.0001f) return;
    f32 s = 2.0f / dot;

    proj[2]  = cp.x * s;
    proj[6]  = cp.y * s;
    proj[10] = cp.z * s + 1.0f;
    proj[14] = cp.w * s;
}

static void set_camera_uniforms(const camera_t* cam)
{
    f32 view[16], proj[16];
    mat4_lookat(view, cam->pos, vec3_add(cam->pos, cam->front), cam->up);
    mat4_perspective(proj, DEG2RAD(45.0f), (f32)state.fb->w / (f32)state.fb->h, 0.1f, 100.0f);

    GL20.glUniformMatrix4fv(state.data->u_view, 1, GL_FALSE, view);
    GL20.glUniformMatrix4fv(state.data->u_proj, 1, GL_FALSE, proj);
    level_set_frustum(view, proj);
}

static void render_portals(const level_data_t* level, const camera_t* cam, i32 depth, i32 stencil_ref)
{
    if (depth >= MAX_PORTAL_DEPTH) return;

    for (i32 s = 0; s < level->sector_count; s++)
    {
        const level_sector_data_t* sector = &level->sectors[s];
        for (i32 q = 0; q < sector->quad_count; q++)
        {
            portal_link_t link;
            camera_t portal_cam;
            const level_quad_t* quad = &sector->quads[q];

            if (quad->portal_id <= 0) continue;
            if (!portal_find_link(level, quad, &link)) continue;
            if (link.src != quad) continue;
            if (!quad_visible(link.src)) continue;
            if (!portal_build_camera(link.src, link.dst, cam, &portal_cam)) continue;

            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glStencilMask(0xFF);
            glStencilFunc(GL_EQUAL, stencil_ref, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

            level_render_quad(link.src, (vec4s){1,1,1,1});

            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, stencil_ref + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glDepthRange(1.0, 1.0);

            level_render_quad(link.src, (vec4s){1,1,1,1});

            glDepthRange(0.0, 1.0);
            glDepthFunc(GL_LESS);

            f32 view[16], proj[16];
            mat4_lookat(view, portal_cam.pos, vec3_add(portal_cam.pos, portal_cam.front), portal_cam.up);
            mat4_perspective(proj, DEG2RAD(45.0f), (f32)state.fb->w / (f32)state.fb->h, 0.1f, 100.0f);

            oblique_near_clip(proj, view, link.dst->pos, quad_world_normal(link.dst));

            GL20.glUniformMatrix4fv(state.data->u_view, 1, GL_FALSE, view);
            GL20.glUniformMatrix4fv(state.data->u_proj, 1, GL_FALSE, proj);
            level_set_frustum(view, proj);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, stencil_ref + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            render_portals(level, &portal_cam, depth + 1, stencil_ref + 1);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, stencil_ref + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            GL20.glUniformMatrix4fv(state.data->u_view, 1, GL_FALSE, view);
            GL20.glUniformMatrix4fv(state.data->u_proj, 1, GL_FALSE, proj);

            level_render(level, &portal_cam);
            render_debug_shots();

            set_camera_uniforms(cam);

            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, stencil_ref + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            level_render_quad(link.src, (vec4s){1,1,1,1});

            glDepthFunc(GL_LESS);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_EQUAL, stencil_ref + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

            level_render_quad(link.src, (vec4s){1,1,1,1});

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }
    }
}

void render_main(i32 rw, i32 rh)
{
    state.fb->w = rw;
    state.fb->h = rh;

    fbo_resize(rw, rh);

    GL30.glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, rw, rh);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    GL20.glUseProgram(state.data->program);
    f32 model[16];
    mat4_identity(model);
    GL20.glUniformMatrix4fv(state.data->u_model, 1, GL_FALSE, model);
    set_camera_uniforms(state.cam);

    render_portals(state.editor->level, state.cam, 0, 0);
    level_render(state.editor->level, state.cam);
    if (state.id == STATE_EDITOR) editor_render_borders();

    render_debug_shots();
    gun_render(rw, rh);
}
