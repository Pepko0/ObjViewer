#include "material.h"
#include <stdio.h>
#include <string.h>

/* stb_image */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
 * @brief Inicjalizacja domyślna.
 */
void material_init(Material* m)
{
    m->diffuse[0] = 1.0f;
    m->diffuse[1] = 1.0f;
    m->diffuse[2] = 1.0f;
    m->diffuseTex = 0;
}

/**
 * @brief Wczytuje teksturę 2D z pliku.
 */
static GLuint load_texture_2d(const char* path)
{
    int w, h, n;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) {
        printf("Failed to load texture: %s\n", path);
        return 0;
    }

    GLenum format = (n == 3) ? GL_RGB : GL_RGBA;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

/**
 * @brief Parser MTL (minimalny).
 */
int material_load_mtl(const char* path, Material* out)
{
    material_init(out);

    FILE* f = fopen(path, "r");
    if (!f) {
        printf("Cannot open MTL: %s\n", path);
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Kd ", 3) == 0) {
            sscanf(line, "Kd %f %f %f",
                   &out->diffuse[0],
                   &out->diffuse[1],
                   &out->diffuse[2]);
        }
        else if (strncmp(line, "map_Kd ", 7) == 0) {
            char texPath[256];
            sscanf(line, "map_Kd %255s", texPath);
            out->diffuseTex = load_texture_2d(texPath);
        }
    }

    fclose(f);
    return 1;
}

/**
 * @brief Aktywuje materiał w shaderze.
 */
void material_bind(const Material* m, GLuint shaderProgram)
{
    glUniform3fv(
        glGetUniformLocation(shaderProgram, "uMaterial.diffuseColor"),
        1,
        m->diffuse
    );

    if (m->diffuseTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m->diffuseTex);
        glUniform1i(
            glGetUniformLocation(shaderProgram, "uMaterial.diffuseMap"),
            0
        );
        glUniform1i(
            glGetUniformLocation(shaderProgram, "uMaterial.hasTexture"),
            1
        );
    } else {
        glUniform1i(
            glGetUniformLocation(shaderProgram, "uMaterial.hasTexture"),
            0
        );
    }
}
