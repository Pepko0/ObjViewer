#pragma once
#include <glad/glad.h>

/**
 * @brief Struktura wierzchołka zgodna z formatem OBJ.
 *
 * Każdy wierzchołek posiada:
 *  - pozycję (v)
 *  - normalną (vn)
 *  - współrzędne tekstury (vt)
 */
typedef struct Vertex {
    float position[3];  // v
    float normal[3];    // vn
    float texcoord[2];  // vt
} Vertex;

/**
 * @brief Struktura reprezentująca siatkę (mesh) GPU.
 *
 * Przechowuje:
 *  - bufory OpenGL (VAO, VBO, EBO)
 *  - liczbę indeksów potrzebną do rysowania
 */
typedef struct Mesh {
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    unsigned int index_count;
} Mesh;

/**
 * @brief Tworzy siatkę GPU z tablic wierzchołków i indeksów.
 *
 * @param vertices      Tablica wierzchołków.
 * @param vertex_count  Liczba wierzchołków.
 * @param indices       Tablica indeksów.
 * @param index_count   Liczba indeksów.
 * @return Mesh gotowy do rysowania przez glDrawElements.
 */
Mesh mesh_create(
    const Vertex* vertices,
    unsigned int vertex_count,
    const unsigned int* indices,
    unsigned int index_count
);

/**
 * @brief Rysuje siatkę przy użyciu glDrawElements.
 *
 * @param mesh Wskaźnik na siatkę.
 */
void mesh_draw(const Mesh* mesh);

/**
 * @brief Usuwa bufory OpenGL powiązane z siatką.
 *
 * @param mesh Wskaźnik na siatkę.
 */
void mesh_destroy(Mesh* mesh);
