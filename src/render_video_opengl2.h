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
#include <glm/glm.hpp>
//#include <glm/detail/qualifier.hpp>
//#include <glm/detail/type_mat4x2.hpp>
//#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>


using std::cout;
using std::endl;

int window_width = 640;
int window_height = 480;

GLFWwindow *window;

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

static void draw_frame(const cv::Mat &frame) {
    glDisable(GL_DEPTH_TEST);


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

//4.66885e-310
//9.48606e-321
//1.4822e-323
//4.66885e-310

//0            0            0 9.48606e-321
//3.83472e-314            0            0  1.4822e-323
//4.79933e-314            0  6.9531e-310            0
//0            0  6.9531e-310 4.66885e-310


//glm::mat4 *initial_pose;
std::shared_ptr<openvslam::Mat44_t> initial_pose = nullptr;

Eigen::Matrix4d glmToEigen(const glm::highp_dmat4 &v) {
    Eigen::Matrix4d result;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            result(j, i) = v[i][j];
        }
    }

    return result;
}

void drawCube(openvslam::Mat44_t &pose) {

//    if (initial_pose == nullptr) {
//        initial_pose = std::make_shared<openvslam::Mat44_t>(pose);
//        cout << "[[ENTER]] -- MEM INITIAL POSE: " << initial_pose << endl;
//        cout << "[[ENTER]] -- INITIAL POSE: " << *initial_pose << endl;
//        return;
//    }


    glEnable(GL_DEPTH_TEST); // Depth Testing

    glMatrixMode(GL_PROJECTION_MATRIX);
    glLoadIdentity();
    gluPerspective(45, (double) window_width / (double) window_height, 0.1, 100);

    glMatrixMode(GL_MODELVIEW_MATRIX);

    glm::highp_dmat4 model = glm::highp_dmat4(1.0f);
    model = glm::translate(model, glm::highp_dvec3(0.01, 0.01, -5));

    const Eigen::Matrix4d eigen_model = glmToEigen(model);

    const auto model_view = pose * eigen_model;

    cout << model_view << endl;

    const double d = model_view(0);
    glLoadMatrixd(&d);


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


bool shouldWindowClose() {
    return glfwWindowShouldClose(window);
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


    glViewport(0, 0, window_width, window_height); // use a screen size of WIDTH x HEIGHT


}


void update(cv::Mat &frame, openvslam::Mat44_t &pose) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
////////////////////
    draw_frame(frame);


    drawCube(pose);
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



