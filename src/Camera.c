#include "camera.h"
#include <math.h>

/**
 * @brief Inicjalizacja domyślnej kamery FPS.
 */
void camera_init(Camera* cam)
{
    glm_vec3_copy((vec3){0.0f, 0.0f, 2.0f}, cam->position);
    glm_vec3_copy((vec3){0.0f, 0.0f, -1.0f}, cam->front);
    glm_vec3_copy((vec3){0.0f, 1.0f,  0.0f}, cam->world_up);

    cam->yaw   = -90.0f;
    cam->pitch = 0.0f;

    cam->speed = 2.5f;
    cam->sensitivity = 0.1f;

    camera_update_vectors(cam);
}

/**
 * @brief Aktualizuje wektory front/right/up kamery.
 */
void camera_update_vectors(Camera* cam)
{
    vec3 front;
    front[0] = cosf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
    front[1] = sinf(glm_rad(cam->pitch));
    front[2] = sinf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
    glm_vec3_normalize(front);
    glm_vec3_copy(front, cam->front);

    glm_vec3_cross(cam->front, cam->world_up, cam->right);
    glm_vec3_normalize(cam->right);

    glm_vec3_cross(cam->right, cam->front, cam->up);
    glm_vec3_normalize(cam->up);
}

/**
 * @brief Obsługa klawiatury (WASD).
 */
void camera_process_keyboard(Camera* cam, float dt, int* keys)
{
    float velocity = cam->speed * dt;
    vec3 tmp;

    if (keys['W']) {
        glm_vec3_scale(cam->front, velocity, tmp);
        glm_vec3_add(cam->position, tmp, cam->position);
    }
    if (keys['S']) {
        glm_vec3_scale(cam->front, velocity, tmp);
        glm_vec3_sub(cam->position, tmp, cam->position);
    }
    if (keys['A']) {
        glm_vec3_scale(cam->right, velocity, tmp);
        glm_vec3_sub(cam->position, tmp, cam->position);
    }
    if (keys['D']) {
        glm_vec3_scale(cam->right, velocity, tmp);
        glm_vec3_add(cam->position, tmp, cam->position);
    }
}

/**
 * @brief Obsługa ruchu myszy.
 */
void camera_process_mouse(Camera* cam, float dx, float dy)
{
    dx *= cam->sensitivity;
    dy *= cam->sensitivity;

    cam->yaw   += dx;
    cam->pitch += dy;

    if (cam->pitch > 89.0f)  cam->pitch = 89.0f;
    if (cam->pitch < -89.0f) cam->pitch = -89.0f;

    camera_update_vectors(cam);
}

/**
 * @brief Buduje macierz widoku (lookAt).
 */
void camera_get_view_matrix(Camera* cam, mat4 out)
{
    vec3 target;
    glm_vec3_add(cam->position, cam->front, target);
    glm_lookat(cam->position, target, cam->up, out);
}
