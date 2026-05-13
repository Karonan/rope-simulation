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

#include "rope.h"
#include "shader.h"

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Rope Simulation", nullptr, nullptr);
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

  // MARK: Rope
  Rope myRope;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Resize callback
  glfwSetFramebufferSizeCallback(window,
                                 [](GLFWwindow *window, int width, int height) {
                                   glViewport(0, 0, width, height);
                                 });

  // Build and compile our shader program
  // Note: Adjust paths if you run the executable from a different directory
  Shader ourShader("../shaders/shader.vs", "../shaders/shader.fs");

  GLuint ropeVAO, ropeVBO;
  glGenVertexArrays(1, &ropeVAO);
  glGenBuffers(1, &ropeVBO);
  glBindVertexArray(ropeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, ropeVBO);
  glBufferData(GL_ARRAY_BUFFER, myRope.particles.size() * sizeof(glm::vec3),
               nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(0);

  // Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Render loop
  float lastFrame = 0.0f;
  while (!glfwWindowShouldClose(window)) {

    // MARK: imgui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Rope Params");
    ImGui::Text("Hello!");
    ImGui::SliderInt("Iterations", &myRope.iterations, 1, 30);
    ImGui::SliderFloat("Damping", &myRope.damping, 0.9f, 1.0f);
    ImGui::End();

    // Input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Render
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate shader
    ourShader.use();

    // Create transformations
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    // For now, let's just use identity matrices or a simple ortho
    // projection = glm::perspective(glm::radians(45.0f), (float)800 /
    // (float)600, 0.1f, 100.0f); view = glm::translate(view, glm::vec3(0.0f,
    // 0.0f, -3.0f));

    ourShader.setMat4("model", model);
    ourShader.setMat4("view", view);
    ourShader.setMat4("projection", projection);
    ourShader.setVec3("objectColor", 0.0f, 0.7f, 0.9f); // Nice blue color

    // MARK: Draw rope
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Update simulation
    if (deltaTime > 0.0f) {
      myRope.update(deltaTime);
    }

    std::vector<glm::vec3> positions;
    for (auto &p : myRope.particles)
      positions.push_back(p.pos);

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