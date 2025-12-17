#include "ObjLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* =========================================================
   Proste dynamiczne tablice (realloc)
   ========================================================= */

/**
 * @brief Dynamiczna tablica float (np. pozycje, normalne, uv).
 */
typedef struct FloatArray {
    float* data;
    size_t count;
    size_t capacity;
} FloatArray;

/**
 * @brief Dynamiczna tablica Vertex.
 */
typedef struct VertexArray {
    Vertex* data;
    size_t count;
    size_t capacity;
} VertexArray;

/**
 * @brief Dynamiczna tablica indeksów (EBO).
 */
typedef struct UIntArray {
    unsigned int* data;
    size_t count;
    size_t capacity;
} UIntArray;

static void fa_push(FloatArray* a, float v) {
    if (a->count + 1 > a->capacity) {
        size_t newCap = a->capacity ? a->capacity * 2 : 256;
        a->data = (float*)realloc(a->data, newCap * sizeof(float));
        a->capacity = newCap;
    }
    a->data[a->count++] = v;
}

static void va_push(VertexArray* a, Vertex v) {
    if (a->count + 1 > a->capacity) {
        size_t newCap = a->capacity ? a->capacity * 2 : 256;
        a->data = (Vertex*)realloc(a->data, newCap * sizeof(Vertex));
        a->capacity = newCap;
    }
    a->data[a->count++] = v;
}

static void ua_push(UIntArray* a, unsigned int v) {
    if (a->count + 1 > a->capacity) {
        size_t newCap = a->capacity ? a->capacity * 2 : 256;
        a->data = (unsigned int*)realloc(a->data, newCap * sizeof(unsigned int));
        a->capacity = newCap;
    }
    a->data[a->count++] = v;
}

/* =========================================================
   Hash map: (vi,ti,ni) -> index w VertexArray
   (open addressing)
   ========================================================= */

typedef struct Key {
    int vi, ti, ni; // 0-based; -1 jeśli brak
} Key;

typedef struct Entry {
    Key key;
    unsigned int value; // indeks w VertexArray
    int used;
} Entry;

typedef struct KeyMap {
    Entry* entries;
    size_t capacity; // power of two
    size_t size;
} KeyMap;

static uint64_t hash_u64(uint64_t x) {
    // proste mieszanie (splitmix64-ish)
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static uint64_t key_hash(Key k) {
    uint64_t a = (uint32_t)(k.vi + 1);
    uint64_t b = (uint32_t)(k.ti + 1);
    uint64_t c = (uint32_t)(k.ni + 1);
    uint64_t x = (a << 42) ^ (b << 21) ^ c;
    return hash_u64(x);
}

static int key_eq(Key a, Key b) {
    return a.vi == b.vi && a.ti == b.ti && a.ni == b.ni;
}

static void map_init(KeyMap* m, size_t capPow2) {
    m->capacity = capPow2;
    m->size = 0;
    m->entries = (Entry*)calloc(m->capacity, sizeof(Entry));
}

static void map_free(KeyMap* m) {
    free(m->entries);
    m->entries = NULL;
    m->capacity = 0;
    m->size = 0;
}

static void map_rehash(KeyMap* m);

static int map_get_or_insert(KeyMap* m, Key key, unsigned int* outValue, int* outInserted)
{
    if (m->size * 10 >= m->capacity * 7) { // load factor ~0.7
        map_rehash(m);
    }

    uint64_t h = key_hash(key);
    size_t mask = m->capacity - 1;
    size_t i = (size_t)h & mask;

    for (;;) {
        Entry* e = &m->entries[i];
        if (!e->used) {
            // insert
            e->used = 1;
            e->key = key;
            e->value = 0; // caller ustawi
            m->size++;
            *outInserted = 1;
            *outValue = 0;
            return 1;
        }
        if (key_eq(e->key, key)) {
            *outInserted = 0;
            *outValue = e->value;
            return 1;
        }
        i = (i + 1) & mask;
    }
}

static void map_rehash(KeyMap* m)
{
    KeyMap nm;
    map_init(&nm, m->capacity ? m->capacity * 2 : 1024);

    for (size_t i = 0; i < m->capacity; i++) {
        Entry* e = &m->entries[i];
        if (!e->used) continue;

        unsigned int dummyVal = 0;
        int inserted = 0;
        map_get_or_insert(&nm, e->key, &dummyVal, &inserted);

        // wpisz wartość
        uint64_t h = key_hash(e->key);
        size_t mask = nm.capacity - 1;
        size_t j = (size_t)h & mask;
        for (;;) {
            Entry* ne = &nm.entries[j];
            if (ne->used && key_eq(ne->key, e->key)) {
                ne->value = e->value;
                break;
            }
            j = (j + 1) & mask;
        }
    }

    free(m->entries);
    *m = nm;
}

/* =========================================================
   Parsowanie OBJ
   ========================================================= */

/**
 * @brief Zamienia indeks OBJ (1-based, może być ujemny) na 0-based.
 *
 * @param idx      indeks z OBJ (np. 1,2,3 lub -1,-2,...)
 * @param count    liczba elementów w danej liście (v/vt/vn)
 * @return 0-based index, albo -1 jeśli idx==0 / niepoprawny.
 */
static int resolve_index(int idx, int count)
{
    if (idx == 0) return -1;
    if (idx > 0) return idx - 1;
    // ujemny: -1 oznacza ostatni element
    return count + idx;
}

/**
 * @brief Pomija białe znaki.
 */
static char* skip_ws(char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/**
 * @brief Parsuje token postaci: v/t/n lub v//n lub v/t lub v
 *
 * @param tok      token (np. "12/5/9")
 * @param out_vi   indeks v (0-based lub -1)
 * @param out_ti   indeks vt (0-based lub -1)
 * @param out_ni   indeks vn (0-based lub -1)
 */
static void parse_face_token(const char* tok, int* out_vi, int* out_ti, int* out_ni)
{
    *out_vi = 0; *out_ti = 0; *out_ni = 0;

    // Skopiujemy do bufora, żeby łatwo splitować
    char buf[128];
    strncpy(buf, tok, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    char* p = buf;
    char* a = p;
    char* b = NULL;
    char* c = NULL;

    // znajdź pierwszy '/'
    b = strchr(a, '/');
    if (!b) {
        *out_vi = atoi(a);
        *out_ti = 0;
        *out_ni = 0;
        return;
    }

    *b = '\0';
    b++;

    // znajdź drugi '/'
    c = strchr(b, '/');
    if (!c) {
        *out_vi = atoi(a);
        *out_ti = (*b) ? atoi(b) : 0;
        *out_ni = 0;
        return;
    }

    *c = '\0';
    c++;

    *out_vi = atoi(a);
    *out_ti = (*b) ? atoi(b) : 0; // v//n => b=="" => 0
    *out_ni = (*c) ? atoi(c) : 0;
}

/**
 * @brief Buduje Vertex (pos/normal/uv) na podstawie indeksów.
 *
 * @param pos   float array pozycji: x,y,z,x,y,z...
 * @param uv    float array uv: u,v,u,v...
 * @param nor   float array normalnych: x,y,z...
 * @param vi    indeks pozycji (0-based)
 * @param ti    indeks uv (0-based lub -1)
 * @param ni    indeks normalnej (0-based lub -1)
 * @return Vertex
 */
static Vertex make_vertex(const FloatArray* pos, const FloatArray* uv, const FloatArray* nor,
                          int vi, int ti, int ni)
{
    Vertex v;
    // position
    v.position[0] = pos->data[vi*3 + 0];
    v.position[1] = pos->data[vi*3 + 1];
    v.position[2] = pos->data[vi*3 + 2];

    // normal (jeśli brak -> default 0,0,1)
    if (ni >= 0 && (size_t)(ni*3 + 2) < nor->count) {
        v.normal[0] = nor->data[ni*3 + 0];
        v.normal[1] = nor->data[ni*3 + 1];
        v.normal[2] = nor->data[ni*3 + 2];
    } else {
        v.normal[0] = 0.0f; v.normal[1] = 0.0f; v.normal[2] = 1.0f;
    }

    // texcoord (jeśli brak -> 0,0)
    if (ti >= 0 && (size_t)(ti*2 + 1) < uv->count) {
        v.texcoord[0] = uv->data[ti*2 + 0];
        v.texcoord[1] = uv->data[ti*2 + 1];
    } else {
        v.texcoord[0] = 0.0f; v.texcoord[1] = 0.0f;
    }

    return v;
}

/**
 * @brief Wczytuje OBJ i tworzy unikalne wierzchołki + indeksy.
 */
int obj_load(const char* path, ObjModelData* out)
{
    if (!out) return 0;
    memset(out, 0, sizeof(*out));

    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("ERROR: cannot open OBJ: %s\n", path);
        return 0;
    }

    FloatArray positions = {0};
    FloatArray texcoords = {0};
    FloatArray normals   = {0};

    VertexArray vertices = {0};
    UIntArray indices    = {0};

    KeyMap map;
    map_init(&map, 1024);

    char line[1024];

    // Liczniki “elementów” (nie floatów)
    int posCount = 0; // ile v
    int uvCount  = 0; // ile vt
    int norCount = 0; // ile vn

    while (fgets(line, sizeof(line), f))
    {
        char* s = skip_ws(line);
        if (*s == '\0' || *s == '#') continue;

        // v
        if (s[0] == 'v' && isspace((unsigned char)s[1])) {
            float x, y, z;
            if (sscanf(s, "v %f %f %f", &x, &y, &z) == 3) {
                fa_push(&positions, x);
                fa_push(&positions, y);
                fa_push(&positions, z);
                posCount++;
            }
            continue;
        }

        // vt
        if (s[0] == 'v' && s[1] == 't' && isspace((unsigned char)s[2])) {
            float u, v;
            if (sscanf(s, "vt %f %f", &u, &v) >= 2) {
                fa_push(&texcoords, u);
                fa_push(&texcoords, v);
                uvCount++;
            }
            continue;
        }

        // vn
        if (s[0] == 'v' && s[1] == 'n' && isspace((unsigned char)s[2])) {
            float x, y, z;
            if (sscanf(s, "vn %f %f %f", &x, &y, &z) == 3) {
                fa_push(&normals, x);
                fa_push(&normals, y);
                fa_push(&normals, z);
                norCount++;
            }
            continue;
        }

        // f
        if (s[0] == 'f' && isspace((unsigned char)s[1])) {
            // zbierz tokeny wierzchołków face
            // obsłużymy dowolną liczbę wierzchołków i zrobimy triangulację fan
            char* p = s + 1;

            // tymczasowo przechowamy indeksy wierzchołków face (po deduplikacji)
            unsigned int faceIdx[64];
            int faceN = 0;

            while (*p) {
                p = skip_ws(p);
                if (!*p || *p == '\n' || *p == '\r') break;

                // token do spacji
                char tok[128];
                int ti = 0;
                while (*p && !isspace((unsigned char)*p) && ti < (int)sizeof(tok)-1) {
                    tok[ti++] = *p++;
                }
                tok[ti] = '\0';
                if (ti == 0) break;

                int v_i, t_i, n_i;
                parse_face_token(tok, &v_i, &t_i, &n_i);

                int vi0 = resolve_index(v_i, posCount);
                int ti0 = resolve_index(t_i, uvCount);
                int ni0 = resolve_index(n_i, norCount);

                if (vi0 < 0 || vi0 >= posCount) {
                    printf("ERROR: face references invalid position index in %s\n", path);
                    fclose(f);
                    map_free(&map);
                    free(positions.data); free(texcoords.data); free(normals.data);
                    free(vertices.data); free(indices.data);
                    return 0;
                }

                Key key = {vi0, ti0, ni0};

                unsigned int existing = 0;
                int inserted = 0;
                map_get_or_insert(&map, key, &existing, &inserted);

                if (inserted) {
                    Vertex vtx = make_vertex(&positions, &texcoords, &normals, vi0, ti0, ni0);
                    unsigned int newIndex = (unsigned int)vertices.count;
                    va_push(&vertices, vtx);

                    // wpisz wartość do mapy (musimy znaleźć entry i przypisać)
                    uint64_t h = key_hash(key);
                    size_t mask = map.capacity - 1;
                    size_t slot = (size_t)h & mask;
                    for (;;) {
                        Entry* e = &map.entries[slot];
                        if (e->used && key_eq(e->key, key)) {
                            e->value = newIndex;
                            break;
                        }
                        slot = (slot + 1) & mask;
                    }

                    faceIdx[faceN++] = newIndex;
                } else {
                    faceIdx[faceN++] = existing;
                }

                if (faceN >= 64) break; // proste zabezpieczenie
            }

            // triangulacja fan: (0, i, i+1)
            if (faceN >= 3) {
                for (int i = 1; i < faceN - 1; i++) {
                    ua_push(&indices, faceIdx[0]);
                    ua_push(&indices, faceIdx[i]);
                    ua_push(&indices, faceIdx[i + 1]);
                }
            }

            continue;
        }

        // resztę ignorujemy na tym etapie (usemtl/mtllib zrobimy później)
    }

    fclose(f);
    map_free(&map);

    out->vertices = vertices.data;
    out->indices = indices.data;
    out->vertex_count = vertices.count;
    out->index_count = indices.count;

    // pos/uv/nor już nie potrzebne po zbudowaniu VBO/EBO
    free(positions.data);
    free(texcoords.data);
    free(normals.data);

    if (out->vertex_count == 0 || out->index_count == 0) {
        printf("ERROR: OBJ produced empty mesh: %s\n", path);
        obj_free(out);
        return 0;
    }

    return 1;
}

/**
 * @brief Zwalnia dane modelu OBJ.
 */
void obj_free(ObjModelData* data)
{
    if (!data) return;
    free(data->vertices);
    free(data->indices);
    data->vertices = NULL;
    data->indices = NULL;
    data->vertex_count = 0;
    data->index_count = 0;
}
