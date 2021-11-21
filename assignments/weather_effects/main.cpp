#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"
#include "primitives.h"

#include "Camera.h"

// structure to hold render info
// -----------------------------
struct SceneObject {
    unsigned int VAO;
    unsigned int vertexCount;
    void drawSceneObject() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};

struct ParticleSystem {
    static const int ATTR_SIZE = 3;

    std::shared_ptr<Shader> shader;
    unsigned int VAO{}; // self-explanatory
    unsigned int VBO{}; // self-explanatory
    unsigned int particleCount; // total amount of particles
    unsigned int particleId = 0; // id of last updated particle
    glm::vec3 offsets = glm::vec3(0);
    ParticleSystem(Shader* part_shader, unsigned int particleCount) : particleCount(particleCount), shader(part_shader) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // initialize particle buffer, set all values to 0
        std::vector<float> data(particleCount * ATTR_SIZE);
        for(unsigned int i = 0; i < data.size(); i++)
            data[i] = 0.0f;
        // allocate at openGL controlled memory
        glBufferData(GL_ARRAY_BUFFER, particleCount * ATTR_SIZE * 4, &data[0], GL_DYNAMIC_DRAW);
        GLuint vertexLocation = glGetAttribLocation(shader->ID, "pos");
        std::cout << shader->ID << std::endl;
        std::cout << vertexLocation << std::endl;
        glEnableVertexAttribArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, ATTR_SIZE * 4, nullptr);
    }

    void emitParticle(float x, float y, float z) {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        float pos[ATTR_SIZE];
        pos[0] = x;
        pos[1] = y;
        pos[2] = z;
        glBufferSubData(GL_ARRAY_BUFFER, particleId * ATTR_SIZE * 4, ATTR_SIZE * 4, pos);
        particleId = (particleId + 1) % particleCount;
    }
    void drawParticles() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, particleCount);
    }
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObjects(glm::mat4 viewProjection);
void drawWeather(glm::mat4 viewProjection);
void simulateWeather();
glm::mat4 getViewProjection();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
Camera* camera;
SceneObject cube;
SceneObject floorObj;
//! Scene object holding the weather particles
ParticleSystem* weather;
Shader* geometryShader;
Shader* particleShader;

// global variables used for control
// ---------------------------------
//! Random
std::random_device rd;
std::default_random_engine eng(rd());

//! Time
float currentTime, deltaTime = 0.f, lastFrame = 0.f;

//! Particle emission
glm::vec3 gravityOffset = glm::vec3(0), windOffset = glm::vec3(0);
float particleSize = 1.f, boxSize = 50.f, gravity = 10.f, windSpeed = 5.f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 5.2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    camera = new Camera(glm::vec3(.0f, 1.6f, 0.0f));

    // setup mesh objects
    // ---------------------------------------
    setup();

    // set up the z-buffer
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system);
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    // NEW!
    // enable built in variable gl_PointSize in the vertex shader
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(window);

        glClearColor(0.02f, 0.01f, 0.2f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        geometryShader->use();
        auto viewProjection = getViewProjection();
        drawObjects(viewProjection);
        drawWeather(viewProjection);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete geometryShader;
    delete particleShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

glm::mat4 getViewProjection() {
    glm::mat4 projection = glm::perspectiveFov(glm::radians(90.0f), (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = camera->getViewMatrix();
   return projection * view;
}

void drawObjects(glm::mat4 viewProjection){
    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);

    // NEW!
    // update the camera pose and projection, and compose the two into the viewProjection with a matrix multiplication
    // projection * view = world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order (projection * view), we could read
    // perspective_projection_from_view <- view_from_world


    // draw floor (the floor was built so that it does not need to be transformed)
    geometryShader->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different locations and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);
}

void simulateWeather() {
//    std::uniform_real_distribution<float> distr(-0.1f, 0.1f);
    gravityOffset -= glm::vec3(0, gravity * deltaTime, 0);
    windOffset += glm::vec3(sin(currentTime)/10, 0, cos(currentTime)/10);
    weather->offsets = gravityOffset + windOffset;
}

void drawWeather(glm::mat4 viewProjection) {
    weather->shader->use();
    simulateWeather();
    auto fwdOffset = camera->forward * boxSize / 2.f;
    weather->offsets -= camera->position + fwdOffset + boxSize / 2.f;
    weather->offsets = glm::mod(weather->offsets, boxSize);
    weather->shader->setMat4("viewMat", viewProjection);
    weather->shader->setVec3("cameraPos", camera->position);
    weather->shader->setVec3("forwardOffset", fwdOffset);
    weather->shader->setVec3("offsets", weather->offsets);
    weather->drawParticles();
}


void drawCube(glm::mat4 model){
    // draw object
    geometryShader->setMat4("model", model);
    cube.drawSceneObject();
}

void setup(){
    // initialize shaders
    geometryShader = new Shader("shaders/geometry.vert", "shaders/geometry.frag");
    particleShader = new Shader("shaders/particle.vert", "shaders/particle.frag");

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    std::uniform_real_distribution<float> distr(-boxSize/2, boxSize/2);

    weather = new ParticleSystem(particleShader, 10000);
//    weather->shader->setFloat("boxSize", 50.f);
    for(int i = 0; i < 10000; i++) {
        // create 5 to 10 particles per frame
        weather->emitParticle(distr(eng), distr(eng), distr(eng));
    }
}


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(geometryShader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(geometryShader->ID, "color");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // creates and bind the EBO
    createElementArrayBuffer(indices);

    return VAO;
}


unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

    return VBO;
}


unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

    return EBO;
}

// NEW!
// instead of using the NDC to transform from screen space you can now define the range using the
// min and max parameters
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y){
    float sum = max - min;
    float xInRange = (float) screenX / (float) screenW * sum - sum/2.0f;
    float yInRange = (float) screenY / (float) screenH * sum - sum/2.0f;
    x = xInRange;
    y = -yInRange; // flip screen space y axis
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    static auto lastCoord = glm::vec2(posX, posY);
    auto offset = glm::vec2(posX - lastCoord.x, (posY - lastCoord.y) * -1.f);
    lastCoord = glm::vec2(posX, posY);
    // calc camera direction
    camera->processMouseInput(offset.x, offset.y);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera->processKeyInput(Direction::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera->processKeyInput(Direction::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera->processKeyInput(Direction::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera->processKeyInput(Direction::RIGHT, deltaTime);

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}