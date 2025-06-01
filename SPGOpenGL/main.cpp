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
using namespace std;
//Dimensiuni camera
const float ROOM_WIDTH = 4.0f;
const float ROOM_LENGTH = 20.0f;
const float ROOM_HEIGHT = 5.0f;

const float CAMERA_Y = -0.5f;
//Inaltimea mea
const float CLAMP_EPS = 0.3f;
//Limite
const float ROOM_MIN_X = -ROOM_WIDTH / 2 + CLAMP_EPS;
const float ROOM_MAX_X = ROOM_WIDTH / 2 - CLAMP_EPS;
const float ROOM_MIN_Z = -ROOM_LENGTH / 2 + CLAMP_EPS;
const float ROOM_MAX_Z = ROOM_LENGTH / 2 - CLAMP_EPS;

// Fereastra
static const int WIDTH = 800, HEIGHT = 600;

// Fariabile camera
glm::vec3 cameraPos = { 0.0f, -2.0f, 3.0f };
glm::vec3 cameraFront = { 0.0f, 0.0f, -1.0f };
glm::vec3 cameraUp = { 0.0f, 1.0f,  0.0f };
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f;

// GL resurse
GLuint shaderProgram;
GLuint wallDiffuse, wallNormal;
GLuint floorDiffuse, floorNormal;
GLuint ceilDiffuse, ceilNormal;

bool  keys[256] = { false };

//timer + input
float deltaTime = 0.0f, lastFrame = 0.0f;

//Soare
bool autonomicMode = false;
float timeOfDay = 12.0f;
float dayDuration = 120.0f; // 2 minute = 1 zi completă
glm::vec3 sunColor = glm::vec3(1.0f, 0.9f, 0.7f);
glm::vec3 sunPosition = glm::vec3(0.0f, 2.0f, 0.0f);

//Candelabru
Mesh chandelier;
GLuint chandelierTex;
glm::vec3 chandelierPos = { 0.0f,  ROOM_HEIGHT - 4.0f, 3.0f };

//Lumina candelabru
const int MAX_LIGHTS = 6;
glm::vec3 lightPositions[MAX_LIGHTS];
int numLights = 6;
float lightIntensity = 0.0f; // Schimbat la 0.0f pentru a începe cu candelabrul oprit
bool chandelierEnabled = false; // Nou: control explicit pentru candelabru

//Masa
Mesh table;
GLuint tableTex;
glm::vec3 tablePos = { 1.0f, -0.95f, -5.0f };
glm::vec3 tableScale = { 0.3f, 0.3f, 0.3f };
float tableRotation = 0.0f;

//Lumina Soarelui - Nou control independent
float sunIntensityManual = 0.0f; // Începe cu soarele oprit pentru testare
bool sunEnabled = false; // Începe cu soarele oprit

//SHADER

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
    return sh;
}

void initShaders() {
    GLuint vs = compileShader("vertex.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("fragment.frag", GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
}

//SOARE

// Logica Soare 
// Miscarea soarelui se face de la o fereasta (-Z) la cealalta (Z)
glm::vec3 calculateSunPosition(float timeOfDay) {
    float sunAngle = (timeOfDay - 6.0f) / 12.0f * M_PI;
    float sunHeight = sin(sunAngle) * 4.0f + 1.0f;
    float sunZ = cos(sunAngle) * ROOM_LENGTH * 0.8f;

    return glm::vec3(0.0f, sunHeight, sunZ);
}

// Logica intensitate Soare - modificat pentru control manual
float calculateNaturalLightIntensity(float timeOfDay) {
    if (!sunEnabled) return 0.0f; // Dacă soarele e dezactivat, returnează 0

    if (timeOfDay < 6.0f || timeOfDay > 18.0f) return 0.0f;
    if (timeOfDay >= 10.0f && timeOfDay <= 14.0f) return sunIntensityManual;

    if (timeOfDay < 10.0f) {
        return (timeOfDay - 6.0f) / 4.0f * sunIntensityManual;
    }
    else {
        return (18.0f - timeOfDay) / 4.0f * sunIntensityManual;
    }
}

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
        cerr << "Failed to load texture: " << path << endl;
    }
    return id;
}

//COLLISION DETECTION

bool checkBoxCollision(const glm::vec3& pos, const glm::vec3& boxCenter, const glm::vec3& boxSize) {
    float margin = 0.3f;
    return (pos.x >= boxCenter.x - boxSize.x - margin && pos.x <= boxCenter.x + boxSize.x + margin &&
        pos.z >= boxCenter.z - boxSize.z - margin && pos.z <= boxCenter.z + boxSize.z + margin);
}
bool checkTableCollision(const glm::vec3& pos) {
    glm::vec3 tableSize = glm::vec3(1.5f, 0.5f, 0.8f) * tableScale.x;
    return checkBoxCollision(pos, tablePos, tableSize);
}

bool checkAllCollisions(const glm::vec3& newPos) {
    return checkTableCollision(newPos);
}

void doMovement() {
    float speed = 2.5f * deltaTime;
    glm::vec3 fw = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    glm::vec3 rt = glm::normalize(glm::cross(fw, cameraUp));

    glm::vec3 newPos = cameraPos;

    if (keys['w']) newPos += fw * speed;
    if (keys['s']) newPos -= fw * speed;
    if (keys['a']) newPos -= rt * speed;
    if (keys['d']) newPos += rt * speed;

    newPos.x = glm::clamp(newPos.x, ROOM_MIN_X, ROOM_MAX_X);
    newPos.z = glm::clamp(newPos.z, ROOM_MIN_Z, ROOM_MAX_Z);
    newPos.y = CAMERA_Y; // Menține înălțimea constantă

    // Verifică coliziunile cu obiectele
    if (!checkAllCollisions(newPos)) {
        // Nu există coliziuni - actualizează poziția
        cameraPos = newPos;
    }
    else {
        // Există coliziune - încearcă mișcare parțială pe fiecare axă
        glm::vec3 testPos;

        // Încearcă doar mișcarea pe X
        testPos = cameraPos;
        if (keys['w']) testPos += glm::vec3(fw.x, 0, 0) * speed;
        if (keys['s']) testPos -= glm::vec3(fw.x, 0, 0) * speed;
        if (keys['a']) testPos -= glm::vec3(rt.x, 0, 0) * speed;
        if (keys['d']) testPos += glm::vec3(rt.x, 0, 0) * speed;
        testPos.x = glm::clamp(testPos.x, ROOM_MIN_X, ROOM_MAX_X);

        if (!checkAllCollisions(testPos)) {
            cameraPos.x = testPos.x;
        }

        // Încearcă doar mișcarea pe Z
        testPos = cameraPos;
        if (keys['w']) testPos += glm::vec3(0, 0, fw.z) * speed;
        if (keys['s']) testPos -= glm::vec3(0, 0, fw.z) * speed;
        if (keys['a']) testPos -= glm::vec3(0, 0, rt.z) * speed;
        if (keys['d']) testPos += glm::vec3(0, 0, rt.z) * speed;
        testPos.z = glm::clamp(testPos.z, ROOM_MIN_Z, ROOM_MAX_Z);

        if (!checkAllCollisions(testPos)) {
            cameraPos.z = testPos.z;
        }
    }
}

//Reglaje obiecte/lumini - ACTUALIZAT cu noi controale
void keyDown(unsigned char k, int x, int y) {
    keys[k] = true;

    // Controale pentru candelabru
    if (k == '+' || k == '=') {
        if (chandelierEnabled) {
            lightIntensity = min(2.0f, lightIntensity + 0.1f);
            cout << "Chandelier intensity: " << lightIntensity << endl;
        }
    }
    if (k == '-') {
        if (chandelierEnabled) {
            lightIntensity = max(0.0f, lightIntensity - 0.1f);
            cout << "Chandelier intensity: " << lightIntensity << endl;
        }
    }

    // Control ON/OFF pentru candelabru
    if (k == 'c' || k == 'C') {
        chandelierEnabled = !chandelierEnabled;
        if (!chandelierEnabled) {
            lightIntensity = 0.0f;
        }
        else {
            lightIntensity = 0.5f; // Setează o valoare inițială când se pornește
        }
        cout << "Chandelier: " << (chandelierEnabled ? "ON" : "OFF")
            << " - Intensity: " << lightIntensity << endl;
    }

    // Controale pentru soare
    if (k == 'u' || k == 'U') {
        sunIntensityManual = min(3.0f, sunIntensityManual + 0.1f);
        cout << "Sun intensity: " << sunIntensityManual << endl;
    }
    if (k == 'j' || k == 'J') {
        sunIntensityManual = max(0.0f, sunIntensityManual - 0.1f);
        cout << "Sun intensity: " << sunIntensityManual << endl;
    }

    // Control ON/OFF pentru soare
    if (k == 's' && keys[27]) { // Ctrl+S pentru a evita conflictul cu mișcarea
        // Nu folosim Ctrl+S, folosim altă tastă
    }
    if (k == 'p' || k == 'P') { // P pentru sun (sPace/sun)
        sunEnabled = !sunEnabled;
        cout << "Sun: " << (sunEnabled ? "ON" : "OFF")
            << " - Intensity: " << sunIntensityManual << endl;
    }

    // Mod total întuneric (oprește tot)
    if (k == 'x' || k == 'X') {
        chandelierEnabled = false;
        lightIntensity = 0.0f;
        sunEnabled = false;
        cout << "TOTAL DARKNESS MODE - All lights OFF" << endl;
    }

    // Resetează la normal
    if (k == 'r' || k == 'R') {
        chandelierEnabled = true;
        lightIntensity = 0.5f;
        sunEnabled = true;
        sunIntensityManual = 1.0f;
        cout << "RESET - All lights restored" << endl;
    }

    // Controlul automat al timpului (păstrat din codul original)
    if (k == 't' || k == 'T') {
        autonomicMode = !autonomicMode;
        cout << "Autonomic mode: " << (autonomicMode ? "ON" : "OFF") << endl;
        cout << "Time of day: " << timeOfDay << "h" << endl;
    }

    if (k == 'n' || k == 'N') {
        timeOfDay += 1.0f;
        if (timeOfDay >= 24.0f) timeOfDay = 0.0f;
        sunPosition = calculateSunPosition(timeOfDay);
        cout << "Time: " << (int)timeOfDay << ":00h" << endl;
    }

    if (k == 'b' || k == 'B') {
        timeOfDay -= 1.0f;
        if (timeOfDay < 0.0f) timeOfDay = 23.0f;
        sunPosition = calculateSunPosition(timeOfDay);
        cout << "Time: " << (int)timeOfDay << ":00h" << endl;
    }

    // Controale masă (păstrate din codul original)
    if (k == '5') {
        tableRotation += 5.0f;
        if (tableRotation >= 360.0f) tableRotation -= 360.0f;
        std::cout << "Table Rotation: " << tableRotation << " degrees" << std::endl;
    }
    if (k == '6') {
        tableRotation -= 5.0f;
        if (tableRotation < 0.0f) tableRotation += 360.0f;
        std::cout << "Table Rotation: " << tableRotation << " degrees" << std::endl;
    }
    if (k == '7') {
        tablePos.x += 0.1f;
        std::cout << "Table X: " << tablePos.x << std::endl;
    }
    if (k == '8') {
        tablePos.x -= 0.1f;
        std::cout << "Table X: " << tablePos.x << std::endl;
    }
    if (k == '9') {
        tablePos.z += 0.1f;
        std::cout << "Table Z: " << tablePos.z << std::endl;
    }
    if (k == '0') {
        tablePos.z -= 0.1f;
        std::cout << "Table Z: " << tablePos.z << std::endl;
    }

    // Afișează help
    if (k == 'h' || k == 'H') {
        cout << "\n=== LIGHT CONTROLS ===" << endl;
        cout << "C - Toggle Chandelier ON/OFF" << endl;
        cout << "+/- - Chandelier intensity (when ON)" << endl;
        cout << "P - Toggle Sun ON/OFF" << endl;
        cout << "U/J - Sun intensity UP/DOWN" << endl;
        cout << "X - Total darkness (all OFF)" << endl;
        cout << "R - Reset all lights" << endl;
        cout << "N/B - Change time of day" << endl;
        cout << "T - Toggle autonomic time mode" << endl;
        cout << "H - Show this help" << endl;
        cout << "======================" << endl;
    }
}

void keyUp(unsigned char k, int x, int y) { keys[k] = false; }

// Mouse
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

//Lumina Candelabru
void initLights() {
    float radius = 0.8f;
    float height = chandelierPos.y - 0.3f;

    for (int i = 0; i < numLights; i++) {
        float angle = (2.0f * M_PI * i) / numLights;
        lightPositions[i] = glm::vec3(chandelierPos.x + radius * cos(angle), height, chandelierPos.z + radius * sin(angle));
    }
}

void setLightingUniforms(GLuint program, const glm::vec3& viewPos) {
    glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(viewPos));

    // Trimite numărul de lumini doar dacă candelabrul e pornit
    int activeLights = chandelierEnabled ? numLights : 0;
    glUniform1i(glGetUniformLocation(program, "numLights"), activeLights);
    glUniform1f(glGetUniformLocation(program, "lightIntensity"), lightIntensity);

    // Trimite pozițiile luminilor doar dacă sunt active
    for (int i = 0; i < numLights; i++) {
        string uniformName = "lightPositions[" + to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, uniformName.c_str()), 1, glm::value_ptr(lightPositions[i]));
    }
    glUniform1f(glGetUniformLocation(program, "normalMapStrength"), 1.0f);

    // Adaugă uniform-urile pentru soare
    glUniform3fv(glGetUniformLocation(program, "sunPosition"), 1, glm::value_ptr(sunPosition));
    glUniform1f(glGetUniformLocation(program, "sunIntensity"), sunEnabled ? sunIntensityManual : 0.0f);
    glUniform1f(glGetUniformLocation(program, "timeOfDay"), timeOfDay);
}

void drawTable(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& viewPos) {
    glUseProgram(shaderProgram);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glm::mat4 tableModel = glm::translate(glm::mat4(1.0f), tablePos);
    tableModel = glm::rotate(tableModel, glm::radians(tableRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    tableModel = glm::scale(tableModel, tableScale);

    glm::mat4 tableMVP = projection * view * tableModel;
    glm::mat4 tableNormalMatrix = glm::transpose(glm::inverse(tableModel));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(tableMVP));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(tableModel));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(tableNormalMatrix));

    setLightingUniforms(shaderProgram, viewPos);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tableTex);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tableTex);

    glBindVertexArray(table.vao);
    glDrawElements(GL_TRIANGLES, table.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void display() {
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = now - lastFrame;
    lastFrame = now;
    doMovement();

    // Actualizare automată doar dacă e activă
    if (autonomicMode) {
        timeOfDay += deltaTime * 24.0f / dayDuration;
        if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;
        sunPosition = calculateSunPosition(timeOfDay);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::vec3 viewPos = cameraPos;

    // Randare candelabru (chiar dacă e oprit, obiectul rămâne vizibil)
    glm::mat4 chandModel = glm::translate(glm::mat4(1.0f), chandelierPos);
    chandModel = glm::scale(chandModel, glm::vec3(1.0f));

    glm::mat4 chandMVP = proj * view * chandModel;
    glm::mat4 chandNormalMatrix = glm::transpose(glm::inverse(chandModel));

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(chandMVP));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(chandModel));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(chandNormalMatrix));

    setLightingUniforms(shaderProgram, viewPos);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, chandelierTex);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, chandelierTex);

    glBindVertexArray(chandelier.vao);
    glDrawElements(GL_TRIANGLES, chandelier.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Calculează intensitatea soarelui cu noile controale
    float sunIntensity = calculateNaturalLightIntensity(timeOfDay);

    drawTable(proj, view, viewPos);
    drawRoom(proj, view, lightPositions, chandelierEnabled ? numLights : 0, viewPos, 1.0f, lightIntensity, sunPosition, sunIntensity, timeOfDay);
    drawWindows(proj, view, viewPos, timeOfDay, sunPosition, sunIntensity);

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Room - Light Control");

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
    table = loadOBJ("Objects/Table/table.obj");
    tableTex = loadTex("Objects/Table/table_diffuse.jpg");

    // Inițializează room cu toate texturile
    initRoom(wallDiffuse, wallNormal, floorDiffuse, floorNormal, ceilDiffuse, ceilNormal, shaderProgram);

    // Afișează controalele la început
    cout << "\n=== LIGHT CONTROLS ===" << endl;
    cout << "C - Toggle Chandelier ON/OFF" << endl;
    cout << "+/- - Chandelier intensity (when ON)" << endl;
    cout << "P - Toggle Sun ON/OFF" << endl;
    cout << "U/J - Sun intensity UP/DOWN" << endl;
    cout << "X - Total darkness (all OFF)" << endl;
    cout << "R - Reset all lights" << endl;
    cout << "N/B - Change time of day" << endl;
    cout << "T - Toggle autonomic time mode" << endl;
    cout << "H - Show this help" << endl;
    cout << "======================" << endl;

    glutMainLoop();
    return 0;
}