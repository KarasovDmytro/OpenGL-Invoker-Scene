#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/shader_m.h"
#include "../include/camera.h"
#include "../include/mesh.h"
#include "../include/model.h"
#include "../include/sphere_gen.h"

#include <iostream>
#include <vector>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float orbitRadius = 1.25f;

bool meteorActive = false;
glm::vec3 meteorPos(0.0f);
glm::vec3 meteorDirection(0.0f);
float meteorSpeed = 15.0f;
float meteorRotation = 0.0f;

bool isGhostWalk = false;
bool ghostKeyPressed = false;

glm::vec3 lightColors[] = {
    glm::vec3(0.2f, 0.2f, 1.0f), // Quas - синій
    glm::vec3(1.0f, 0.2f, 1.0f), // Wex - рожевий
    glm::vec3(1.0f, 0.5f, 0.0f)  // Exort - помаранчевий
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(std::vector<std::string> faces);

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

int main() {

    glfwInit();
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Invoker", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Компіляція шейдерів
    Shader ourShader("assets/shaders/shader.vs", "assets/shaders/shader.fs");
    Shader lampShader("assets/shaders/lamp.vs", "assets/shaders/lamp.fs");
    Shader skyboxShader("assets/shaders/skybox.vs", "assets/shaders/skybox.fs");

    // Лоад моделей
    stbi_set_flip_vertically_on_load(false);
    Model ourModel("assets/models/Invoker/dota_2_invoker_kid.obj");
    std::cout << "Invoker loaded!" << std::endl;
    Model meteorModel("assets/models/Meteor/meteor.obj");

    Mesh sphereMesh = SphereGenerator::createSphere(1.0f);

    // Кубемапа
    float skyboxVertices[] = {
        -1.0f, -1.0f,  1.0f, // 0
         1.0f, -1.0f,  1.0f, // 1
         1.0f,  1.0f,  1.0f, // 2
        -1.0f,  1.0f,  1.0f, // 3
        -1.0f, -1.0f, -1.0f, // 4
         1.0f, -1.0f, -1.0f, // 5
         1.0f,  1.0f, -1.0f, // 6
        -1.0f,  1.0f, -1.0f  // 7
    };

    unsigned int skyboxIndices[] = {
        1, 2, 6,
        6, 5, 1,

        0, 4, 7,
        7, 3, 0,

        3, 7, 6,
        6, 2, 3,

        0, 1, 5,
        5, 4, 0,

        4, 5, 6,
        6, 7, 4,

        0, 3, 2,
        2, 1, 0
    };

    unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    std::vector<std::string> faces{
        "assets/textures/skybox/space_rt.png",
        "assets/textures/skybox/space_lf.png",
        "assets/textures/skybox/space_up.png",
        "assets/textures/skybox/space_dn.png",
        "assets/textures/skybox/space_bk.png",
        "assets/textures/skybox/space_ft.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    ourShader.use();
    ourShader.setInt("material.diffuse", 0);
    ourShader.setInt("material.specular", 1);
    ourShader.setFloat("material.shininess", 32.0f);

    for (int i = 0; i < 3; i++) {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        ourShader.setFloat(base + ".constant", 1.0f);
        ourShader.setFloat(base + ".linear", 0.09f);
        ourShader.setFloat(base + ".quadratic", 0.032f);
    }

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    Mesh moonMesh = SphereGenerator::createSphere(1.0f, 64, 64);

    unsigned int moonTexture = loadTexture("assets/textures/moon.jpg");

    Texture moonTexStruct;
    moonTexStruct.id = moonTexture;
    moonTexStruct.type = "texture_diffuse";
    moonMesh.textures.push_back(moonTexStruct);

    unsigned int quasTex = loadTexture("assets/textures/quas.jpg");
    unsigned int wexTex = loadTexture("assets/textures/wex.jpg");
    unsigned int exortTex = loadTexture("assets/textures/exort.jpg");

    unsigned int orbTextures[] = { quasTex, wexTex, exortTex };

    lampShader.use();
    lampShader.setInt("lightTexture", 0);



    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glStencilMask(0xFF);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // 1 - Намалювати кубемапу
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 viewSky = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("view", viewSky);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // 2 - Намалювати луну
        ourShader.use();

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 modelMoon = glm::mat4(1.0f);
        modelMoon = glm::translate(modelMoon, glm::vec3(0.0f, -6.0f, 0.0f));

        modelMoon = glm::scale(modelMoon, glm::vec3(6.0f));

        modelMoon = glm::rotate(modelMoon, (float)glfwGetTime() * 0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

        ourShader.setMat4("model", modelMoon);

        moonMesh.Draw(ourShader);

        // 3 - Порахувати локації сфер
        glm::vec3 lightPositions[3];
        float a1 = (float)glfwGetTime() * 2.0f;
        lightPositions[0] = glm::vec3(sin(a1) * orbitRadius, 1.3f, cos(a1) * orbitRadius);
        float a2 = (float)glfwGetTime() * 2.0f + 2.09f;
        lightPositions[1] = glm::vec3(sin(a2) * orbitRadius, 1.3f, cos(a2) * orbitRadius);
        float a3 = (float)glfwGetTime() * 2.0f + 4.18f;
        lightPositions[2] = glm::vec3(sin(a3) * orbitRadius, 1.3f, cos(a3) * orbitRadius);

        // 4 - Намалювати сфери
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);

        for (int i = 0; i < 3; i++) {
            glm::mat4 modelOrb = glm::mat4(1.0f);
            modelOrb = glm::translate(modelOrb, lightPositions[i]);
            modelOrb = glm::scale(modelOrb, glm::vec3(0.15f));

            lampShader.setMat4("model", modelOrb);
            lampShader.setVec3("lightColor", lightColors[i]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, orbTextures[i]);

            sphereMesh.Draw(lampShader);
        }

        // 5 - Намалювати Chaos Meteor
        if (meteorActive) {
            meteorPos += meteorDirection * meteorSpeed * deltaTime;
            meteorRotation += 5.0f * deltaTime;

            if (meteorPos.y < -10.0f) {
                meteorActive = false;
            }

            lampShader.use();

            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);

            glm::mat4 modelMeteor = glm::mat4(1.0f);

            modelMeteor = glm::translate(modelMeteor, meteorPos);

            modelMeteor = glm::rotate(modelMeteor, meteorRotation, glm::vec3(1.0f, 0.5f, 0.0f));

            modelMeteor = glm::scale(modelMeteor, glm::vec3(0.03f));

            lampShader.setMat4("model", modelMeteor);

            lampShader.setVec3("lightColor", glm::vec3(1.5f, 0.5f, 0.0f));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, orbTextures[2]); // [2] - це Exort
            lampShader.setInt("lightTexture", 0);

            meteorModel.Draw(lampShader);
        }

        // 6 - Намалювати Інвокера

        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        ourShader.use();
        ourShader.setVec3("viewPos", camera.Position);
        for (int i = 0; i < 3; i++) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            ourShader.setVec3(base + ".position", lightPositions[i]);
            ourShader.setVec3(base + ".ambient", lightColors[i] * 0.1f);
            ourShader.setVec3(base + ".diffuse", lightColors[i]);
            ourShader.setVec3(base + ".specular", lightColors[i]);
        }
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        float levitationHeight = sin(glfwGetTime() * 1.5f) * 0.2f + 0.5f;

        model = glm::translate(model, glm::vec3(0.0f, levitationHeight, 0.0f));

        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);

        if (isGhostWalk) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            ourShader.setFloat("alpha", 0.2f);
        }
        else {
            ourShader.setFloat("alpha", 1.0f);
            glDisable(GL_BLEND);
        }

        ourModel.Draw(ourShader);

        glDisable(GL_BLEND);

        if (isGhostWalk) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // 7 - Намалювати обводку

        if (!isGhostWalk) {
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);

            lampShader.use();
            lampShader.setVec3("lightColor", glm::vec3(1.0f, 0.8f, 0.0f)); // Золотий
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);

            glm::mat4 modelOutline = glm::mat4(1.0f);
            modelOutline = glm::translate(modelOutline, glm::vec3(0.0f, levitationHeight, 0.0f)); // <-- ТЕЖ ЛЕВІТУЄ
            modelOutline = glm::scale(modelOutline, glm::vec3(1.02f));
            lampShader.setMat4("model", modelOutline);

            ourModel.Draw(lampShader);
        }

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentSpeed = 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.MovementSpeed = 10.0f;
    }
    else {
        camera.MovementSpeed = 2.5f;
    }

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        orbitRadius += 2.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        orbitRadius -= 2.0f * deltaTime;

    if (orbitRadius < 1.0f) orbitRadius = 1.0f;
    if (orbitRadius > 15.0f) orbitRadius = 15.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !meteorActive) {
        meteorActive = true;

        meteorPos = glm::vec3(0.0f, 15.0f, 0.0f);

        meteorDirection = glm::normalize(glm::vec3(0.0f, -0.8f, 0.5f));

        meteorRotation = 0.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !ghostKeyPressed) {
        isGhostWalk = !isGhostWalk;
        ghostKeyPressed = true;

        if (isGhostWalk) {
            lightColors[0] = glm::vec3(0.8f, 0.0f, 1.0f);
            lightColors[1] = glm::vec3(0.8f, 0.0f, 1.0f);
            lightColors[2] = glm::vec3(0.8f, 0.0f, 1.0f);
        }
        else {
            lightColors[0] = glm::vec3(0.2f, 0.2f, 1.0f);
            lightColors[1] = glm::vec3(1.0f, 0.2f, 1.0f);
            lightColors[2] = glm::vec3(1.0f, 0.5f, 0.0f);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
        ghostKeyPressed = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}