// main.cpp
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "room_data.h"    
#include <string>
#include <fstream>
#include <iostream>
#include "obj_loader.hpp"
#include "window_data.h" 

#ifndef M_PI
#define M_PI 3.14
#endif


// dimensiunile încăperii (trebuie să fie în sync cu room_data.cpp)
const float ROOM_WIDTH = 4.0f;
const float ROOM_LENGTH = 20.0f;
const float ROOM_HEIGHT = 5.0f;
// înălţimea ochilor deasupra podelei
const float CAMERA_Y = -0.5f;
// cât să stai departe de pereţi
const float CLAMP_EPS = 0.3f;

// limite interioare pe X şi Z
const float ROOM_MIN_X = -ROOM_WIDTH / 2 + CLAMP_EPS;
const float ROOM_MAX_X = ROOM_WIDTH / 2 - CLAMP_EPS;
const float ROOM_MIN_Z = -ROOM_LENGTH / 2 + CLAMP_EPS;
const float ROOM_MAX_Z = ROOM_LENGTH / 2 - CLAMP_EPS;

// fereastră
static const int WIDTH = 800, HEIGHT = 600;

// stare camera
glm::vec3 cameraPos = { 0.0f, -2.0f, 3.0f };
glm::vec3 cameraFront = { 0.0f, 0.0f, -1.0f };
glm::vec3 cameraUp = { 0.0f, 1.0f,  0.0f };
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f;

// timer + input
float deltaTime = 0.0f, lastFrame = 0.0f;
bool  keys[256] = { false };

// GL resurse
GLuint shaderProgram;
GLuint wallDiffuse, wallNormal;
GLuint floorDiffuse, floorNormal;
GLuint ceilDiffuse, ceilNormal;

//Windows
bool autonomicMode = false;
float timeOfDay = 6.0f; // 0-24 ore (6 = dimineața)
float dayDuration = 120.0f; // 2 minute = 1 zi completă
glm::vec3 sunColor = glm::vec3(1.0f, 0.9f, 0.7f);
glm::vec3 sunPosition = glm::vec3(0.0f, 2.0f, 0.0f); // Poziție inițială


glm::vec3 calculateSunPosition(float timeOfDay) {
    float sunAngle = (timeOfDay - 6.0f) / 12.0f * M_PI; // Rasarit la 6, apus la 18
    float sunHeight = sin(sunAngle) * 3.0f; // Înălțime max 3 unități

    // Soarele se mișcă de la geamul din spate (-Z) la cel din față (+Z)
    float sunZ = cos(sunAngle) * ROOM_LENGTH * 0.6f;

    return glm::vec3(0.0f, sunHeight, sunZ);
}

// Calculează intensitatea luminii naturale
float calculateNaturalLightIntensity(float timeOfDay) {
    if (timeOfDay < 6.0f || timeOfDay > 18.0f) return 0.0f; // Noapte
    if (timeOfDay >= 10.0f && timeOfDay <= 14.0f) return 1.0f; // Amiază

    // Tranziții dimineața și seara
    if (timeOfDay < 10.0f) {
        return (timeOfDay - 6.0f) / 4.0f; // Crește de la 0 la 1
    }
    else {
        return (18.0f - timeOfDay) / 4.0f; // Scade de la 1 la 0
    }
}



Mesh chandelier;
GLuint chandelierTex;
glm::vec3 chandelierPos = { 0.0f,  ROOM_HEIGHT - 4.0f, 3.0f };

// Multiple light sources for chandelier bulbs
const int MAX_LIGHTS = 6;
glm::vec3 lightPositions[MAX_LIGHTS];
int numLights = 6;
float lightIntensity = 0.5f;


// utilitar: citeşte un fişier text în std::string
std::string readFile(const char* path) {
    std::ifstream f(path);
    std::string s, line;
    while (std::getline(f, line)) s += line + "\n";
    return s;
}

// compilează un shader din fişier
GLuint compileShader(const char* path, GLenum type) {
    std::string src = readFile(path);
    const char* c = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    return sh;
}

// încarcă şi leagă shaderele
void initShaders() {
    GLuint vs = compileShader("vertex.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("fragment.frag", GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
}

// Initialize light positions around the chandelier
void initLights() {
    float radius = 0.8f; // Radius around chandelier center
    float height = chandelierPos.y - 0.3f; // Slightly below chandelier center

    // Create 6 lights in a circle around the chandelier
    for (int i = 0; i < numLights; i++) {
        float angle = (2.0f * M_PI * i) / numLights;
        lightPositions[i] = glm::vec3(
            chandelierPos.x + radius * cos(angle),
            height,
            chandelierPos.z + radius * sin(angle)
        );
    }
}

// încarcă o textură de pe disc
GLuint loadTex(const char* path) {
    GLuint id; glGenTextures(1, &id);
    int w, h, comp;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &comp, 0);
    GLenum fmt = comp == 4 ? GL_RGBA : GL_RGB;
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    return id;
}

// mişcare & clamp
void doMovement() {
    float speed = 2.5f * deltaTime;
    glm::vec3 fw = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    glm::vec3 rt = glm::normalize(glm::cross(fw, cameraUp));

    if (keys['w']) cameraPos += fw * speed;
    if (keys['s']) cameraPos -= fw * speed;
    if (keys['a']) cameraPos -= rt * speed;
    if (keys['d']) cameraPos += rt * speed;

    // limite interioare pereţi
    cameraPos.x = glm::clamp(cameraPos.x, ROOM_MIN_X, ROOM_MAX_X);
    cameraPos.z = glm::clamp(cameraPos.z, ROOM_MIN_Z, ROOM_MAX_Z );
    // blocăm Y la nivelul ochilor
    cameraPos.y = CAMERA_Y;
}

// input
//void keyDown(unsigned char k, int x, int y) { keys[k] = true; }
void keyUp(unsigned char k, int x, int y) { keys[k] = false; }

void keyDown(unsigned char k, int x, int y) {
    keys[k] = true;

    // Add these controls
    if (k == '+' || k == '=') {
        lightIntensity = std::min(2.0f, lightIntensity + 0.1f);
        std::cout << "Light intensity: " << lightIntensity << std::endl;
    }
    if (k == '-') {
        lightIntensity = std::max(0.0f, lightIntensity - 0.1f);
        std::cout << "Light intensity: " << lightIntensity << std::endl;
    }
    if (k == 'T' || k == 't') {
		autonomicMode = !autonomicMode;
        std::cout << "Autonomic mode: " << (autonomicMode ? "ON" : "OFF") << std::endl;
        std::cout << "Time of day: " << timeOfDay << "h" << std::endl;
    }
    if (k == 'n' || k == 'N') {  // Next hour (avansează timpul)
        timeOfDay += 1.0f;
        if (timeOfDay >= 24.0f) timeOfDay = 0.0f;

        // Actualizează lighting și soare
        float naturalLight = calculateNaturalLightIntensity(timeOfDay);
        lightIntensity = 0.2f + (1.0f - naturalLight) * 0.8f;
        sunPosition = calculateSunPosition(timeOfDay);

        std::cout << "Time: " << (int)timeOfDay << ":00h - Light: " << lightIntensity << std::endl;
    }
}

// mouse look
void mouseMove(int x, int y) {
    static int cx = WIDTH / 2, cy = HEIGHT / 2;
    float dx = (x - cx) * 0.1f, dy = (cy - y) * 0.1f;
    yaw += dx; pitch += dy;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(dir);
    glutWarpPointer(cx, cy);
}

void reshape(int w, int h) { glViewport(0, 0, w, h); }
void idle() { glutPostRedisplay(); }

// Helper function to set lighting uniforms
void setLightingUniforms(GLuint program, const glm::vec3& viewPos) {
    glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform1i(glGetUniformLocation(program, "numLights"), numLights);
    glUniform1f(glGetUniformLocation(program, "lightIntensity"), lightIntensity);  // Add this line

    for (int i = 0; i < numLights; i++) {
        std::string uniformName = "lightPositions[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, uniformName.c_str()), 1, glm::value_ptr(lightPositions[i]));
    }

    glUniform1f(glGetUniformLocation(program, "normalMapStrength"), 1.0f);
}

// desenare
void display() {
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = now - lastFrame; 
    lastFrame = now;
    doMovement();


    if (autonomicMode) {
        timeOfDay += deltaTime * 24.0f / dayDuration; // 24 ore în dayDuration secunde
        if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;

        // Calculează intensitatea luminilor artificiale (invers proporțional cu lumina naturală)
        float naturalLight = calculateNaturalLightIntensity(timeOfDay);
        lightIntensity = 0.2f + (1.0f - naturalLight) * 0.8f; // Min 0.2, max 1.0

        sunPosition = calculateSunPosition(timeOfDay);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // proiectie + view
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::vec3 viewPos = cameraPos;

    // desenare chandelier
    glm::mat4 chandModel = glm::translate(glm::mat4(1.0f), chandelierPos);
    chandModel = glm::scale(chandModel, glm::vec3(1.0f));

    glm::mat4 chandMVP = proj * view * chandModel;
    glm::mat4 chandNormalMatrix = glm::transpose(glm::inverse(chandModel));

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(chandMVP));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(chandModel));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(chandNormalMatrix));

    // Set lighting uniforms
    setLightingUniforms(shaderProgram, viewPos);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, chandelierTex);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, chandelierTex); // Using same texture as normal map fallback

    glBindVertexArray(chandelier.vao);
    glDrawElements(GL_TRIANGLES, chandelier.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    // draw room with updated lighting system
    drawRoom(proj, view, lightPositions, numLights, viewPos, 1.0f, lightIntensity);
    if (autonomicMode) {
        float sunIntensity = calculateNaturalLightIntensity(timeOfDay);
        drawWindows(proj, view, viewPos, timeOfDay, sunPosition, sunIntensity);
    }
    else {
        // În modul manual, afișează geamurile cu setări fixe
        drawWindows(proj, view, viewPos, 12.0f, glm::vec3(0, 2, 0), 1.0f);
    }

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Room");

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);

    // callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(mouseMove);
    glutIdleFunc(idle);

    // shaders + texturi + iniţializare room
    initShaders();
	initWindowShaders();
	loadWindowTextures();

    // Initialize lighting system
    initLights();
	initWindows();


    // Încarcă texturile
    wallDiffuse = loadTex("Textures/Wall/wall_Color.jpg");
    wallNormal = loadTex("Textures/Wall/wall_NormalGL.jpg");
    floorDiffuse = loadTex("Textures/FloorWood/floor_Color.jpg");
    floorNormal = loadTex("Textures/FloorWood/floor_NormalGL.jpg");
    ceilDiffuse = loadTex("Textures/Ceiling/ceiling_Color.jpg");
    ceilNormal = loadTex("Textures/Ceiling/ceiling_NormalGL.jpg");
    chandelier = loadOBJ("Objects/Chandelier/chandelier.obj");
    chandelierTex = loadTex("Objects/Chandelier/chandelier_diffuse.jpg");

    // Inițializează room cu toate texturile
    initRoom(wallDiffuse, wallNormal, floorDiffuse, floorNormal, ceilDiffuse, ceilNormal, shaderProgram);

    glutMainLoop();
    return 0;
}