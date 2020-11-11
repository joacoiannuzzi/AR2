

//
// Created by joaco on 5/11/20.
//

//#include <glad/glad.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>


#include "shader.h"
//#include "camera.h"

#include <opencv2/highgui.hpp>
#include <openvslam/type.h>
#include <opencv2/imgproc.hpp>

class Drawer2 {
private:
    GLFWwindow *window;

    // build and compile our shader program
    std::shared_ptr<Shader> ourShader;

    unsigned int VBO, VAO, EBO;

    unsigned int texture;

    // settings
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

// camera
//    std::shared_ptr<Camera> camera;

// timing
    float deltaTime = 0.0f;    // time between current frame and last frame
    float lastFrame = 0.0f;

    static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        glViewport(0, 0, width, height);
    }

    void processInput(GLFWwindow *window) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

    }

    unsigned char *cvMat2TexInput(cv::Mat &img) {
        cvtColor(img, img, cv::COLOR_BGR2RGB);
        flip(img, img, -1);
        return img.data;
    }

    void draw(cv::Mat &frame) {
        unsigned char *image = cvMat2TexInput(frame);
        if (image) {
            int width = frame.cols;
            int height = frame.rows;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        } else {
            std::cout << "Failed to load texture." << std::endl;
        }

        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Draw Rectangle
        ourShader->use();
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void drawCameraFrame(const cv::Mat &frame) {

        std::cout << "Drawing camera frame" << std::endl;

        int w = frame.cols;
        int h = frame.rows;

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Upload new texture data:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);


        const GLfloat bgTextureVertices[] = {0, 0, static_cast<GLfloat>(w), 0, 0, static_cast<GLfloat>(h),
                                             static_cast<GLfloat>(w), static_cast<GLfloat>(h)};
        const GLfloat bgTextureCoords[] = {1, 0, 1, 1, 0, 0, 0, 1};
        const GLfloat proj[] = {0, -2.f / w, 0, 0, -2.f / h, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1};

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(proj);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Update attribute values.
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(2, GL_FLOAT, 0, bgTextureVertices);
        glTexCoordPointer(2, GL_FLOAT, 0, bgTextureCoords);

        glColor4f(1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);

        std::cout << "Finished drawing camera frame" << std::endl;

    }


    void setup() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for Mac OSX
#endif

        // glfw window creation
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "AR", NULL, NULL);
        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // glad: load all OpenGL function pointers
//        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
//            std::cout << "Failed to initialize GLAD" << std::endl;
//            return;
//        }

        //  Initialise glew (must occur AFTER window creation or glew will error)
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            std::cout << "GLEW initialisation error: " << glewGetErrorString(err) << std::endl;
            exit(-1);
        }
        std::cout << "GLEW okay - using version: " << glewGetString(GLEW_VERSION) << std::endl;


        std::cout << "LOG :: GL INITIALIZING" << std::endl;

        glEnable(GL_DEPTH_TEST);

        std::cout << "LOG :: GL ENABLED" << std::endl;


        ourShader = std::make_shared<Shader>("cam_vertex2.vs", "cam_fragment2.fs");

        float vertices[] = {
                //     Position       TexCoord
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top right
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // below left
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f  // below right
        };
        // Set up index
        unsigned int indices[] = {
                0, 1, 2,
                1, 2, 3
        };

//        unsigned int VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

//        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    }

public:

    Drawer2() {
        setup();
    }


    void update(cv::Mat &frame, openvslam::Mat44_t &pose) {
        std::cout << "Drawer starting to draw" << std::endl;

//        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); // Clear entire screen:
//        drawCameraFrame(frame);
//        glFlush();
        draw(frame);

        std::cout << "Drawer finished drawing" << std::endl;


    }

    bool shouldWindowClose() {
        return glfwWindowShouldClose(window);
    }

    void terminate() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);

        // glfw: terminate, clearing all previously allocated GLFW resources.
        // ------------------------------------------------------------------
        glfwTerminate();
    }

};
