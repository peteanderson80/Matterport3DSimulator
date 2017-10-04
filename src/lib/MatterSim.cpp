#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MatterSim.hpp"

namespace mattersim {

// cube indices for index buffer object
GLushort cube_indices[] = {
    0, 1, 2, 3,
    3, 2, 6, 7,
    7, 6, 5, 4,
    4, 5, 1, 0,
    0, 3, 7, 4,
    1, 2, 6, 5,
};

char* loadFile(const char *filename) {
    char* data;
    int len;
    std::ifstream ifs(filename, std::ifstream::in);

    ifs.seekg(0, std::ios::end);
    len = ifs.tellg();

    ifs.seekg(0, std::ios::beg);
    data = new char[len + 1];

    ifs.read(data, len);
    data[len] = 0;

    ifs.close();

    return data;
}


void setupCubeMap(GLuint& texture) {
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void setupCubeMap(GLuint& texture, cv::Mat &xpos, cv::Mat &xneg, cv::Mat &ypos, cv::Mat &yneg, cv::Mat &zpos, cv::Mat &zneg) {
    setupCubeMap(texture);
    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_UNPACK_ALIGNMENT, (xneg.step & 3) ? 1 : 4);
    //set length of one complete row in data (doesn't need to equal image.cols)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, xneg.step/xneg.elemSize());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, xpos.rows, xpos.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, xpos.ptr());
    std::cout << glGetError() << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, xneg.rows, xneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, xneg.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, ypos.rows, ypos.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, ypos.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, yneg.rows, yneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, yneg.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, zpos.rows, zpos.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, zpos.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, zneg.rows, zneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, zneg.ptr());
}

void Simulator::setDatasetPath(std::string path) {
    datasetPath = path; // FIXME check for existence?
}

void Simulator::setNavGraphPath(std::string path) {
    navGraphPath = path; // FIXME check for existence?
}

void Simulator::setScreenResolution(int width, int height) {
    this->width = width;
    this->height = height;
}

void Simulator::setScanId(std::string id) {
    scanId = id; // FIXME check for existence?
}

void Simulator::init() {
    state->rgb.create(height, width, CV_8UC3);

    cv::namedWindow("renderwin", cv::WINDOW_OPENGL);
    cv::setOpenGlContext("renderwin");
    // initialize the extension wrangler
    glewInit();

    // set up the cube map texture
    auto datafolder = datasetPath + "/v1/scans/" + scanId + "/matterport_skybox_images/";

    auto xpos = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox2_sami.jpg");
    auto xneg = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox4_sami.jpg");
    auto ypos = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox0_sami.jpg");
    auto yneg = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox5_sami.jpg");
    auto zpos = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox1_sami.jpg");
    auto zneg = cv::imread(datafolder + "7b017f053981438a9c075e599f2c5866_skybox3_sami.jpg");
    GLuint cubemap_texture;
    setupCubeMap(cubemap_texture, xpos, xneg, ypos, yneg, zpos, zneg);

// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

// The texture we're going to render to
    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);

// "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, renderedTexture);

// Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);

// Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

// Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

// Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

// Always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "FRAMEBUFFER FAILURE" << std::endl;
        return;
    }

    // set our viewport, clear color and depth, and enable depth testing
    glViewport(0, 0, this->width, this->height);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // load our shaders and compile them.. create a program and link it
    GLuint glShaderV = glCreateShader(GL_VERTEX_SHADER);
    GLuint glShaderF = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vShaderSource = loadFile("src/lib/vertex.sh");
    const GLchar* fShaderSource = loadFile("src/lib/fragment.sh");
    glShaderSource(glShaderV, 1, &vShaderSource, NULL);
    glShaderSource(glShaderF, 1, &fShaderSource, NULL);
    delete [] vShaderSource;
    delete [] fShaderSource;
    glCompileShader(glShaderV);
    glCompileShader(glShaderF);
    GLuint glProgram = glCreateProgram();
    glAttachShader(glProgram, glShaderV);
    glAttachShader(glProgram, glShaderF);
    glLinkProgram(glProgram);
    glUseProgram(glProgram);

    // shader logs
    int  vlength,    flength;
    char vlog[2048], flog[2048];
    glGetShaderInfoLog(glShaderV, 2048, &vlength, vlog);
    glGetShaderInfoLog(glShaderF, 2048, &flength, flog);

    // grab the pvm matrix and vertex location from our shader program
    PVM    = glGetUniformLocation(glProgram, "PVM");
    vertex = glGetAttribLocation(glProgram, "vertex");

    // these won't change for now
    Projection = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    View       = glm::mat4(1.0f);
    Model      = glm::scale(glm::mat4(1.0f),glm::vec3(50,50,50));

    // cube vertices for vertex buffer object
    GLfloat cube_vertices[] = {
        -1.0,  1.0,  1.0,
        -1.0, -1.0,  1.0,
        1.0, -1.0,  1.0,
        1.0,  1.0,  1.0,
        -1.0,  1.0, -1.0,
        -1.0, -1.0, -1.0,
        1.0, -1.0, -1.0,
        1.0,  1.0, -1.0,
    };
    GLuint vbo_cube_vertices;
    glGenBuffers(1, &vbo_cube_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(vertex);
    glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint ibo_cube_indices;
    glGenBuffers(1, &ibo_cube_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
}

void Simulator::newEpisode() {
    std::cout << "FIXME new episode" << std::endl;
}

bool Simulator::isEpisodeFinished() {
    //std::cout << "FIXME episodes!" << std::endl;
    return false;
}

SimStatePtr Simulator::getState() {
    return this->state;
}

void Simulator::makeAction(int index, float heading, float elevation) {
    // rendering
    state->heading = heading;
    state->elevation = elevation;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), state->elevation, glm::vec3(-1.0f, 0.0f, 0.0f));
    glm::mat4 RotateY = glm::rotate(RotateX, state->heading, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 M = Projection * View * Model * RotateY;
    glUniformMatrix4fv(PVM, 1, GL_FALSE, glm::value_ptr(M));

    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    glViewport(0, 0, width, height);
    glDrawElements(GL_QUADS, sizeof(cube_indices)/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

    cv::Mat img(height, width, CV_8UC3);
    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);

    //set length of one complete row in destination data (doesn't need to equal img.cols)
    glPixelStorei(GL_PACK_ROW_LENGTH, img.step/img.elemSize());
    glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    cv::flip(img, img, 0);
    this->state->rgb = img;
}

void Simulator::close() {
    std::cout << "FIXME implement close" << std::endl;
}
}
