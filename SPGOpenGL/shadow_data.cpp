#include "shadow_data.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// Global instance
ShadowSystem* g_shadowSystem = nullptr;

// Shader loading utility
std::string readShaderFile(const char* path) {
    std::ifstream file(path);
    std::string content, line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    return content;
}

GLuint compileShaderFromSource(const std::string& source, GLenum type) {
    const char* src = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shadow shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

ShadowSystem::ShadowSystem()
    : sunShadowFBO(0), sunShadowMap(0), shadowShaderProgram(0),
    shadowMapSize(2048), maxChandelierLights(6) {
    chandelierShadowFBOs.resize(maxChandelierLights, 0);
    chandelierShadowMaps.resize(maxChandelierLights, 0);
    chandelierLightSpaceMatrices.resize(maxChandelierLights);
}

ShadowSystem::~ShadowSystem() {
    cleanup();
}

bool ShadowSystem::initialize(int shadowMapSize) {
    this->shadowMapSize = shadowMapSize;

    // Initialize shadow shaders
    if (!initializeShadowShaders()) {
        std::cerr << "Failed to initialize shadow shaders" << std::endl;
        return false;
    }

    // Create sun shadow map
    if (!createShadowMap(sunShadowFBO, sunShadowMap)) {
        std::cerr << "Failed to create sun shadow map" << std::endl;
        return false;
    }

    // Create chandelier shadow maps
    for (int i = 0; i < maxChandelierLights; i++) {
        if (!createShadowMap(chandelierShadowFBOs[i], chandelierShadowMaps[i])) {
            std::cerr << "Failed to create chandelier shadow map " << i << std::endl;
            return false;
        }
    }

    std::cout << "Shadow system initialized successfully" << std::endl;
    return true;
}

bool ShadowSystem::createShadowMap(GLuint& fbo, GLuint& shadowMap) {
    // Generate framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Generate shadow map texture
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Shadow map parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Border color for areas outside shadow map
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

    // No color buffer needed
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool ShadowSystem::initializeShadowShaders() {
    // Shadow vertex shader
    std::string shadowVertexSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

void main() {
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(aPos, 1.0);
}
)";

    // Shadow fragment shader
    std::string shadowFragmentSource = R"(
#version 330 core

void main() {
    // gl_FragDepth is automatically written
}
)";

    // Compile shaders
    GLuint vertexShader = compileShaderFromSource(shadowVertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShaderFromSource(shadowFragmentSource, GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }

    // Create program
    shadowShaderProgram = glCreateProgram();
    glAttachShader(shadowShaderProgram, vertexShader);
    glAttachShader(shadowShaderProgram, fragmentShader);
    glLinkProgram(shadowShaderProgram);

    // Check linking
    GLint success;
    glGetProgramiv(shadowShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shadowShaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shadow shader linking failed: " << infoLog << std::endl;
        return false;
    }

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

glm::mat4 ShadowSystem::calculateLightSpaceMatrix(const glm::vec3& lightPos,
    const glm::vec3& targetPos,
    float nearPlane, float farPlane, float size) {
    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

void ShadowSystem::beginSunShadowPass(const glm::vec3& sunPosition,
    const glm::vec3& sceneCenter,
    float sceneRadius) {
    // Calculate light space matrix for sun
    sunLightSpaceMatrix = calculateLightSpaceMatrix(sunPosition, sceneCenter, 1.0f, 50.0f, sceneRadius);

    // Bind sun shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, sunShadowFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Use shadow shader
    glUseProgram(shadowShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(sunLightSpaceMatrix));

    // Enable depth testing and disable color writes
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Enable front face culling to reduce peter panning
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void ShadowSystem::beginChandelierShadowPass(int lightIndex, const glm::vec3& lightPosition) {
    if (lightIndex < 0 || lightIndex >= maxChandelierLights) return;

    // Calculate light space matrix for chandelier light
    glm::vec3 sceneCenter(0.0f, -1.0f, 0.0f); // Room center
    chandelierLightSpaceMatrices[lightIndex] = calculateLightSpaceMatrix(
        lightPosition, sceneCenter, 0.1f, 25.0f, 8.0f);

    // Bind chandelier shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, chandelierShadowFBOs[lightIndex]);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Use shadow shader
    glUseProgram(shadowShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(chandelierLightSpaceMatrices[lightIndex]));

    // Enable depth testing and disable color writes
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Enable front face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void ShadowSystem::endShadowPass() {
    // Restore default framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 800, 600); // Should be passed as parameter or stored

    // Re-enable color writes
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Restore back face culling
    glCullFace(GL_BACK);
}

void ShadowSystem::bindShadowMapsForRendering(GLuint shaderProgram) {
    // Bind sun shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sunShadowMap);
    glUniform1i(glGetUniformLocation(shaderProgram, "sunShadowMap"), 3);

    // Bind chandelier shadow maps
    for (int i = 0; i < maxChandelierLights && i < 3; i++) { // Limit to 3 for texture units
        glActiveTexture(GL_TEXTURE4 + i);
        glBindTexture(GL_TEXTURE_2D, chandelierShadowMaps[i]);
        std::string uniformName = "chandelierShadowMaps[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(shaderProgram, uniformName.c_str()), 4 + i);
    }
}

void ShadowSystem::setShadowUniforms(GLuint shaderProgram, const glm::mat4& viewMatrix) {
    // Set sun light space matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "sunLightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(sunLightSpaceMatrix));

    // Set chandelier light space matrices
    for (int i = 0; i < maxChandelierLights; i++) {
        std::string uniformName = "chandelierLightSpaceMatrices[" + std::to_string(i) + "]";
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, uniformName.c_str()),
            1, GL_FALSE, glm::value_ptr(chandelierLightSpaceMatrices[i]));
    }
}

GLuint ShadowSystem::getChandelierShadowMap(int index) const {
    if (index >= 0 && index < maxChandelierLights) {
        return chandelierShadowMaps[index];
    }
    return 0;
}

glm::mat4 ShadowSystem::getChandelierLightSpaceMatrix(int index) const {
    if (index >= 0 && index < maxChandelierLights) {
        return chandelierLightSpaceMatrices[index];
    }
    return glm::mat4(1.0f);
}

void ShadowSystem::cleanup() {
    if (sunShadowFBO) {
        glDeleteFramebuffers(1, &sunShadowFBO);
        sunShadowFBO = 0;
    }

    if (sunShadowMap) {
        glDeleteTextures(1, &sunShadowMap);
        sunShadowMap = 0;
    }

    for (int i = 0; i < maxChandelierLights; i++) {
        if (chandelierShadowFBOs[i]) {
            glDeleteFramebuffers(1, &chandelierShadowFBOs[i]);
            chandelierShadowFBOs[i] = 0;
        }

        if (chandelierShadowMaps[i]) {
            glDeleteTextures(1, &chandelierShadowMaps[i]);
            chandelierShadowMaps[i] = 0;
        }
    }

    if (shadowShaderProgram) {
        glDeleteProgram(shadowShaderProgram);
        shadowShaderProgram = 0;
    }
}