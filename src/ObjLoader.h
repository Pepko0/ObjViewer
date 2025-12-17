#pragma once
#include <stddef.h>
#include "Mesh.h"

/**
 * @brief Wynik wczytania OBJ w postaci “CPU modelu”.
 *
 * vertices/indices to gotowe dane do mesh_create().
 */
typedef struct ObjModelData {
    Vertex* vertices;
    unsigned int* indices;
    size_t vertex_count;
    size_t index_count;
} ObjModelData;

/**
 * @brief Wczytuje plik OBJ (v/vt/vn/f) i generuje VBO/EBO na CPU:
 *  - tworzy listę unikalnych wierzchołków (pozycja+normal+uv),
 *  - tworzy indeksy (EBO) pod glDrawElements.
 *
 * @param path Ścieżka do pliku .obj.
 * @param out  Struktura wyjściowa z zaalokowanymi buforami.
 * @return 1 jeśli OK, 0 jeśli błąd.
 *
 * @note Obsługuje f z trójkątów i wielokątów (triangulacja “fan”).
 * @note Obsługuje indeksy dodatnie i ujemne w OBJ.
 */
int obj_load(const char* path, ObjModelData* out);

/**
 * @brief Zwalnia pamięć zaalokowaną w ObjModelData.
 *
 * @param data Wskaźnik na dane.
 */
void obj_free(ObjModelData* data);
