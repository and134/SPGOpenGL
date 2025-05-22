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
//glm::vec3 cameraPos = { 0.0f, CAMERA_Y +500, ROOM_LENGTH / 2 - CLAMP_EPS };
glm::vec3 cameraPos = { 0.0f, -2.0f, 3.0f};
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
    cameraPos.z = glm::clamp(cameraPos.z, ROOM_MIN_Z, ROOM_MAX_Z);
    // blocăm Y la nivelul ochilor
    cameraPos.y = CAMERA_Y;
}

// input
void keyDown(unsigned char k, int x, int y) { keys[k] = true; }
void keyUp(unsigned char k, int x, int y) { keys[k] = false; }

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

// desenare
void display() {
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = now - lastFrame; lastFrame = now;
    doMovement();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // proiectie + view
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // lumina în centru tavan
    glm::vec3 lightPos = { 0.0f, ROOM_HEIGHT, 0.0f };
    glm::vec3 viewPos = cameraPos;

    // apel către room_data
    drawRoom(proj, view, lightPos, viewPos, 1.0f);

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
    // Încarcă texturile
    wallDiffuse = loadTex("Textures/Wall/wall_Color.jpg");
    wallNormal = loadTex("Textures/Wall/wall_NormalGL.jpg");
    floorDiffuse = loadTex("Textures/FloorWood/floor_Color.jpg");
    floorNormal = loadTex("Textures/FloorWood/floor_NormalGL.jpg");
    ceilDiffuse = loadTex("Textures/Ceiling/ceiling_Color.jpg");
    ceilNormal = loadTex("Textures/Ceiling/ceiling_NormalGL.jpg");

    // Inițializează room cu toate texturile
    initRoom(wallDiffuse, wallNormal, floorDiffuse, floorNormal, ceilDiffuse, ceilNormal, shaderProgram);


    glutMainLoop();
    return 0;
}
