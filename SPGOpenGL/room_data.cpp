#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
using namespace std;

GLuint roomVAO;
GLuint roomVBO, roomEBO;
GLuint texWall, normWall;
GLuint texFloor, normFloor;
GLuint texCeil, normCeil;
GLuint shader_programme;

float roomVertices[] = {
    // poziție             // normală        // texCoord
    // Podea (y = -1)
    -2, -1, -10,   0, 1, 0,   0, 0,
     2, -1, -10,   0, 1, 0,   2, 0,
     2, -1,  10,   0, 1, 0,   2, 10,
    -2, -1,  10,   0, 1, 0,   0, 10,

    // Tavan (y = 1)
    -2, 1, -10,   0, -1, 0,   0, 0,
     2, 1, -10,   0, -1, 0,   4, 0,
     2, 1,  10,   0, -1, 0,   4, 10,
    -2, 1,  10,   0, -1, 0,   0, 10,

    // Perete spate (z = -10)
    -2, -1, -10,   0, 0, 1,   0, 0,
     2, -1, -10,   0, 0, 1,   4, 0,
     2,  1, -10,   0, 0, 1,   4, 2,
    -2,  1, -10,   0, 0, 1,   0, 2,

    // Perete față (z = 10)
    -2, -1, 10,   0, 0, -1,   0, 0,
     2, -1, 10,   0, 0, -1,   4, 0,
     2,  1, 10,   0, 0, -1,   4, 2,
    -2,  1, 10,   0, 0, -1,   0, 2,

    // Perete stânga (x = -2)
    -2, -1, -10,   1, 0, 0,   0, 0,
    -2, -1,  10,   1, 0, 0,   20, 0,
    -2,  1,  10,   1, 0, 0,   20, 2,
    -2,  1, -10,   1, 0, 0,   0, 2,

    // Perete dreapta (x = 2)
     2, -1, -10,  -1, 0, 0,   0, 0,
     2, -1,  10,  -1, 0, 0,   20, 0,
     2,  1,  10,  -1, 0, 0,   20, 2,
     2,  1, -10,  -1, 0, 0,   0, 2,
};

GLuint roomIndices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9,10,10,11, 8,
   12,13,14,14,15,12,
   16,17,18,18,19,16,
   20,21,22,22,23,20
};

void initRoom(GLuint wallTex, GLuint wallNorm,
    GLuint floorTex, GLuint floorNorm,
    GLuint ceilTex, GLuint ceilNorm,
    GLuint shader)
{
    texWall = wallTex;
    normWall = wallNorm;
    texFloor = floorTex;
    normFloor = floorNorm;
    texCeil = ceilTex;
    normCeil = ceilNorm;
    shader_programme = shader;

    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glGenBuffers(1, &roomEBO);

    glBindVertexArray(roomVAO);

    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roomVertices), roomVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roomEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(roomIndices), roomIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}


void setRoomLightingUniforms(const glm::vec3* lightPositions, int numLights, const glm::vec3& viewPos, float normalMapStrength, float lightIntensity) {
    glUniform3fv(glGetUniformLocation(shader_programme, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform1i(glGetUniformLocation(shader_programme, "numLights"), numLights);
    
    float adjustedIntensity = lightIntensity + 0.5f;
    glUniform1f(glGetUniformLocation(shader_programme, "lightIntensity"), adjustedIntensity);

    for (int i = 0; i < numLights && i < 6; i++) {
        string uniformName = "lightPositions[" + to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shader_programme, uniformName.c_str()), 1, glm::value_ptr(lightPositions[i]));
    }

    glUniform1f(glGetUniformLocation(shader_programme, "normalMapStrength"), normalMapStrength);
}

void drawRoom(const glm::mat4& projection, const glm::mat4& view, const glm::vec3* lightPositions, int numLights, const glm::vec3& viewPos, float normalMapStrength, float lightIntensity, const glm::vec3& sunPosition,
    float sunIntensity,
    float timeOfDay) {
    glUseProgram(shader_programme);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;
    glm::mat4 normalMatrix = glm::transpose(glm::inverse(model));

    glUniformMatrix4fv(glGetUniformLocation(shader_programme, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(shader_programme, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader_programme, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    setRoomLightingUniforms(lightPositions, numLights, viewPos, normalMapStrength, lightIntensity);


    glBindVertexArray(roomVAO);

    // Fața 0 (podeaua)
    glUniform1i(glGetUniformLocation(shader_programme, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texFloor);

    glUniform1i(glGetUniformLocation(shader_programme, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normFloor);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));

    // Fața 1 (tavan)
    glUniform1i(glGetUniformLocation(shader_programme, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texCeil);

    glUniform1i(glGetUniformLocation(shader_programme, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normCeil);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));

    // Fețele 2-5 (pereții)
    glUniform1i(glGetUniformLocation(shader_programme, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texWall);

    glUniform1i(glGetUniformLocation(shader_programme, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normWall);

    for (int i = 2; i <= 5; i++) {
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(GLuint)));
    }

    glBindVertexArray(0);
}