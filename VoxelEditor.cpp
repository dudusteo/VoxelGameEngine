#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <glm/matrix.hpp>
#include <iostream>
#include <queue>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "camera/camera.hpp"
#include "file_handler/file_handler.hpp"
#include "material/material.hpp"
#include "object/object.hpp"
#include "shader/shader.hpp"

#define SCR_WIDTH 1280
#define SCR_HEIGHT 720
#define APPLICATION_NAME "VoxelGameEngine"
#define FRAME_TIME_SIZE 60 * 20

// Timings
float currentFrame = 0;
float deltaTime = 0;
float lastFrame = 0;
float frameTime[FRAME_TIME_SIZE];

// temporal
double mouseYOffset = 0;
bool wireVisible = false;
float raycastLength = 1.f;
Material cubeMaterial;
glm::vec3 mouse_ray;

Light light = {{0.f, 0.f, -1.f},
           {0.2f, 0.2f, 0.2f},
           {0.5f, 0.5f, 0.5f},
           {1.0f, 1.0f, 1.0f}};

class DebugDraw {
  public:
    void init() {
      m_shader.init("files/debug.vert", "files/debug.frag");
    }
    void drawLine(glm::vec3 start, glm::vec3 end, glm::mat4 projection, glm::mat4 view) {
      glm::mat4 model = glm::mat4(1.f);

      glGenVertexArrays(1, &m_VAO);
      glBindVertexArray(m_VAO); 

      glGenBuffers(1, &m_VBO);
      std::vector<glm::vec3> t_vertices;
      t_vertices.push_back(end);
      t_vertices.push_back(end + glm::vec3(1.f, 0.f, 0.f));
      

      glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(t_vertices), &t_vertices.front(), GL_STATIC_DRAW);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
      glEnableVertexAttribArray(0);

      m_shader.use();
      m_shader.setVec3("color", glm::vec3(1.f, 0.f, 0.f));
      m_shader.setMat4("projection", projection);
      m_shader.setMat4("view", view);
      m_shader.setMat4("model", model);

      glDrawArrays(GL_LINE_STRIP, 0, t_vertices.size()); 

      glBindVertexArray(0); 
    }
  private:
    uint32_t m_VBO, m_VAO;
    Shader m_shader;
};

class VoxelGameEngine {
public:
  void run() {
    initWindow();
    initOpenGL();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow *window;
  uint32_t VAO, VBO, EBO;
  Shader shader;
  Camera *camera;
  std::vector<Vertex> t_vertices;
  std::vector<uint32_t> t_indices;
  DebugDraw debugDraw;
  Object *object;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    window =
        glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, APPLICATION_NAME, NULL, NULL);
    if (window == NULL) {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      glfwTerminate();
    }

    // Imgui init

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();
  }

  void initOpenGL() {

    loadVertexBuffer(t_vertices);
    loadIndexBuffer(t_indices);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); 
    
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, t_vertices.size() * sizeof(Vertex),
                 &t_vertices.front(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, t_indices.size() * sizeof(uint32_t),
                 &t_indices.front(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)sizeof(glm::vec3));
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    shader.init("files/basic.vert", "files/basic.frag");
    debugDraw.init();
    object = new Object();

    // TEMPORAL STUFF HERE
    camera = new Camera();
    camera->Position = glm::vec3(0.f, 0.f, 20.f);
    memset(frameTime, 0, sizeof(frameTime));

    cubeMaterial = loadMaterial("files/ruby.mat");

    object->addVoxel(glm::ivec3(0, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(2, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(4, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(6, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(8, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(10, 0, 0), cubeMaterial);
    object->addVoxel(glm::ivec3(12, 0, 0), cubeMaterial);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
      drawGUI();
      glfwSwapBuffers(window);
    }
  }

  void drawFrame() {
    currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    for (size_t i = 1; i < FRAME_TIME_SIZE; i++) {
      frameTime[i - 1] = frameTime[i];
    }
    frameTime[FRAME_TIME_SIZE - 1] = deltaTime * 1000;

    glClearColor(1.f, 1.f, 1.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (wireVisible) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    MVP mvp;
    mvp.projection = glm::perspective(
        glm::radians(45.f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.f);
    mvp.view = camera->GetViewMatrix();
    mvp.model = glm::mat4(1.f);

    processInput();
    object->checkRay(camera->Position, getRayCast(mvp.projection, mvp.view));
    //debugDraw.drawLine(camera->Position, mouse_ray, projection, view);

    shader.use();

    shader.setVec3("viewPos", camera->Position);

    shader.setVec3("light.direction", light.direction);
    shader.setVec3("light.ambient", light.ambient);
    shader.setVec3("light.diffuse", light.diffuse);
    shader.setVec3("light.specular", light.specular);

    shader.setMat4("projection", mvp.projection);
    shader.setMat4("view", mvp.view);

    //temp
    std::vector<Voxel> voxels = object->getListOfVoxels();
    glm::mat4 objectModel = mvp.model;

    for (Voxel voxel : voxels) {
      objectModel = glm::translate(mvp.model, voxel.pos);
      shader.setMat4("model", objectModel);
      shader.setVec3("material.ambient", voxel.mat.ambient);
      shader.setVec3("material.diffuse", voxel.mat.diffuse);
      shader.setVec3("material.specular", voxel.mat.specular);
      shader.setFloat("material.shininess", voxel.mat.shininess * 128);
      
      for (int start = 0; start < 31; start += 6) {
        glBindVertexArray(VAO);   
        glDrawElements(GL_TRIANGLES, (GLsizei)36 / 6, GL_UNSIGNED_INT,
                   (void *)(start * sizeof(uint32_t)));
      }
    }
  }

  void drawGUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    debugInfoGUI();
    lightPropertiesGUI();
    objectHandlingGUI();
    rayCastingInfoGUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void cleanup() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void debugInfoGUI() {
    ImGui::Begin("Debug Info");
    if (ImGui::Button("WireFrame mode"))
      wireVisible ^= true;
    ImGui::PlotHistogram("", frameTime, IM_ARRAYSIZE(frameTime), 0, NULL, 0.0f,
                         16.f, ImVec2(200, 80));
    ImGui::End();
  }

  void lightPropertiesGUI() {
    ImGui::Begin("Light Properties");
    ImGui::SliderFloat3("Direction", (float *)&light.direction, -1.f, 1.f);
    ImGui::SliderFloat3("Light Ambient", (float *)&light.ambient, 0.f, 1.f);
    ImGui::SliderFloat3("Light Diffuse", (float *)&light.diffuse, 0.f, 1.f);
    ImGui::SliderFloat3("Light Specular", (float *)&light.specular, 0.f, 1.f);
    ImGui::End();
  }

  void objectHandlingGUI() {
    static glm::ivec3 pos = glm::ivec3(0, 0, 0);
    ImGui::Begin("Object Handler");
    ImGui::InputInt3("Voxel Position", (int *)&pos);
    if(ImGui::Button("Add Voxel"))
      object->addVoxel(pos, cubeMaterial);
    ImGui::End();
  }

  void rayCastingInfoGUI() {
    ImGui::Begin("Ray Casting Info");
    ImGui::InputFloat3("Ray Position", (float *)&mouse_ray);
    ImGui::InputFloat3("Camera Position", (float *)&camera->Position);
    ImGui::InputFloat("Ray Length", &raycastLength);
    ImGui::End();
  }

  void processInput() {
    //ESC to leave
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);

    //Camera Handler
    static bool blockedCamera = true;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS &&
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      camera->ProcessKeyboard((float)xpos, (float)ypos, deltaTime,
                              blockedCamera);

      if (blockedCamera)
        blockedCamera = false;
    } else if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) ==
                GLFW_PRESS)) {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      camera->ProcessMouseMovement((float)xpos, (float)ypos, blockedCamera);

      if (blockedCamera)
        blockedCamera = false;
    } else if (!blockedCamera)
      blockedCamera = true;

    //LMB handle the click
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      
    }
  }

  glm::vec2 getScreenPos() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    xpos = xpos / (SCR_WIDTH / 2) - 1;
    ypos = 1 - ypos / (SCR_HEIGHT / 2);

    return glm::vec2(xpos, ypos);
  }

  glm::vec3 getRayCast(glm::mat4 projection, glm::mat4 view) {
    glm::vec4 ray_point = glm::vec4(getScreenPos(), -1.f, 1.f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_point;
    ray_eye = glm::vec4(glm::vec2(ray_eye), -1.f, 0.f);
    glm::vec3 ray_world = glm::vec3(glm::inverse(view) * ray_eye);
    return ray_world;
  }

  inline static auto scroll_callback(GLFWwindow *window, double xoffset,
                                     double yoffset) -> void {
    VoxelGameEngine *voxelGame =
        static_cast<VoxelGameEngine *>(glfwGetWindowUserPointer(window));
    voxelGame->camera->ProcessMouseScroll((float)yoffset, deltaTime);
  }
};

int main() {
  VoxelGameEngine app;
  app.run();
}
