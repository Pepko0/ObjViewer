#pragma once
#include <glad/glad.h>

/**
 * @brief Prosty wrapper na program shaderów OpenGL.
 *
 * id = uchwyt programu shaderów (glCreateProgram()).
 */
typedef struct ShaderProgram
{
    GLuint id;
} ShaderProgram;

/**
 * @brief Wczytuje, kompiluje i linkuje shadery z plików na dysku.
 *
 * @param vertex_path   Ścieżka do pliku shadera wierzchołków (.vert).
 * @param fragment_path Ścieżka do pliku shadera fragmentów (.frag).
 * @return ShaderProgram z ustawionym .id (0 jeśli błąd).
 *
 * @note Funkcja wypisuje błędy kompilacji/linkowania na stdout.
 */
ShaderProgram shader_load_from_files(const char *vertex_path, const char *fragment_path);

/**
 * @brief Ustawia dany program shaderów jako aktywny (glUseProgram).
 *
 * @param s ShaderProgram do użycia.
 */
void shader_use(ShaderProgram s);

/**
 * @brief Usuwa program shaderów z GPU (glDeleteProgram) i zeruje id.
 *
 * @param s Wskaźnik na ShaderProgram.
 */
void shader_destroy(ShaderProgram *s);
