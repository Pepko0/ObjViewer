#pragma once
#include <cglm/cglm.h>

/**
 * @brief Kamera pierwszoosobowa (FPS).
 *
 * Przechowuje pozycję, kierunek patrzenia
 * oraz parametry ruchu i obrotu.
 */
typedef struct Camera {
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 world_up;

    float yaw;    // obrót w poziomie
    float pitch;  // obrót w pionie

    float speed;        // prędkość ruchu
    float sensitivity; // czułość myszy
} Camera;

/**
 * @brief Inicjalizuje kamerę FPS.
 *
 * @param cam Wskaźnik na kamerę.
 */
void camera_init(Camera* cam);

/**
 * @brief Aktualizuje wektory kierunkowe kamery.
 *
 * @param cam Wskaźnik na kamerę.
 */
void camera_update_vectors(Camera* cam);

/**
 * @brief Przesuwa kamerę w zależności od klawiszy WASD.
 *
 * @param cam   Kamera.
 * @param dt    Delta time (sekundy).
 * @param keys  Tablica stanów klawiszy GLFW.
 */
void camera_process_keyboard(Camera* cam, float dt, int* keys);

/**
 * @brief Obraca kamerę na podstawie ruchu myszy.
 *
 * @param cam Kamera.
 * @param dx  Przesunięcie myszy w osi X.
 * @param dy  Przesunięcie myszy w osi Y.
 */
void camera_process_mouse(Camera* cam, float dx, float dy);

/**
 * @brief Zwraca macierz widoku (view matrix).
 *
 * @param cam Kamera.
 * @param out Mat4 wyjściowa.
 */
void camera_get_view_matrix(Camera* cam, mat4 out);
