#include <glad/glad.h>
// GLAD must be included before GLFW
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "camera.h"
#include "rope.h"
#include "shader.h"

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        "Rope Simulation", nullptr, nullptr);
  if (!window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // Callbacks
  glfwSetFramebufferSizeCallback(window,
                                 [](GLFWwindow *window, int width, int height) {
                                   glViewport(0, 0, width, height);
                                 });
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Build and compile our shader program
  // Note: Adjust paths if you run the executable from a different directory
  Shader ourShader("../shaders/shader.vs", "../shaders/shader.fs");

  // MARK: VAO, VBO
  // Rope
  Rope myRope;

  GLuint ropeVAO, ropeVBO;
  glGenVertexArrays(1, &ropeVAO);
  glGenBuffers(1, &ropeVBO);
  glBindVertexArray(ropeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, ropeVBO);
  glBufferData(GL_ARRAY_BUFFER, myRope.particles.size() * sizeof(glm::vec3),
               nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(0);

  // Floor
  float floorSize = 1.0f;
  float floorHalf = floorSize * 0.5f;
  float floorVertices[] = {
      // Positions // Normals
      floorHalf,  myRope.floor_y, floorHalf,  0.0f, 1.0f, 0.0f, // top right
      floorHalf,  myRope.floor_y, -floorHalf, 0.0f, 1.0f, 0.0f, // bottom right
      -floorHalf, myRope.floor_y, -floorHalf, 0.0f, 1.0f, 0.0f, // bottom left
      -floorHalf, myRope.floor_y, floorHalf,  0.0f, 1.0f, 0.0f  // top left
  };

  GLuint floorVAO, floorVBO;
  glGenVertexArrays(1, &floorVAO);
  glGenBuffers(1, &floorVBO);
  glBindVertexArray(floorVAO);
  glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices,
               GL_STATIC_DRAW);
  // Positions
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // Normals
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Render loop
  while (!glfwWindowShouldClose(window)) {
    // Timing
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Input
    processInput(window);

    // MARK: imgui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Rope Params");
    ImGui::Text("Hello!");
    ImGui::SliderInt("Iterations", &myRope.iterations, 1, 30);
    ImGui::SliderFloat("Damping", &myRope.damping, 0.9f, 1.0f);
    ImGui::SliderFloat("Bounciness", &myRope.restitution, 0.0f, 1.0f);
    ImGui::Checkbox("XPBD", &myRope.xpbd);
    if (myRope.xpbd) {
      ImGui::SliderFloat("Compliance", &myRope.compliance, 0.0f, 0.01f, "%.5f");
    }
    ImGui::End();

    // MARK: Render
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate shader
    ourShader.use();

    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Floor
    ourShader.use();
    glm::mat4 modelFloor = glm::mat4(1.0f);
    ourShader.setMat4("model", modelFloor);
    ourShader.setMat4("view", view);
    ourShader.setMat4("projection", projection);
    ourShader.setVec3("objectColor", 0.5f, 0.5f, 0.5f); // Grey color

    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Update simulation
    if (deltaTime > 0.0f) {
      myRope.update(deltaTime);
    }

    // Rope
    std::vector<glm::vec3> positions;
    for (auto &p : myRope.particles)
      positions.push_back(p.pos);

    glm::mat4 model = glm::mat4(1.0f);

    ourShader.setMat4("model", model);
    ourShader.setMat4("view", view);
    ourShader.setMat4("projection", projection);
    ourShader.setVec3("objectColor", 0.0f, 0.7f, 0.9f); // Nice blue color

    glBindBuffer(GL_ARRAY_BUFFER, ropeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3),
                    positions.data());

    glUseProgram(ourShader.ID);
    glBindVertexArray(ropeVAO);
    glDrawArrays(GL_LINE_STRIP, 0, myRope.particles.size());

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers and poll IO events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // De-allocate resources
  glDeleteVertexArrays(1, &ropeVAO);
  glDeleteBuffers(1, &ropeVBO);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
  if (ImGui::GetIO().WantCaptureKeyboard)
    return;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  if (ImGui::GetIO().WantCaptureMouse)
    return;

  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    camera.ProcessMouseMovement(xoffset, yoffset);
  }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  if (ImGui::GetIO().WantCaptureMouse)
    return;
  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}