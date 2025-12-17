#pragma once
#include <glad/glad.h>

/**
 * @brief Struktura materiału (MTL).
 *
 * Obsługujemy:
 *  - kolor dyfuzyjny (Kd)
 *  - mapę dyfuzyjną (map_Kd)
 */
typedef struct Material {
    float diffuse[3];   // Kd
    GLuint diffuseTex;  // mapa_Kd (0 jeśli brak)
} Material;

/**
 * @brief Inicjalizuje domyślny materiał.
 */
void material_init(Material* m);

/**
 * @brief Wczytuje plik MTL (obsługa newmtl, Kd, map_Kd).
 *
 * @param path Ścieżka do pliku .mtl
 * @param out  Materiał wyjściowy
 * @return 1 jeśli OK, 0 jeśli błąd
 */
int material_load_mtl(const char* path, Material* out);

/**
 * @brief Aktywuje materiał (bindowanie tekstury + uniformy).
 *
 * @param m   Materiał
 * @param sh  ID programu shaderów
 */
void material_bind(const Material* m, GLuint shaderProgram);
