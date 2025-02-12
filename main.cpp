#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/string_cast.hpp>
#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>

gps::Window myWindow;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

gps::Camera myCamera(
    glm::vec3(-92.25f, 14.05f, 29.97f), 
    glm::vec3(0.0f, 0.0f, 0.0f),  
    glm::vec3(0.0f, 1.0f, 0.0f)   
);

GLfloat cameraSpeed = 1.0f;

GLboolean pressedKeys[1024];

// models
gps::Model3D scenaFinala;
gps::Model3D doarMorisca;

GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader depthShader;


GLboolean mouseControlEnabled = GL_TRUE;

static float yaw = -90.0f;
static float pitch = 0.0f;

glm::vec3 wheelPivotPoint(54.69f, 19.73f, -55.22f);
GLfloat wheelRotationAngle = 0.0f; 

glm::mat4 sceneModelMatrix = glm::mat4(1.0f);

const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;

GLuint shadowMapFBO;
GLuint depthMapTexture;


// Point light properties
glm::vec3 pointLightPos(-30.35f, 16.25f, 64.9f);
glm::vec3 pointLightColor(1.0f, 1.0f, 0.8f);    

// Attenuation factors
float pointLightConstant = 1.0f;  
float pointLightLinear = 0.09f;   
float pointLightQuadratic = 0.032f; 

static float lightAngle = 0.0f; 
float lightRadius = 30.0f;
glm::vec3 lightPos; 

float fogStart = 100.0f;
float fogEnd = 700.0f;

enum RenderMode {
    SOLID,
    WIREFRAME,
    POLYGONAL,
    SMOOTH
};

RenderMode currentRenderMode = SOLID;

struct AnimationPhase {
    double startTime;         
    double duration;          
    std::function<void()> action;
};

std::vector<AnimationPhase> animationPhases;
double totalAnimationTime = 0.0;
GLboolean isAnimationActive = GL_TRUE;
double animationStartTime = 0.0;      


void setupAnimationPhases() {
    animationPhases.push_back({
        totalAnimationTime, 4.0, []() {
            myCamera.move(gps::MOVE_FORWARD, 0.7f); 
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
        });
    totalAnimationTime += 4.0;

    animationPhases.push_back({
        totalAnimationTime, 18.0, []() {
            yaw += 0.7f;
            myCamera.rotate(pitch, yaw);
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
        });
    totalAnimationTime += 18.0;

    animationPhases.push_back({
        totalAnimationTime, 5.0, []() {
            myCamera.move(gps::MOVE_UP, 0.1f);
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
        });
    totalAnimationTime += 5.0;

    animationPhases.push_back({
        totalAnimationTime, 5.0, []() {
            myCamera = gps::Camera(
                glm::vec3(-92.25f, 12.30f, 29.97f), 
                glm::vec3(0.0f, 0.0f, 0.0f),       
                glm::vec3(0.0f, 1.0f, 0.0f)        
            );
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
        });
    totalAnimationTime += 5.0;
}


void playAnimation(double elapsedTime) {
    for (const auto& phase : animationPhases) {
        if (elapsedTime >= phase.startTime && elapsedTime < (phase.startTime + phase.duration)) {
            phase.action();
            break;
        }
    }
}


void setRenderMode(RenderMode mode) {
    switch (mode) {
    case SOLID:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glShadeModel(GL_FLAT);
        break;

    case WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;

    case POLYGONAL:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;

    case SMOOTH:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glShadeModel(GL_SMOOTH);
        break;
    }
}



GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        mouseControlEnabled = !mouseControlEnabled;
        if (mouseControlEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        currentRenderMode = SOLID;
        setRenderMode(currentRenderMode);
        std::cout << "Render Mode: Solid" << std::endl;
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        currentRenderMode = WIREFRAME;
        setRenderMode(currentRenderMode);
        std::cout << "Render Mode: Wireframe" << std::endl;
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        currentRenderMode = POLYGONAL;
        setRenderMode(currentRenderMode);
        std::cout << "Render Mode: Polygonal" << std::endl;
    }

    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        currentRenderMode = SMOOTH;
        setRenderMode(currentRenderMode);
        std::cout << "Render Mode: Smooth" << std::endl;
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS) { 
        isAnimationActive = GL_TRUE;                
        animationStartTime = glfwGetTime();
        std::cout << "Animation Restarted" << std::endl;
        myCamera = gps::Camera(
            glm::vec3(-92.25f, 12.30f, 29.97f), 
            glm::vec3(0.0f, 0.0f, 0.0f),       
            glm::vec3(0.0f, 1.0f, 0.0f)        
        );
        yaw = -90.0f;
        pitch = 0.0f;
    }

    if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
        fogEnd += 50.0f;
    }

    if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
        fogEnd -= 50.0f; 
        if (fogEnd <= fogStart) fogEnd = fogStart + 1.0f;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = 1024.0f / 2.0f;
    static float lastY = 768.0f / 2.0f;
    static bool firstMouse = true;

    if (!mouseControlEnabled) {
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float mouseSensitivity = 0.1f;
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f)
        pitch = 89.0f;

    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}


void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_UP]) { 
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_Q]) {
        yaw -= 1.0f; 
        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (pressedKeys[GLFW_KEY_E]) {
        yaw += 1.0f; 
        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
        if (lightAngle < 0.0f) {
            lightAngle += 360.0f;
        }
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
        if (lightAngle > 360.0f) {
            lightAngle -= 360.0f;
        }
    }

    float lx = lightRadius * cos(glm::radians(lightAngle));
    float lz = lightRadius * sin(glm::radians(lightAngle));
    lightPos = glm::vec3(lx, 10.0f, lz); 
}


void initOpenGLWindow() {
    myWindow.Create(1024, 768, "Valhalla in the Snow");
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);

    glCullFace(GL_BACK); 
    glFrontFace(GL_CCW);

    setRenderMode(currentRenderMode);
}

void initModels() {
    scenaFinala.LoadModel("models/scenaFinala/finalScene.obj");
    doarMorisca.LoadModel("models/doarMorisca/scenaMorisca.obj");
}

void initShaders() {
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");

    depthShader.loadShader(
        "shaders/depthShader.vert",
        "shaders/depthShader.frag");
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    GLint loc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix");
    if (loc == -1) {
        std::cout << "Uniform 'lightSpaceTrMatrix' not found in myCustomShader!" << std::endl;
    }

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}


void renderScenaFinala(gps::Shader shader) {
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(sceneModelMatrix));

    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    scenaFinala.Draw(shader);
}


glm::mat4 wheelModelMatrix;

void renderDoarMorisca(gps::Shader shader) {
    shader.useShaderProgram();

    wheelModelMatrix = glm::mat4(1.0f);
    wheelModelMatrix = glm::translate(wheelModelMatrix, wheelPivotPoint);
    wheelModelMatrix = glm::rotate(wheelModelMatrix, glm::radians(wheelRotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    wheelModelMatrix = glm::translate(wheelModelMatrix, -wheelPivotPoint);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheelModelMatrix));
    doarMorisca.Draw(shader);
}


glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
    return lightProjection * lightView;
}

void renderScenaDepth(gps::Shader& depthShader, const glm::mat4& modelMat) {
    depthShader.useShaderProgram();

    GLint modelLocDepth = glGetUniformLocation(depthShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLocDepth, 1, GL_FALSE, glm::value_ptr(modelMat));

    scenaFinala.Draw(depthShader);
}

void renderDoarMoriscaDepth(gps::Shader& depthShader, const glm::mat4& modelMat) {
    depthShader.useShaderProgram();

    GLint modelLocDepth = glGetUniformLocation(depthShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLocDepth, 1, GL_FALSE, glm::value_ptr(modelMat));

    doarMorisca.Draw(depthShader);
}

void renderScenaLit(gps::Shader& lightingShader,
    const glm::mat4& modelMat,
    const glm::mat3& normalMat) {
    lightingShader.useShaderProgram();

    GLint modelLocMain = glGetUniformLocation(lightingShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLocMain, 1, GL_FALSE, glm::value_ptr(modelMat));

    GLint normalMatrixLocMain = glGetUniformLocation(lightingShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLocMain, 1, GL_FALSE, glm::value_ptr(normalMat));

    scenaFinala.Draw(lightingShader);
}

void renderDoarMoriscaLit(gps::Shader& lightingShader,
    const glm::mat4& modelMat,
    const glm::mat3& normalMat) {
    lightingShader.useShaderProgram();

    GLint modelLocMain = glGetUniformLocation(lightingShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLocMain, 1, GL_FALSE, glm::value_ptr(modelMat));

    GLint normalMatrixLocMain = glGetUniformLocation(lightingShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLocMain, 1, GL_FALSE, glm::value_ptr(normalMat));

    doarMorisca.Draw(lightingShader);
}

void renderShadowMap() {
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.useShaderProgram();

    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    GLint lightSpaceLoc = glGetUniformLocation(depthShader.shaderProgram, "lightSpaceTrMatrix");
    glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glm::mat4 sceneModel = glm::mat4(1.0f);
    renderScenaDepth(depthShader, sceneModel);

    glm::mat4 wheelModel = glm::mat4(1.0f);
    wheelModel = glm::translate(wheelModel, wheelPivotPoint);
    wheelModel = glm::rotate(wheelModel, glm::radians(wheelRotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    wheelModel = glm::translate(wheelModel, -wheelPivotPoint);

    renderDoarMoriscaDepth(depthShader, wheelModel);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderFinalScene() {
    myBasicShader.useShaderProgram();

    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    GLint lightSpaceLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix");
    glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    GLint shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
    glUniform1i(shadowMapLoc, 3);

    GLint lightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightPos");
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

    GLint pointLightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos");
    GLint pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
    GLint pointLightConstantLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightConstant");
    GLint pointLightLinearLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightLinear");
    GLint pointLightQuadraticLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightQuadratic");

    glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(pointLightPos));
    glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
    glUniform1f(pointLightConstantLoc, pointLightConstant);
    glUniform1f(pointLightLinearLoc, pointLightLinear);
    glUniform1f(pointLightQuadraticLoc, pointLightQuadratic);

    glm::vec3 fogColor(0.7f, 0.7f, 0.7f); // Grey fog color            

    GLint fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));

    GLint fogStartLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogStart");
    glUniform1f(fogStartLoc, fogStart);

    GLint fogEndLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnd");
    glUniform1f(fogEndLoc, fogEnd);

    glm::mat4 sceneModel = glm::mat4(1.0f);
    glm::mat3 sceneNormalMat = glm::mat3(glm::inverseTranspose(view * sceneModel));
    renderScenaLit(myBasicShader, sceneModel, sceneNormalMat);

    glm::mat4 wheelModel = glm::mat4(1.0f);
    wheelModel = glm::translate(wheelModel, wheelPivotPoint);
    wheelModel = glm::rotate(wheelModel, glm::radians(wheelRotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    wheelModel = glm::translate(wheelModel, -wheelPivotPoint);

    glm::mat3 wheelNormalMat = glm::mat3(glm::inverseTranspose(view * wheelModel));
    renderDoarMoriscaLit(myBasicShader, wheelModel, wheelNormalMat);
}


void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderShadowMap();
    renderFinalScene();
}

void cleanup() {
    myWindow.Delete();
}


void initShadowMapping() {
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLfloat wheelRotationSpeed = 30.0f;

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();

    initShadowMapping();

    setWindowCallbacks();

    setupAnimationPhases();
    std::cout << "Total Animation Time: " << totalAnimationTime << " seconds" << std::endl;

    animationStartTime = glfwGetTime();

    glCheckError();

    static double lastFrameTime = 0.0;
    double currentFrameTime = 0.0;

    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - animationStartTime;

        currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        wheelRotationAngle += wheelRotationSpeed * deltaTime;
        if (wheelRotationAngle > 360.0f) {
            wheelRotationAngle -= 360.0f;
        }

        if (isAnimationActive) {
            if (elapsedTime >= totalAnimationTime) {
                isAnimationActive = GL_FALSE;
            }
            else {
                playAnimation(elapsedTime);
            }
        }

        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}
