#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <jsoncpp/json/json.h>
#include "MatterSim.hpp"
#include "Benchmark.hpp"

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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, xneg.rows, xneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, xneg.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, ypos.rows, ypos.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, ypos.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, yneg.rows, yneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, yneg.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, zpos.rows, zpos.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, zpos.ptr());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, zneg.rows, zneg.cols, 0, GL_BGR, GL_UNSIGNED_BYTE, zneg.ptr());
}

void Simulator::setDatasetPath(std::string path) {
    datasetPath = path;
}

void Simulator::setNavGraphPath(std::string path) {
    navGraphPath = path;
}

void Simulator::setScreenResolution(int width, int height) {
    this->width = width;
    this->height = height;
}

void Simulator::setScanId(std::string id) {
    scanId = id;
}

void Simulator::init() {
    state->rgb.create(height, width, CV_8UC3);

    ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
    buffer = malloc(width * height * 4 * sizeof(GLubyte));
    if (!buffer) {
      throw std::runtime_error( "Malloc image buffer failed" );
    }
    if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, width, height)){
      throw std::runtime_error( "OSMesaMakeCurrent failed" );
    }

    Json::Value root;
    auto navGraphFile =  navGraphPath + "/" + scanId + "_connectivity.json";
    std::ifstream ifs(navGraphFile, std::ifstream::in);
    if (ifs.fail()){
        throw std::invalid_argument( "Could not open navigation graph file: " + navGraphFile );
    }
    ifs >> root;

    for (auto viewpoint : root) {
        float posearr[16];
        int i = 0;
        for (auto f : viewpoint["pose"]) {
            posearr[i++] = f.asFloat();
        }
        glm::mat4 pose = glm::transpose(glm::make_mat4(posearr));

        glm::vec3 pos{pose[3][0], pose[3][1], pose[3][2]};
        pos[0] = -pos[0]; // Flip x coordinate since we flip x in the model matrix.
        pose[3] = {0,0,0,1};

        std::vector<bool> unobstructed;
        for (auto u : viewpoint["unobstructed"]) {
            unobstructed.push_back(u.asBool());
        }

        auto image_id = viewpoint["image_id"].asString();
        GLuint cubemap_texture = 0;

        Location l{viewpoint["included"].asBool(), image_id, pose, pos, unobstructed, cubemap_texture};
        locations.push_back(std::make_shared<Location>(l));
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
    Model      = glm::scale(glm::mat4(1.0f),glm::vec3(-50,50,50));

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

void Simulator::populateNavigable() {
    std::vector<ViewpointPtr> updatedNavigable;
    updatedNavigable.push_back(state->location);
    unsigned int idx = state->location->id;
    unsigned int i = 0;
    cv::Point3f curPos = state->location->location;
    float adjustedheading = state->heading + M_PI / 2;
    for (auto u : locations[idx]->unobstructed) {
        if (u) {
            if (i == state->location->id) {
                std::cout << "self-reachable" << std::endl;
                continue;
            }
            glm::vec3 pos(locations[i]->pos);
            float bearing = atan2(pos[1] - curPos.y, pos[0] - curPos.x);
            float headingDelta = bearing - adjustedheading;
            while (headingDelta > M_PI) {
                headingDelta -= 2 * M_PI;
            }
            while (headingDelta < -M_PI) {
                headingDelta += 2 * M_PI;
            }
            if (fabs(headingDelta) < M_PI / 4) {
                Viewpoint v{i, cv::Point3f(pos[0], pos[1], pos[2])};
                updatedNavigable.push_back(std::make_shared<Viewpoint>(v));
            }
        }
        i++;
    }

    for (auto v : state->navigableLocations) {
        bool retained = false;
        for (auto updatedV : updatedNavigable) {
            if (v->id == updatedV->id) {
                retained = true;
                break;
            }
        }
        if (!retained) {
            glDeleteTextures(1, &locations[v->id]->cubemap_texture);
            locations[v->id]->cubemap_texture = 0;
        }
    }
    state->navigableLocations = updatedNavigable;
}

void Simulator::loadTexture(int locationId) {
    cpuLoadTimer.Start();
    auto datafolder = datasetPath + "/v1/scans/" + scanId + "/matterport_skybox_images/";
    auto image_id = locations[locationId]->image_id;
    auto xpos = cv::imread(datafolder + image_id + "_skybox2_sami.jpg");
    auto xneg = cv::imread(datafolder + image_id + "_skybox4_sami.jpg");
    auto ypos = cv::imread(datafolder + image_id + "_skybox0_sami.jpg");
    auto yneg = cv::imread(datafolder + image_id + "_skybox5_sami.jpg");
    auto zpos = cv::imread(datafolder + image_id + "_skybox1_sami.jpg");
    auto zneg = cv::imread(datafolder + image_id + "_skybox3_sami.jpg");
    if (xpos.empty() || xneg.empty() || ypos.empty() || yneg.empty() || zpos.empty() || zneg.empty()) {
      throw std::invalid_argument( "Could not open skybox files at: " + datafolder + image_id + "_skybox*_sami.jpg");
    }
    cpuLoadTimer.Stop();
    gpuLoadTimer.Start();
    setupCubeMap(locations[locationId]->cubemap_texture, xpos, xneg, ypos, yneg, zpos, zneg);
    gpuLoadTimer.Stop();
    if (!glIsTexture(locations[state->location->id]->cubemap_texture)){
      throw std::runtime_error( "loadTexture failed" );
    }
}

void Simulator::newEpisode() {
    std::cout << "FIXME new episode" << std::endl;
    std::default_random_engine generator;
    generator.seed(time(NULL));
    std::uniform_int_distribution<int> distribution(0,locations.size()-1);
    int ix = distribution(generator);  // generates random starting point
    std::cout << "Starting at " << ix <<  std::endl;
    glm::vec3 pos(locations[ix]->pos);
    Viewpoint v{ix, cv::Point3f(pos[0], pos[1], pos[2])};
    state->location = std::make_shared<Viewpoint>(v);
    populateNavigable();
    totalTimer.Start();
    loadTexture(state->location->id);
    totalTimer.Stop();
}

bool Simulator::isEpisodeFinished() {
    //std::cout << "FIXME episodes!" << std::endl;
    return false;
}

SimStatePtr Simulator::getState() {
    return this->state;
}

void Simulator::makeAction(int index, float heading, float elevation) {
    totalTimer.Start();
    // move
    if (index < 0 || index >= state->navigableLocations.size() ){
        throw std::domain_error( "Invalid action index: " + index );
    }
    state->location = state->navigableLocations[index];
    populateNavigable();
    state->heading += heading; // TODO handle boundaries
    state->elevation += elevation;
    
    // loading cubemap
    if (!glIsTexture(locations[state->location->id]->cubemap_texture)) {
        loadTexture(state->location->id);
    }

    renderTimer.Start();
    // rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), state->elevation - (float)M_PI / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
    glm::mat4 RotateY = glm::rotate(RotateX, state->heading, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 M = Projection * View * Model * RotateY * locations[state->location->id]->pose;
    glUniformMatrix4fv(PVM, 1, GL_FALSE, glm::value_ptr(M));

    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_CUBE_MAP, locations[state->location->id]->cubemap_texture);
    glDrawElements(GL_QUADS, sizeof(cube_indices)/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

    cv::Mat img(height, width, CV_8UC3);
    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);

    //set length of one complete row in destination data (doesn't need to equal img.cols)
    glPixelStorei(GL_PACK_ROW_LENGTH, img.step/img.elemSize());
    glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    cv::flip(img, img, 0);
    this->state->rgb = img;
    renderTimer.Stop();
    totalTimer.Stop();
    
    std::cout << "\ntotalTimer: " << totalTimer.MilliSeconds() << " ms" << std::endl;
    std::cout << "cpuLoadTimer: " << cpuLoadTimer.MilliSeconds() << " ms" << std::endl;
    std::cout << "gpuLoadTimer: " << gpuLoadTimer.MilliSeconds() << " ms" << std::endl;
    std::cout << "renderTimer: " << renderTimer.MilliSeconds() << " ms" << std::endl;
    
    //cpuLoadTimer.Reset();
    //gpuLoadTimer.Reset();
    //renderTimer.Reset();
    //totalTimer.Reset();
}

void Simulator::close() {
   std::cout << "FIXME implement close" << std::endl;
   free( buffer );
   OSMesaDestroyContext( ctx );
}
}
