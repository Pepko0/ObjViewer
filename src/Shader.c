#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Wczytuje cały plik tekstowy do bufora w pamięci.
 *
 * @param path Ścieżka do pliku.
 * @return Wskaźnik na zaalokowany bufor zakończony '\0' lub NULL jeśli błąd.
 *
 * @note Caller musi zrobić free() na zwróconym wskaźniku.
 */
static char *read_file_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("ERROR: cannot open file: %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *data = (char *)malloc((size_t)size + 1);
    if (!data)
    {
        fclose(f);
        return NULL;
    }

    fread(data, 1, (size_t)size, f);
    data[size] = '\0';
    fclose(f);
    return data;
}

/**
 * @brief Kompiluje shader danego typu (vertex/fragment) z kodu źródłowego.
 *
 * @param type  GL_VERTEX_SHADER lub GL_FRAGMENT_SHADER.
 * @param src   Kod źródłowy GLSL (null-terminated).
 * @param label Etykieta do logów (np. nazwa pliku).
 * @return GLuint shader id lub 0 jeśli kompilacja się nie powiodła.
 */
static GLuint compile_shader(GLenum type, const char *src, const char *label)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);

    int ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        printf("ERROR: shader compile failed (%s):\n%s\n", label, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

/**
 * @brief Wczytuje shadery z plików, kompiluje i linkuje program.
 *
 * @param vertex_path   Ścieżka do .vert.
 * @param fragment_path Ścieżka do .frag.
 * @return ShaderProgram (.id==0 oznacza błąd).
 */
ShaderProgram shader_load_from_files(const char *vertex_path, const char *fragment_path)
{
    ShaderProgram out = {0};

    char *vsrc = read_file_text(vertex_path);
    char *fsrc = read_file_text(fragment_path);
    if (!vsrc || !fsrc)
    {
        free(vsrc);
        free(fsrc);
        return out;
    }

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc, vertex_path);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc, fragment_path);
    free(vsrc);
    free(fsrc);

    if (!vs || !fs)
    {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        return out;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        printf("ERROR: program link failed:\n%s\n", log);
        glDeleteProgram(prog);
        return out;
    }

    out.id = prog;
    return out;
}

/**
 * @brief Ustawia program shaderów jako aktywny.
 *
 * @param s ShaderProgram do użycia.
 */
void shader_use(ShaderProgram s)
{
    glUseProgram(s.id);
}

/**
 * @brief Usuwa program shaderów z GPU.
 *
 * @param s Wskaźnik na ShaderProgram do zniszczenia.
 */
void shader_destroy(ShaderProgram *s)
{
    if (s && s->id)
    {
        glDeleteProgram(s->id);
        s->id = 0;
    }
}
