//
// Created by joaco on 11/11/20.
//

#ifndef OPEN_GL_TEST_DRAWER3_H
#define OPEN_GL_TEST_DRAWER3_H

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <opencv2/opencv.hpp>
#include <openvslam/type.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "stb_image.h"

using std::cout;
using std::endl;

class Drawer3 {

private:

    GLFWwindow *window;
    std::shared_ptr<Shader> ourShader;

    // Frame counting and limiting
    int frame_count = 0;
    double frame_start_time, frame_end_time, frame_draw_time;

    int window_width = 640;
    int window_height = 480;

    unsigned int texture1, texture2;
    unsigned int VBO, VAO;

    glm::vec3 cubePositions[10] = {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(2.0f, 5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3(2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f, 3.0f, -7.5f),
            glm::vec3(1.3f, -2.0f, -2.5f),
            glm::vec3(1.5f, 2.0f, -2.5f),
            glm::vec3(1.5f, 0.2f, -1.5f),
            glm::vec3(-1.3f, 1.0f, -1.5f)
    };

    GLuint matToTexture(const cv::Mat &mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter) {
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

//    static void resize_callback(GLFWwindow *window, int new_width, int new_height) {
//        glViewport(0, 0, window_width = new_width, window_height = new_height);
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glOrtho(0.0, window_width, window_height, 0.0, 0.0, 100.0);
//        glMatrixMode(GL_MODELVIEW);
//    }

    void init_opengl(int w, int h) {
        glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT

        glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
        glLoadIdentity();
        glOrtho(0.0, w, h, 0.0, 0.0, 100.0);

        glMatrixMode(GL_MODELVIEW);    // Set the matrix mode to object modeling

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
    }


    void draw_frame(const cv::Mat &frame) {
        cout << "Starting to draw frame" << endl;


        // Clear color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

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

        cout << "Finished drawing frame" << endl;

    }

//    template<typename T, int m, int n>
//    inline glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n> &em) {
//        glm::mat<m, n, float, glm::precision::highp> mat;
//        for (int i = 0; i < m; ++i) {
//            for (int j = 0; j < n; ++j) {
//                mat[j][i] = em(i, j);
//            }
//        }
//        return mat;
//    }

//    void drawAugmentedScene(openvslam::Mat44_t &pose) {
//
//        cout << "Starting to draw models" << endl;
//
//        // activate shader
//        ourShader->use();
//
//        // pass projection matrix to shader (note that in this case it could change every frame)
//        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) window_width / (float) window_height,
//                                                0.1f,
//                                                100.0f);
//        ourShader->setMat4("projection", projection);
//
//        // camera/view transformation
//        glm::mat4 view = E2GLM(pose);
//
////        cout << "view matrix: " << pose << endl;
//
//        ourShader->setMat4("view", view);
//
//        // render boxes
//        glBindVertexArray(VAO);
//        for (unsigned int i = 0; i < 10; i++) {
//            // calculate the model matrix for each object and pass it to shader before drawing
//            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
//            model = glm::translate(model, cubePositions[i]);
//            float angle = 20.0f * i;
//            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
//            ourShader->setMat4("model", model);
//
//            glDrawArrays(GL_TRIANGLES, 0, 36);
//        }
//
//        cout << "Finished drawing models" << endl;
//
//    }

public:
    Drawer3(int window_width, int window_height) {

        this->window_width = window_width;
        this->window_height = window_height;

        glfwSetErrorCallback(error_callback);

        if (!glfwInit()) {
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        window = glfwCreateWindow(window_width, window_height, "Simple example", NULL, NULL);
        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwSetKeyCallback(window, key_callback);
//        glfwSetWindowSizeCallback(window, resize_callback);

        glfwMakeContextCurrent(window);

        glfwSwapInterval(1);

        //  Initialise glew (must occur AFTER window creation or glew will error)
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            cout << "GLEW initialisation error: " << glewGetErrorString(err) << endl;
            exit(-1);
        }
        cout << "GLEW okay - using version: " << glewGetString(GLEW_VERSION) << endl;

        ourShader = std::make_shared<Shader>("cam_vertex.vs", "cam_fragment.fs");

        init_opengl(window_width, window_height);

        double video_start_time = glfwGetTime();
        double video_end_time = 0.0;

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
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

        // load and create a texture
        // -------------------------

        // texture 1
        // ---------
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load image, create texture and generate mipmaps
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
        unsigned char *data = stbi_load("./resources/textures/container.jpg", &width,
                                        &height,
                                        &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);
        // texture 2
        // ---------
        glGenTextures(1, &texture2);
        glBindTexture(GL_TEXTURE_2D, texture2);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load image, create texture and generate mipmaps
        data = stbi_load("./resources/textures/awesomeface.png", &width, &height,
                         &nrChannels,
                         0);
        if (data) {
            // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);

        // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
        // -------------------------------------------------------------------------------------------
        ourShader->use();
        ourShader->setInt("texture1", 0);
        ourShader->setInt("texture2", 1);
    }

    void update(cv::Mat &frame, openvslam::Mat44_t &pose) {
//        frame_start_time = glfwGetTime();

        draw_frame(frame);
//        drawAugmentedScene(pose);

//        video_end_time = glfwGetTime();

        glfwSwapBuffers(window);
        glfwPollEvents();

//        ++frame_count;
//        lock_frame_rate(fps);
    }

    bool shouldWindowClose() {
        return glfwWindowShouldClose(window);
    }

    void terminate() {
        glfwDestroyWindow(window);
        glfwTerminate();

    }


};

#endif //OPEN_GL_TEST_DRAWER3_H
