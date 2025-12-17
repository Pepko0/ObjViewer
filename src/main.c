#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "ObjLoader.h"
#include "Material.h"

/* =========================================================
   Zmienne globalne do obsługi kamery i inputu
   ========================================================= */

Camera camera;
int keys[1024] = {0};

float lastX = 640.0f;
float lastY = 360.0f;
int firstMouse = 1;

/* =========================================================
   Callbacki GLFW
   ========================================================= */

/**
 * @brief Callback zmiany rozmiaru okna.
 */
static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/**
 * @brief Callback klawiatury (WASD).
 */
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = 1;
        else if (action == GLFW_RELEASE)
            keys[key] = 0;
    }
}

/**
 * @brief Callback ruchu myszy (obrót kamery).
 */
static void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = 0;
    }

    float dx = (float)(xpos - lastX);
    float dy = (float)(lastY - ypos); // odwrócona oś Y

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera_process_mouse(&camera, dx, dy);
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    /* ---------- GLFW init ---------- */
    if (!glfwInit())
    {
        printf("GLFW init failed\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "OBJ Viewer (C)", NULL, NULL);
    if (!window)
    {
        printf("Window creation failed\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* ---------- GLAD ---------- */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to init GLAD\n");
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    /* ---------- Shader ---------- */
    ShaderProgram sh = shader_load_from_files(
        "shaders/basic.vert",
        "shaders/basic.frag");

    if (!sh.id)
    {
        printf("Shader load failed\n");
        glfwTerminate();
        return -1;
    }

    /* ---------- Kamera ---------- */
    camera_init(&camera);

    /* ---------- Macierze ---------- */
    mat4 model, proj;
    glm_mat4_identity(model);

    glm_perspective(
        glm_rad(60.0f),
        1280.0f / 720.0f,
        0.1f,
        100.0f,
        proj);

    shader_use(sh);
    glUniform3f(
        glGetUniformLocation(sh.id, "uLightDir"),
        -0.3f, -1.0f, -0.5f);

    GLint locModel = glGetUniformLocation(sh.id, "uModel");
    GLint locView = glGetUniformLocation(sh.id, "uView");
    GLint locProj = glGetUniformLocation(sh.id, "uProjection");

    glUniformMatrix4fv(locModel, 1, GL_FALSE, (float *)model);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, (float *)proj);

    /* ---------- Mesh (trójkąt testowy) ---------- */
    ObjModelData modelData;
    if (!obj_load("assets/models/model.obj", &modelData))
    {
        printf("Failed to load OBJ\n");
        shader_destroy(&sh);
        glfwTerminate();
        return -1;
    }
    Material mat;
    material_load_mtl("assets/models/model.mtl", &mat);

    Mesh modelMesh = mesh_create(
        modelData.vertices,
        (unsigned int)modelData.vertex_count,
        modelData.indices,
        (unsigned int)modelData.index_count);

    // dane CPU nie są już potrzebne po wrzuceniu do GPU
    obj_free(&modelData);

    /* ---------- Pętla renderująca ---------- */
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        camera_process_keyboard(&camera, deltaTime, keys);

        glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader_use(sh);

        mat4 view;
        camera_get_view_matrix(&camera, view);
        glUniformMatrix4fv(locView, 1, GL_FALSE, (float *)view);

        material_bind(&mat, sh.id);
        mesh_draw(&modelMesh);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    /* ---------- Cleanup ---------- */
    mesh_destroy(&modelMesh);
    shader_destroy(&sh);

    glfwTerminate();
    return 0;
}
