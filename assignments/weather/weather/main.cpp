#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"

#include "plane_model.h"
#include "primitives.h"

#include <random>

// structure to hold render info
// -----------------------------
struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
    void drawSceneObject() const{
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObjects();

// particles: Particle related functions
void bindParticleAttributes();
void createParticleVertexBufferObject();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);
void drawPlane(glm::mat4 model);
void drawParticles(glm::mat4 viewProjection, int windOffset, int gravityOffset); //particles: d

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject floorObj;
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;
Shader* shaderProgram;

Shader* shaderProgramParticles;
std::vector<Shader> shaderParticlePrograms;
Shader* activeParticleShader;
int activeParticleShaderID = 0;

// global variables used for control
// ---------------------------------
float currentTime;
glm::vec3 camForward(.0f, .0f, -1.0f);
glm::vec3 camPosition(.0f, 1.6f, 0.0f);
float linearSpeed = 0.15f, rotationGain = 30.0f;

float lastX = 400, lastY = 300;
float yaw, pitch;

/* particles: global variables used for particles */
unsigned int VAOParticles, VBOParticles;                          // vertex array and buffer objects
const unsigned int particleCount = 20000;    // # of particles * 2 (top and bottom of raindrop)
const unsigned int particleSize = 3;            // particle attributes: pos(x,y,z) => 3
const unsigned int sizeOfFloat = 4;             // bytes in a float
unsigned int particleId = 0;                    // keep track of last particle to be updated
const int boxSize = 30; // The size of the precipitation box.
glm::mat4 prevViewProjection; // The previous viewProjection (aka previous model -- used for line drawing)


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

    // setup mesh objects and particles
    // ---------------------------------------
    setup();

    // enable alpha blending (used for "fake" motion blur)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // set up the z-buffer
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system); d
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

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
        currentTime = appTime.count(); //d

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw all objects and particles
        drawObjects();


        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete shaderProgram;

    //particles: deleting the shaderProgramParticles
    //delete activeParticleShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void drawObjects(){

    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);

    // NEW!
    // update the camera pose and projection, and compose the two into the viewProjection with a matrix multiplication
    // projection * view = world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order (projection * view), we could read
    // perspective_projection_from_view <- view_from_world
    glm::mat4 projection = glm::perspectiveFov(70.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    glm::mat4 viewProjection = projection * view;

    // draw floor (the floor was built so that it does not need to be transformed)
    shaderProgram->use();
    shaderProgram->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different locations and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);

    drawPlane(viewProjection * glm::translate(-2.0f, .5f, 2.0f) * glm::rotateX(glm::quarter_pi<float>()) * scale);
    drawPlane(viewProjection * glm::translate(2.0f, .5f, -2.0f) * glm::rotateX(glm::quarter_pi<float>() * 3.f) * scale);


    // particles: This part draws the particles. Use view/projection to convert to world space
    activeParticleShader->use(); // Important: Use the particle shader program before proceeding to draw particles

    // Setup random generators for wind and gravity offsets
    std::random_device rd;  //Will be used to obtain a seed for the random number engine dd
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> distrib_wind(1, 3);
    std::uniform_real_distribution<> distrib_gravity(7, 9);

    // for-loop that calls draw particles multiple times with different offsets -> more complexity
    for(int i = 0; i<20; i++) { // Running 20 simulation instances
        int windOffset = distrib_wind(gen);
        int gravityOffset = distrib_gravity(gen);
        drawParticles(viewProjection, windOffset, gravityOffset);
    }
    //drawParticles(viewProjection,2);

    // Save a global reference to the previous viewProjection @ each frame
    prevViewProjection = viewProjection;
}


void drawCube(glm::mat4 model){
    // draw object
    shaderProgram->setMat4("model", model);
    cube.drawSceneObject();
}


void drawPlane(glm::mat4 model){

    // draw plane body and right wing
    shaderProgram->setMat4("model", model);
    planeBody.drawSceneObject();
    planeWing.drawSceneObject();

    // propeller,
    glm::mat4 propeller = model * glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgram->setMat4("model", propeller);
    planePropeller.drawSceneObject();

    // right wing back,
    glm::mat4 wingRightBack = model * glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingRightBack);
    planeWing.drawSceneObject();

    // left wing,
    glm::mat4 wingLeft = model * glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgram->setMat4("model", wingLeft);
    planeWing.drawSceneObject();

    // left wing back,
    glm::mat4 wingLeftBack =  model *  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingLeftBack);
    planeWing.drawSceneObject();
}

//particles:
// Note: model and viewProjection are interchangeable here
void drawParticles(glm::mat4 viewProjection, int windOffset, int gravityOffset) {
    glm::vec3 offsets;
    if(activeParticleShaderID==0){  // rain
        offsets = glm::vec3(-currentTime * windOffset, -currentTime * gravityOffset, -currentTime * windOffset);
    } else {                        // snow (gravity offset reduced to 20 %)
        offsets = glm::vec3(-currentTime * windOffset, -currentTime * gravityOffset*0.2, -currentTime * windOffset);
    }
    offsets -= camPosition + camForward + glm::vec3(boxSize/2);
    offsets = glm::mod(offsets, glm::vec3(boxSize));

    // Create an instance velocity vector from the wind and gravity offsets.
    glm::vec3 instanceVelocity = glm::vec3(windOffset,gravityOffset,windOffset);

    // Update all the general UNIFORMS in the particle shader program
    activeParticleShader->setMat4("viewProjection", viewProjection);
    activeParticleShader->setMat4("prevViewProjection", prevViewProjection);
    activeParticleShader->setVec3("offsets", offsets);
    activeParticleShader->setFloat("boxSize", boxSize);
    activeParticleShader->setVec3("camPosition", camPosition);
    activeParticleShader->setVec3("camForward", camForward);
    activeParticleShader->setVec3("instanceVelocity",instanceVelocity);

    //bindParticleAttributes(); // BONUS: Uncomment this line to create prize winning abstract art

    // Let OpenGL Render the particles
    glBindVertexArray(VAOParticles);
    glDrawArrays(GL_LINES, 0, particleCount);
}


void setup(){
    // initialize shaders
    shaderProgram = new Shader("shaders/shader.vert", "shaders/shader.frag");
    // particles: initialize particle shaders within shaderProgram
    //shaderProgramParticles = new Shader("shaders/snowShader.vert", "shaders/snowShader.frag");
    shaderParticlePrograms.push_back(Shader("shaders/rainShader.vert", "shaders/rainShader.frag"));
    shaderParticlePrograms.push_back(Shader("shaders/snowShader.vert", "shaders/snowShader.frag"));
    activeParticleShader = &shaderParticlePrograms[0];

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // load plane meshes into openGL
    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    planePropeller.VAO = createVertexArray(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();

    // particles: Initialize particle VBO
    createParticleVertexBufferObject();
}


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(shaderProgram->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(shaderProgram->ID, "color");
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
    y = -yInRange; // flip screen space y axis rr ff d
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    // TODO - rotate the camera position based on mouse movements
    //  if you decide to use the lookAt function, make sure that the up vector and the
    //  vector from the camera position to the lookAt target are not collinear
    float xoffset = posX - lastX;
    float yoffset = lastY - posY; // reversed since y-coordinates range from bottom to top
    lastX = posX;
    lastY = posY;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camForward = glm::normalize(direction);

}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // TODO move the camera position based on keys pressed (use either WASD or the arrow keys)
    glm::vec3 camUp = glm::vec3(0.0f,1.0f,0.0f);
    glm::vec3 forwardInXZ = glm::normalize(glm::vec3(camForward.x, 0, camForward.z)); // dTHIS IS WHAT ENSURES WE CANT FLY (ONLY XZ PLANE)

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPosition += linearSpeed * forwardInXZ;
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPosition -= linearSpeed * forwardInXZ;
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPosition -= glm::normalize(glm::cross(forwardInXZ,camUp)) * linearSpeed;
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPosition += glm::normalize(glm::cross(forwardInXZ,camUp)) * linearSpeed;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        activeParticleShader = &shaderParticlePrograms[0];
        activeParticleShaderID = 0;
        std::cout << "Rain is being rendered. \n";
    } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        activeParticleShader= &shaderParticlePrograms[1];
        activeParticleShaderID = 1;
        std::cout << "Snow is being rendered. \n";
    }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

/* ===== particles: new added particle functions ======= */
void createParticleVertexBufferObject(){
    // This function is only for initialization, i.e. called ONCE

    // Generate buffers
    glGenVertexArrays(1, &VAOParticles);
    glGenBuffers(1, &VBOParticles);

    // Bind buffers
    glBindVertexArray(VAOParticles);
    glBindBuffer(GL_ARRAY_BUFFER, VBOParticles);

    // Setup random generator for initial positions
    std::random_device rd;  //Will be used to obtain a seed for the random number engine d
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> distrib(boxSize, 2*boxSize);

    // Initialize particle buffer
    // Each particle is 3 large (x,y,z) for 20.000 particles (10.000 different, each have top/bottom)
    std::vector<float> data(particleCount * particleSize );
    for(unsigned int i = 0; i < data.size(); i=i+particleSize*2){ // Increment step = 2*particleSize (handle top/bottom)
        // Note: top and bottom of raindrop is initialized to *same* random position
        float x = distrib(gen);
        float y = distrib(gen);
        float z = distrib(gen);
        // Top of raindrop
        data[i] = x;
        data[i+1] = y;
        data[i+2] = z;
        // Bottom of raindrop
        data[i+3] = x;
        data[i+4] = y;
        data[i+5] = z;
    }


    // allocate at openGL controlled memory
    glBufferData(GL_ARRAY_BUFFER, particleCount * particleSize * sizeOfFloat, &data[0], GL_DYNAMIC_DRAW);
    bindParticleAttributes();
}

void bindParticleAttributes(){
    int posSize = 3; // each position has x,y,z

    GLuint vertexLocation = glGetAttribLocation(activeParticleShader->ID, "pos");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE, particleSize * sizeOfFloat, 0);
}

