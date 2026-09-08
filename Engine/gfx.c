#include "gfx.h"

#include <stdio.h>

u32 compile_shader(const u32 type, const char* src)
{
    const u32 shader = GL20.glCreateShader(type);
    if (!shader) {
        fprintf(stderr, "BGE: failed to create shader\n");
        return 0;
    }

    GL20.glShaderSource(shader, 1, &src, NULL);
    GL20.glCompileShader(shader);

    i32 ok = 0;
    GL20.glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        GL20.glGetShaderInfoLog(shader, (GLsizei)sizeof(log), NULL, log);
        fprintf(stderr, "BGE: shader compilation failed: %s\n", log);
        GL20.glDeleteShader(shader);
        return 0;
    }

    return shader;
}

u32 create_program(const char* vs, const char* fs)
{
    const u32 vertex = compile_shader(GL_VERTEX_SHADER, vs);
    if (!vertex) return 0;

    const u32 fragment = compile_shader(GL_FRAGMENT_SHADER, fs);
    if (!fragment) {
        GL20.glDeleteShader(vertex);
        return 0;
    }

    const u32 program = GL20.glCreateProgram();
    if (!program) {
        GL20.glDeleteShader(vertex);
        GL20.glDeleteShader(fragment);
        return 0;
    }

    GL20.glAttachShader(program, vertex);
    GL20.glAttachShader(program, fragment);
    GL20.glLinkProgram(program);
    GL20.glDeleteShader(vertex);
    GL20.glDeleteShader(fragment);

    i32 ok = 0;
    GL20.glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        GL20.glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
        fprintf(stderr, "BGE: program link failed: %s\n", log);
        GL20.glDeleteProgram(program);
        return 0;
    }

    return program;
}
