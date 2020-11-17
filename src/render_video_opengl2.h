#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <iostream>

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include "openvslam/system.h"
#include "shader.h"

#include <opencv2/opencv.hpp>
#include <Eigen/src/Core/Matrix.h>
#include <glm/detail/qualifier.hpp>
#include <glm/detail/type_mat4x2.hpp>
#include <glm/ext.hpp>

using std::cout;
using std::endl;

int window_width = 640;
int window_height = 480;

GLFWwindow *window;
std::shared_ptr<Shader> ourShader;

unsigned int VBO, VAO;

//glm::vec3 *cubePositions;

const unsigned int cubesQuantitiy = 1;

glm::vec3 cubePositions[cubesQuantitiy] = {
        glm::vec3(0.0f, 0.0f, 0.0f)
//        glm::vec3(2.0f, 5.0f, -15.0f),
//        glm::vec3(-1.5f, -2.2f, -2.5f),
//        glm::vec3(-3.8f, -2.0f, -12.3f),
//        glm::vec3(2.4f, -0.4f, -3.5f),
//        glm::vec3(-1.7f, 3.0f, -7.5f),
//        glm::vec3(1.3f, -2.0f, -2.5f),
//        glm::vec3(1.5f, 2.0f, -2.5f),
//        glm::vec3(1.5f, 0.2f, -1.5f),
//        glm::vec3(-1.3f, 1.0f, -1.5f)
};

// Function turn a cv::Mat into a texture, and return the texture ID as a GLuint for use
static GLuint matToTexture(const cv::Mat &mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter) {
    // Generate a number for our textureID's unique handle
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Bind to our texture handle
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Catch silly-mistake texture interpolation method for magnification
    if (magFilter == GL_LINEAR_MIPMAP_LINEAR ||
        magFilter == GL_LINEAR_MIPMAP_NEAREST ||
        magFilter == GL_NEAREST_MIPMAP_LINEAR ||
        magFilter == GL_NEAREST_MIPMAP_NEAREST) {
        cout << "You can't use MIPMAPs for magnification - setting filter to GL_LINEAR" << endl;
        magFilter = GL_LINEAR;
    }

    // Set texture interpolation methods for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Set texture clamping method
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapFilter);

    // Set incoming texture format to:
    // GL_BGR       for CV_CAP_OPENNI_BGR_IMAGE,
    // GL_LUMINANCE for CV_CAP_OPENNI_DISPARITY_MAP,
    // Work out other mappings as required ( there's a list in comments in main() )
    GLenum inputColourFormat = GL_BGR;
    if (mat.channels() == 1) {
        inputColourFormat = GL_LUMINANCE;
    }

    // Create the texture
    glTexImage2D(GL_TEXTURE_2D,     // Type of texture
                 0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                 GL_RGB,            // Internal colour format to convert to
                 mat.cols,          // Image width  i.e. 640 for Kinect in standard mode
                 mat.rows,          // Image height i.e. 480 for Kinect in standard mode
                 0,                 // Border width in pixels (can either be 1 or 0)
                 inputColourFormat, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                 GL_UNSIGNED_BYTE,  // Image data type
                 mat.ptr());        // The actual image data itself

    // If we're using mipmaps then generate them. Note: This requires OpenGL 3.0 or higher
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return textureID;
}

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void resize_callback(GLFWwindow *window, int new_width, int new_height) {
    glViewport(0, 0, window_width = new_width, window_height = new_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}


static void init_opengl() {
    glViewport(0, 0, window_width, window_height); // use a screen size of WIDTH x HEIGHT

    glMatrixMode(GL_PROJECTION_MATRIX);     // Make a simple 2D projection on the entire window
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, 0.0, 100.0);

    glMatrixMode(GL_MODELVIEW_MATRIX);    // Set the matrix mode to object modeling

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
}

static void draw_frame(const cv::Mat &frame) {
    glDisable(GL_DEPTH_TEST);
    ourShader->disable();
//    frame_setup();
//    init_opengl();

    glMatrixMode(GL_PROJECTION_MATRIX);     // Make a simple 2D projection on the entire window
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, 0.0, 100.0);

    glMatrixMode(GL_MODELVIEW_MATRIX); // Operate on model-view matrix

    glEnable(GL_TEXTURE_2D);
    GLuint image_tex = matToTexture(frame, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP);

    /* Draw a quad */
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(0, 0);
    glTexCoord2i(0, 1);
    glVertex2i(0, window_height);
    glTexCoord2i(1, 1);
    glVertex2i(window_width, window_height);
    glTexCoord2i(1, 0);
    glVertex2i(window_width, 0);
    glEnd();

    glDeleteTextures(1, &image_tex);
    glDisable(GL_TEXTURE_2D);
}

void drawCube() {

    glEnable(GL_DEPTH_TEST); // Depth Testing
//    glDepthFunc(GL_LEQUAL);
//    glDisable(GL_CULL_FACE);
//    glCullFace(GL_BACK);

    glMatrixMode(GL_PROJECTION_MATRIX);
    glLoadIdentity();
    gluPerspective(60, (double) window_width / (double) window_height, 0.1, 100);

    glMatrixMode(GL_MODELVIEW_MATRIX);
    glTranslatef(0, 0, -5);

    GLfloat vertices[] =
            {
                    -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1,
                    1, -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, -1,
                    -1, -1, -1, -1, -1, 1, 1, -1, 1, 1, -1, -1,
                    -1, 1, -1, -1, 1, 1, 1, 1, 1, 1, 1, -1,
                    -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1, -1,
                    -1, -1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1
            };

    GLfloat colors[] =
            {
                    0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0,
                    1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0,
                    0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,
                    0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
                    0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0,
                    0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1
            };

    static float alpha = 0;
    //attempt to rotate cube
    glRotatef(alpha, 0, 1, 0);

    /* We have a color array and a vertex array */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(3, GL_FLOAT, 0, colors);

    /* Send data : 24 vertices */
    glDrawArrays(GL_QUADS, 0, 24);

    /* Cleanup states */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    alpha += 1;
}

template<typename T, int m, int n>
glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n> &em) {
    glm::mat<m, n, float, glm::precision::highp> mat;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            mat[j][i] = em(i, j);
        }
    }
    return mat;
}

void drawAugmentedScene(openvslam::Mat44_t &pose) {

    glEnable(GL_DEPTH_TEST);

//    cout << "Starting to draw models" << endl;

    // activate shader
    ourShader->use();

    // pass projection matrix to shader (note that in this case it could change every frame)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            (float) window_width / (float) window_height,
                                            0.1f,
                                            100.0f);
    ourShader->setMat4("projection", projection);

    // camera/view transformation
    glm::mat4 view = E2GLM(pose);

    cout << "view matrix: " << pose << endl;

    ourShader->setMat4("view", view);

    // render boxes
    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < cubesQuantitiy; i++) {
        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        ourShader->setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices
    }

//    cout << "Finished drawing models" << endl;

    ourShader->disable();

}


bool shouldWindowClose() {
    return glfwWindowShouldClose(window);
}

void setupModel() {

    ourShader = std::make_shared<Shader>("cam_vertex.vs", "cam_fragment.fs");

    float vertices[180] = {
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f
    };


    glGenVertexArrays(1, &VAO);

    std::cout << "LOG :: glGenVertexArrays SUCCESS" << std::endl;

    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

//    ourShader->use();
}


void setup() {

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(window_width, window_height, "AR", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, resize_callback);

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    //  Initialise glew (must occur AFTER window creation or glew will error)
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cout << "GLEW initialisation error: " << glewGetErrorString(err) << endl;
        exit(-1);
    }
    cout << "GLEW okay - using version: " << glewGetString(GLEW_VERSION) << endl;

//    setupModel();

    glViewport(0, 0, window_width, window_height); // use a screen size of WIDTH x HEIGHT

//    init_opengl();

}


void update(cv::Mat &frame, openvslam::Mat44_t &pose) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
////////////////////
    draw_frame(frame);
//    drawAugmentedScene(pose);

    drawCube();
////////////////////
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void terminate() {
    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}


//int nmain() {
//
//    setup();
//    cv::VideoCapture capture(0);
//    if (!capture.isOpened()) {
//        cout << "Cannot open video: " << endl;
//        exit(EXIT_FAILURE);
//    }
//
//    window_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
//    window_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//    cout << "Video width: " << window_width << endl;
//    cout << "Video height: " << window_height << endl;
//
//    cv::Mat frame;
//
//    while (!shouldWindowClose()) {
//        if (!capture.read(frame)) {
//            cout << "Cannot grab a frame." << endl;
//            break;
//        }
//        update(frame, nullptr);
//    }
//    terminate();
//}



