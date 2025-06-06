#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

float screenWidth = 1600, screenHeight = 800;
static GLuint VBO = 0, VAO = 0;
float deltaTime = 1.0f / 60.0f;
bool isPhysicsPaused = false;

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct PhysicsCircle {
    glm::vec2 position;
    glm::vec2 velocity;
    float radius;
    Color color;
};

struct StaticLine {
    glm::vec4 position;
    Color color;
};

std::vector<PhysicsCircle> circles;
std::vector<StaticLine> lines;

glm::mat4 CreateCamera(float screenWid, float screenHei) {
    return glm::ortho(0.0f, screenWid, 0.0f, screenHei);
}

void cameraUpdate(GLuint shaderProgram, float screenWid, float screenHei) {
    glm::mat4 projection = CreateCamera(screenWidth, screenHeight);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 mvp = projection * view * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
}

const char* LoadShaderFromFile(const char* fileName) {
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to load the shader from file: " << fileName << "\n";
        return nullptr;
    }

    std::string source((std::istreambuf_iterator<char>(inputFile)),
                        std::istreambuf_iterator<char>());
    inputFile.close();

    char* shaderSource = new char[source.size() + 1];
    std::copy(source.begin(), source.end(), shaderSource);
    shaderSource[source.size()] = '\0';

    return shaderSource;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    return shader;
}

GLuint createShaderProgram() {
    const char* vertexShaderSource = LoadShaderFromFile("vertex.shader");
    const char* fragmentShaderSource = LoadShaderFromFile("fragment.shader");
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void drawTriangle(GLuint shaderProgram, float posX, float posY, float scaleX, float scaleY, Color color) {
    
    if (VAO == 0) {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
    model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));

    glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    glm::mat4 view = glm::mat4(1.0f);

    glm::mat4 mvp = projection * view * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    GLuint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);


    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void drawCircle(GLuint shaderProgram, float cx, float cy, float radius, Color color, int segments = 32) {
    static std::vector<float> vertices;
    static GLuint VAO = 0, VBO = 0;

    vertices.clear();
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments;
        float x = cosf(angle) * radius;
        float y = sinf(angle) * radius;
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

    glBindVertexArray(0);
}

void drawLine(GLuint shaderProgram, float x1, float y1, float x2, float y2, Color color, float thickness = 1.0f) {
    static GLuint VAO = 0, VBO = 0;

    float vertices[] = {
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
}

void createPhysicsCircle(float cirX, float cirY, float radius, Color color) {
    PhysicsCircle c;
    c.position = glm::vec2(cirX, cirY);
    c.velocity = glm::vec2(0, 0);
    c.radius = radius;
    c.color = color;

    circles.push_back(c);
}

void createStaticLine(float lineX1, float lineY1, float lineX2, float lineY2, Color color, StaticLine l) {
    l.position = glm::vec4(lineX1, lineY1, lineX2, lineY2);
    l.color = color;

    lines.push_back(l);
}

void updatePhysicsObjects() {
    glm::vec2 gravity(0.0f, -1200.0f);

    for (auto& c : circles) {
        c.velocity += gravity * deltaTime;
        c.position += c.velocity * deltaTime;

        for (auto& l : lines) {
            float floorY = l.position.y;
            if (c.position.y - c.radius < floorY) {
                c.position.y = floorY + c.radius;
                c.velocity.y *= -0.7f;

                if (fabs(c.velocity.y) < 5.0f)
                    c.velocity.y = 0;
            }
        }
    }
}


int main() {
    StaticLine l;
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Physics", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    GLuint shaderProgram = createShaderProgram();

    glClearColor(0.2, 0.3, 0.3, 1);
    
    createPhysicsCircle(300, 600, 100, {1, 0, 1, 1});
    createPhysicsCircle(300, 600, 100, {1, 0, 1, 1});
    createStaticLine(10, 300, 300, 300, {0, 1, 0, 1}, l);
    createStaticLine(10, 200, 300, 300, {0, 1, 0, 1}, l);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        cameraUpdate(shaderProgram, screenWidth, screenHeight);

        for (auto& c : circles) {
            drawCircle(shaderProgram, c.position.x, c.position.y, c. radius, c.color);
        }

        for (auto& l : lines) {
            drawLine(shaderProgram, l.position.x, l.position.y, l.position.z, l.position.w, l.color, 5.0f);
        }

        if (isPhysicsPaused) {

        } else {
            updatePhysicsObjects();
        }

        ImGui::Begin("Inspector");
            ImGui::Text("Enable/Disable physics");
            
            if (ImGui::Button("Enable / Disable")) {
                isPhysicsPaused = !isPhysicsPaused;
            }
            
            int circleIndex = 0;
            for (auto& c : circles) {
                ImGui::Text("Circle %d Position", circleIndex);
                ImGui::SliderFloat(("Y##" + std::to_string(circleIndex)).c_str(), &c.position.y, 0, 801);
                ImGui::SliderFloat(("X##" + std::to_string(circleIndex)).c_str(), &c.position.x, 0, 1601);
                circleIndex++;
            }


            for (auto& l : lines) {
                ImGui::Text("Line Position");
            }

        ImGui::End();
        

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
