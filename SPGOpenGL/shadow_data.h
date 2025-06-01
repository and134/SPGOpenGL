#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class ShadowSystem {
public:
    // Constructor/Destructor
    ShadowSystem();
    ~ShadowSystem();

    // Initialization
    bool initialize(int shadowMapSize = 2048);
    void cleanup();

    // Shadow map generation
    void beginSunShadowPass(const glm::vec3& sunPosition, const glm::vec3& sceneCenter, float sceneRadius);
    void beginChandelierShadowPass(int lightIndex, const glm::vec3& lightPosition);
    void endShadowPass();

    // Rendering with shadows
    void bindShadowMapsForRendering(GLuint shaderProgram);
    void setShadowUniforms(GLuint shaderProgram, const glm::mat4& viewMatrix);

    // Getters
    GLuint getSunShadowMap() const { return sunShadowMap; }
    GLuint getChandelierShadowMap(int index) const;
    glm::mat4 getSunLightSpaceMatrix() const { return sunLightSpaceMatrix; }
    glm::mat4 getChandelierLightSpaceMatrix(int index) const;

    // Shadow shader programs
    GLuint getShadowShaderProgram() const { return shadowShaderProgram; }

private:
    // Shadow map resources
    GLuint sunShadowFBO;
    GLuint sunShadowMap;
    std::vector<GLuint> chandelierShadowFBOs;
    std::vector<GLuint> chandelierShadowMaps;

    // Shadow matrices
    glm::mat4 sunLightSpaceMatrix;
    std::vector<glm::mat4> chandelierLightSpaceMatrices;

    // Shader programs
    GLuint shadowShaderProgram;

    // Configuration
    int shadowMapSize;
    int maxChandelierLights;

    // Helper functions
    bool createShadowMap(GLuint& fbo, GLuint& shadowMap);
    bool initializeShadowShaders();
    glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& targetPos,
        float nearPlane, float farPlane, float size = 10.0f);
};

// Global instance
extern ShadowSystem* g_shadowSystem;