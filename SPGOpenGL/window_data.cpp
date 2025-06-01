#include "window_data.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <iostream>

#include "stb_image.h"
using namespace std;

GLuint windowVAO, windowVBO, windowEBO;
GLuint windowShaderProgram;
GLuint windowFrameTex, landscape1Tex, landscape2Tex;

namespace {
    string readFile(const char* path) {
        ifstream f(path);
        string s, line;
        while (getline(f, line)) s += line + "\n";
        return s;
    }

    GLuint compileShader(const char* path, GLenum type) {
        string src = readFile(path);
        const char* c = src.c_str();
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &c, nullptr);
        glCompileShader(sh);

        GLint success;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(sh, 512, NULL, infoLog);
            cerr << "Shader compilation error (" << path << "): " << infoLog << endl;
        }

        return sh;
    }

    GLuint loadTexture(const char* path) {
        GLuint id;
        glGenTextures(1, &id);
        int w, h, comp;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path, &w, &h, &comp, 0);

        if (!data) {
            cerr << "Failed to load texture: " << path << endl;
            return 0;
        }

        GLenum fmt = comp == 4 ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return id;
    }
}

void initWindows() {

    float windowVertices[] = {
        // Pozitie           // Normala        // TexCoord

        -1.0f, -0.8f, 9.8f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f,  // Bottom-left
         1.0f, -0.8f, 9.8f,   0.0f, 0.0f, -1.0f,   1.0f, 0.0f,  // Bottom-right
         1.0f,  0.7f, 9.8f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,  // Top-right
        -1.0f,  0.7f, 9.8f,   0.0f, 0.0f, -1.0f,   0.0f, 1.0f,  // Top-left

        -1.0f, -0.8f, -9.8f,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,  // Bottom-left
         1.0f, -0.8f, -9.8f,  0.0f, 0.0f, 1.0f,    1.0f, 0.0f,  // Bottom-right
         1.0f,  0.7f, -9.8f,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f,  // Top-right
        -1.0f,  0.7f, -9.8f,  0.0f, 0.0f, 1.0f,    0.0f, 1.0f,  // Top-left
    };

    GLuint windowIndices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glGenBuffers(1, &windowEBO);

    glBindVertexArray(windowVAO);

    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(windowVertices), windowVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, windowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(windowIndices), windowIndices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    cout << "Windows geometry initialized successfully!" << endl;
}

void initWindowShaders() {
    GLuint vs = compileShader("window.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("window.frag", GL_FRAGMENT_SHADER);

    windowShaderProgram = glCreateProgram();
    glAttachShader(windowShaderProgram, vs);
    glAttachShader(windowShaderProgram, fs);
    glLinkProgram(windowShaderProgram);

    GLint success;
    glGetProgramiv(windowShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(windowShaderProgram, 512, NULL, infoLog);
        cerr << "Window shader linking error: " << infoLog << endl;
    }
    else {
        cout << "Window shaders compiled and linked successfully!" << endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void loadWindowTextures() {
    windowFrameTex = loadTexture("Textures/Window/frame.png");
    landscape1Tex = loadTexture("Textures/Landscape/landscape1.jpg");
    landscape2Tex = loadTexture("Textures/Landscape/landscape2.jpg");

    if (windowFrameTex && landscape1Tex && landscape2Tex) {
        cout << "Window textures loaded successfully!" << endl;
    }
    else {
        cerr << "Failed to load some window textures!" << endl;
    }
}

void drawWindows(const glm::mat4& projection, const glm::mat4& view,
    const glm::vec3& viewPos, float timeOfDay,
    const glm::vec3& sunPosition, float sunIntensity) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(windowShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;
    glm::mat4 normalMatrix = glm::transpose(glm::inverse(model));

    glUniformMatrix4fv(glGetUniformLocation(windowShaderProgram, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(windowShaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(windowShaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glUniform3fv(glGetUniformLocation(windowShaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform1f(glGetUniformLocation(windowShaderProgram, "timeOfDay"), timeOfDay);
    glUniform3fv(glGetUniformLocation(windowShaderProgram, "sunPosition"), 1, glm::value_ptr(sunPosition));
    glUniform1f(glGetUniformLocation(windowShaderProgram, "sunIntensity"), sunIntensity);

    glBindVertexArray(windowVAO);

    glUniform1i(glGetUniformLocation(windowShaderProgram, "windowFrame"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, windowFrameTex);

    glUniform1i(glGetUniformLocation(windowShaderProgram, "landscape"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, landscape1Tex);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, landscape2Tex);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void cleanupWindows() {
    glDeleteVertexArrays(1, &windowVAO);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1, &windowEBO);
    glDeleteProgram(windowShaderProgram);
    glDeleteTextures(1, &windowFrameTex);
    glDeleteTextures(1, &landscape1Tex);
    glDeleteTextures(1, &landscape2Tex);

    cout << "Window resources cleaned up!" << endl;
}