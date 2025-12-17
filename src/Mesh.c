#include "mesh.h"
#include <stddef.h> 

/**
 * @brief Tworzy i inicjalizuje VAO/VBO/EBO dla siatki.
 */
Mesh mesh_create(
    const Vertex *vertices,
    unsigned int vertex_count,
    const unsigned int *indices,
    unsigned int index_count)
{
    Mesh mesh = {0};
    mesh.index_count = index_count;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    // VBO — wierzchołki
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertex_count * sizeof(Vertex),
        vertices,
        GL_STATIC_DRAW);

    // EBO — indeksy
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        index_count * sizeof(unsigned int),
        indices,
        GL_STATIC_DRAW);

    // layout(location = 0) -> vec3 position
    // location = 0 -> position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // location = 1 -> normal
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // location = 2 -> texcoord
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void *)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

/**
 * @brief Rysuje siatkę.
 */
void mesh_draw(const Mesh *mesh)
{
    glBindVertexArray(mesh->VAO);
    glDrawElements(
        GL_TRIANGLES,
        mesh->index_count,
        GL_UNSIGNED_INT,
        0);
    glBindVertexArray(0);
}

/**
 * @brief Zwalnia zasoby GPU siatki.
 */
void mesh_destroy(Mesh *mesh)
{
    if (!mesh)
        return;

    glDeleteVertexArrays(1, &mesh->VAO);
    glDeleteBuffers(1, &mesh->VBO);
    glDeleteBuffers(1, &mesh->EBO);

    mesh->VAO = 0;
    mesh->VBO = 0;
    mesh->EBO = 0;
    mesh->index_count = 0;
}
