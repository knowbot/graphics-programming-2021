#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <shader.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <random>

// structure to hold the info necessary to render an object
struct SceneObject {
    unsigned int VAO;           // vertex array object handle
    unsigned int vertexCount;   // number of vertices in the object
    float r, g, b;              // for object color
    float x, y;                 // for position offset
};

// declaration of the function you will implement in voronoi 1.1
SceneObject instantiateCone(float r, float g, float b, float offsetX, float offsetY);
// mouse, keyboard and screen reshape glfw callbacks
void button_input_callback(GLFWwindow* window, int button, int action, int mods);
void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// settings
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables we will use to store our objects, shaders, and active shader
std::vector<SceneObject> sceneObjects;
std::vector<Shader> shaderPrograms;
Shader* activeShader;


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment - Voronoi Diagram", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // setup frame buffer size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // setup input callbacks
    glfwSetMouseButtonCallback(window, button_input_callback); // NEW!
    glfwSetKeyCallback(window, key_input_callback); // NEW!

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // NEW!
    // build and compile the shader programs
    shaderPrograms.push_back(Shader("shaders/shader.vert", "shaders/color.frag"));
    shaderPrograms.push_back(Shader("shaders/shader.vert", "shaders/distance.frag"));
    shaderPrograms.push_back(Shader("shaders/shader.vert", "shaders/distance_color.frag"));
    activeShader = &shaderPrograms[0];

    // NEW!
    // set up the z-buffer
    glDepthRange(1,-1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // background color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // notice that now we are clearing two buffers, the color and the z-buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render the cones
        activeShader->use();
        // TODO voronoi 1.3
        for(auto & sceneObject : sceneObjects) {
            activeShader->setVec2("offset", sceneObject.x, sceneObject.y);
            activeShader->setVec3("color", sceneObject.r, sceneObject.g, sceneObject.b);
            glBindVertexArray(sceneObject.VAO);
            glDrawElements(GL_TRIANGLES, sceneObject.vertexCount, GL_UNSIGNED_INT, 0);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}


// creates a cone triangle mesh, uploads it to openGL and returns the VAO associated to the mesh
SceneObject instantiateCone(float r, float g, float b, float offsetX, float offsetY){
    // TODO voronoi 1.1
    // (exercises 1.7 and 1.8 can help you with implementing this function)

    // Create an instance of a SceneObject,
    SceneObject sceneObject{};
    // you will need to store offsetX, offsetY, r, g and b in the object.
    sceneObject.r = r;
    sceneObject.g = g;
    sceneObject.b = b;
    sceneObject.x = offsetX;
    sceneObject.y = offsetY;
    // Build the geometry into an std::vector<float> or float array.
    const double PI = 3.14159265358979323846;
    unsigned int trisCount = 30;
    float angle = (2 * (float)PI) / (float)trisCount;
    auto radius = (float) 2 * sqrt(2.f);
    float a = 0;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    // add center
    vertices.insert(vertices.end(), {0.0f, 0.0f, 1.0f});
    for(unsigned int i = 1; i <= trisCount; i++) {
        vertices.insert(vertices.end(), {cos(a) * radius, sin(a) * radius, -1.0f});
        auto next = (i == trisCount ? 1 : i + 1);
        indices.insert(indices.end(), {i, 0, next});
        a = (float)i * angle;
    }

    // Store the number of vertices in the mesh in the scene object.
    sceneObject.vertexCount = indices.size();
    // Declare and generate a VAO and VBO (and an EBO if you decide the work with indices).
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    // Bind and set the VAO and VBO (and optionally a EBO) in the correct order.
    // bind vertex array object

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLint), &indices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Set the position attribute pointers in the shader.
    int posSize = 3;
    int posAttributeLocation = glGetAttribLocation(activeShader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, posSize, GL_FLOAT, GL_FALSE, 0, nullptr);
    // Store the VAO handle in the scene object.
    sceneObject.VAO = VAO;

    glBindVertexArray(0);
    // 'return' the scene object for the cone instance you just created.
    return sceneObject;
}

// glfw: called whenever a mouse button is pressed
void button_input_callback(GLFWwindow* window, int button, int action, int mods){
    // TODO voronoi 1.2
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double xPos, yPos;
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &xPos, &yPos);
        auto x = (float)(xPos / width * 2.0f - 1.0f);
        auto y = - (float)(yPos / height * 2.0f - 1.0f);
        auto r = (float) dist(mt);
        auto g = (float) dist(mt);
        auto b = (float) dist(mt);
        sceneObjects.push_back(instantiateCone(r, g, b, x, y));
    }
}

// glfw: called whenever a keyboard key is pressed
void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods){
    // TODO voronoi 1.4

    // Set the activeShader variable by detecting when the keys 1, 2 and 3 were pressed;
    // see documentation at https://www.glfw.org/docs/latest/input_guide.html#input_keyboard
    // Key 1 sets the activeShader to &shaderPrograms[0];
    //   and so on.
    if(button == GLFW_KEY_1 && action == GLFW_PRESS)
        activeShader = &shaderPrograms[0];
    else if(button == GLFW_KEY_2 && action == GLFW_PRESS)
        activeShader = &shaderPrograms[1];
    else if(button == GLFW_KEY_3 && action == GLFW_PRESS)
        activeShader = &shaderPrograms[2];
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}