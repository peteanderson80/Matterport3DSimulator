#include <iostream>
#include <fstream>

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
    5, 6, 2, 1,
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



Simulator::Simulator() :state{new SimState()},
                        width(320),
                        height(240),
                        vfov(0.8),
                        minElevation(-0.94),
                        maxElevation(0.94),
                        frames(0),
                        navGraphPath("./connectivity"),
                        datasetPath("./data/v1/scans/"),
#ifdef OSMESA_RENDERING
                        buffer(NULL),
#endif
                        initialized(false),
                        renderingEnabled(true),
                        discretizeViews(false),
                        preloadImages(false),
                        renderDepth(false),
                        randomSeed(1),
                        cacheSize(200) {
};

Simulator::~Simulator() {
    close();
}


void Simulator::setCameraResolution(int width, int height) {
    this->width = width;
    this->height = height;
}

void Simulator::setCameraVFOV(double vfov) {
    this->vfov = vfov;
}

void Simulator::setRenderingEnabled(bool value) {
    if (!initialized) {
        renderingEnabled = value;
    }
}

void Simulator::setDiscretizedViewingAngles(bool value) {
    if (!initialized) {
        discretizeViews = value;
    }
}

void Simulator::setPreloadingEnabled(bool value) {
     if (!initialized) {
        preloadImages = value;
    } 
}

void Simulator::setDepthEnabled(bool value) {
     if (!initialized) {
        renderDepth = value;
    } 
}

void Simulator::setDatasetPath(const std::string& path) {
    if (!initialized) {
        datasetPath = path;
    }
}

void Simulator::setNavGraphPath(const std::string& path) {
    if (!initialized) {
        navGraphPath = path;
    }
}

void Simulator::setSeed(int seed) {
    if (!initialized) {
        randomSeed = seed;
    }
}

void Simulator::setCacheSize(unsigned int size) {
    if (!initialized) {
        cacheSize = size;
    }
}

void Simulator::initialize() {
    state->rgb = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    state->depth = cv::Mat(height, width, CV_16UC1, cv::Scalar(0));
    if (renderingEnabled) {
#ifdef OSMESA_RENDERING
        ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
        buffer = malloc(width * height * 4 * sizeof(GLubyte));
        if (!buffer) {
            throw std::runtime_error( "MatterSim: Malloc image buffer failed" );
        }
        if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, width, height)) {
            throw std::runtime_error( "MatterSim: OSMesaMakeCurrent failed" );
        }
#elif defined (EGL_RENDERING)
        // Initialize EGL
        eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        assertEGLError("eglGetDisplay");

        EGLint major, minor;

        eglInitialize(eglDpy, &major, &minor);
        assertEGLError("eglInitialize");

        // Select an appropriate configuration
        EGLint numConfigs;
        EGLConfig eglCfg;

        const EGLint configAttribs[] = {
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
          EGL_NONE
        };

        eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
        assertEGLError("eglChooseConfig");

        // Bind the API
        eglBindAPI(EGL_OPENGL_API);
        assertEGLError("eglBindAPI");

        // Create a context and make it current
        EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);
        assertEGLError("eglCreateContext");
        eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglCtx);
        assertEGLError("eglMakeCurrent");

#else
        cv::namedWindow("renderwin", cv::WINDOW_OPENGL);
        cv::setOpenGlContext("renderwin");
        // initialize the extension wrangler
        glewInit();
#endif

#ifndef OSMESA_RENDERING
        GLuint FramebufferName;
        glGenFramebuffers(1, &FramebufferName);
        assertOpenGLError("glGenFramebuffers");
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
        assertOpenGLError("glBindFramebuffer");

        // The texture we're going to render to
        GLuint renderedTexture;
        glGenTextures(1, &renderedTexture);
        assertOpenGLError("glGenTextures");

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, renderedTexture);
        assertOpenGLError("glBindTexture");

        // Give an empty image to OpenGL ( the last "0" )
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        assertOpenGLError("glTexImage2D");

        // Poor filtering. Needed !
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        assertOpenGLError("glTexParameteri");

        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);
        assertOpenGLError("glFramebufferTexture2D");

        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
        assertOpenGLError("glDrawBuffers");

        // Always check that the framebuffer is ok
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
            throw std::runtime_error( "MatterSim: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        } else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
            throw std::runtime_error( "MatterSim: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        } else if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
            throw std::runtime_error( "MatterSim: GL_FRAMEBUFFER_UNSUPPORTED");
        } else if(status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error( "MatterSim: GL_FRAMEBUFFER other failure ");
        }
#endif

        // set our viewport, clear color and depth, and enable depth testing
        glViewport(0, 0, this->width, this->height);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // load our shaders and compile them.. create a program and link it
        glShaderV = glCreateShader(GL_VERTEX_SHADER);
        glShaderF = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar* vShaderSource = loadFile("src/lib/vertex.sh");
        const GLchar* fShaderSource = loadFile("src/lib/fragment.sh");
        glShaderSource(glShaderV, 1, &vShaderSource, NULL);
        glShaderSource(glShaderF, 1, &fShaderSource, NULL);
        delete [] vShaderSource;
        delete [] fShaderSource;
        glCompileShader(glShaderV);
        glCompileShader(glShaderF);
        glProgram = glCreateProgram();
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
        PVMM   = glGetUniformLocation(glProgram, "PVMM");
        vertex = glGetAttribLocation(glProgram, "vertex");

        // these won't change
        Projection = glm::perspective((float)vfov, (float)width / (float)height, 0.1f, 100.0f);
        Scale      = glm::scale(glm::mat4(1.0f),glm::vec3(10,10,10)); // Scale cube to 10m

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
        glGenBuffers(1, &vbo_cube_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(vertex);
        glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glGenBuffers(1, &ibo_cube_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

        if (preloadImages) {
            // trigger loading from disk now, to get predictable timing later
            preloadTimer.Start();
            auto& navGraph = NavGraph::getInstance(navGraphPath, datasetPath, preloadImages, 
                              renderDepth, randomSeed, cacheSize);
            preloadTimer.Stop();
        }
    }
    initialized = true;
}

void Simulator::populateNavigable() {
    std::vector<ViewpointPtr> updatedNavigable;
    updatedNavigable.push_back(state->location);
    unsigned int idx = state->location->ix;
    double adjustedheading = M_PI/2.0 - state->heading;
    glm::vec3 camera_horizon_dir(cos(adjustedheading), sin(adjustedheading), 0.f);
    double cos_half_hfov = cos(vfov * width / height / 2.0);
    
    auto& navGraph = NavGraph::getInstance(navGraphPath, datasetPath, preloadImages, renderDepth, randomSeed, cacheSize);
    for (unsigned int i : navGraph.adjacentViewpointIndices(state->scanId, idx)) {
        // Check if visible between camera left and camera right
        glm::vec3 target_dir = navGraph.cameraPosition(state->scanId,i) - navGraph.cameraPosition(state->scanId,idx);
        double rel_distance = glm::length(target_dir);
        double tar_z = target_dir.z;
        target_dir.z = 0.f; // project to xy plane
        double rel_elevation = atan2(tar_z, glm::length(target_dir)) - state->elevation;
        glm::vec3 normed_target_dir = glm::normalize(target_dir);
        double cos_angle = glm::dot(normed_target_dir, camera_horizon_dir);
        if (cos_angle >= cos_half_hfov) {
            glm::vec3 pos = navGraph.cameraPosition(state->scanId,i);
            double rel_heading = atan2( target_dir.x*camera_horizon_dir.y - target_dir.y*camera_horizon_dir.x,
                    target_dir.x*camera_horizon_dir.x + target_dir.y*camera_horizon_dir.y );
            Viewpoint v{navGraph.viewpoint(state->scanId,i), i, pos[0], pos[1], pos[2],
                  rel_heading, rel_elevation, rel_distance};
            updatedNavigable.push_back(std::make_shared<Viewpoint>(v));
        }
    }
    std::sort(updatedNavigable.begin(), updatedNavigable.end(), ViewpointPtrComp());
    state->navigableLocations = updatedNavigable;
}

void Simulator::setHeadingElevation(double heading, double elevation) {
    // Normalize heading to range [0, 360]
    state->heading = fmod(heading, M_PI*2.0);
    while (state->heading < 0.0) {
        state->heading += M_PI*2.0;
    }
    if (discretizeViews) {
        // Snap heading to nearest discrete value
        double headingIncrement = M_PI*2.0/headingCount;
        int heading_step = std::lround(state->heading/headingIncrement);
        if (heading_step == headingCount) heading_step = 0;
        state->heading = (double)heading_step * headingIncrement;
        // Snap elevation to nearest discrete value (disregarding elevation limits)
        state->elevation = elevation;
        if (state->elevation < -elevationIncrement/2.0) {
          state->elevation = -elevationIncrement;
          state->viewIndex = heading_step;
        } else if (state->elevation > elevationIncrement/2.0) {
          state->elevation = elevationIncrement;
          state->viewIndex = heading_step + 2*headingCount;
        } else {
          state->elevation = 0.0;
          state->viewIndex = heading_step + headingCount;
        }
    } else {
        // Set elevation with limits
        state->elevation = std::max(std::min(elevation, maxElevation), minElevation);
    }
}

bool Simulator::setElevationLimits(double min, double max) {
    if (min < 0.0 && min > -M_PI/2.0 && max > 0.0 && max < M_PI/2.0) {
        minElevation = min;
        maxElevation = max;
        return true;
    } else {
        return false;
    }
}

void Simulator::newEpisode(const std::string& scanId,
                           const std::string& viewpointId,
                           double heading,
                           double elevation) {
    wallTimer.Start();
    processTimer.Start();
    if (!initialized) {
        initialize();
    }
    state->step = 0;
    state->scanId = scanId;
    setHeadingElevation(heading, elevation);
    auto& navGraph = NavGraph::getInstance(navGraphPath, datasetPath, preloadImages, renderDepth, randomSeed, cacheSize);
    const std::string vid = viewpointId.empty() ? navGraph.randomViewpoint(scanId) : viewpointId;
    unsigned int ix = navGraph.index(state->scanId, vid);
    glm::vec3 pos = navGraph.cameraPosition(scanId, ix);
    Viewpoint v {
        vid, 
        ix,
        pos[0], pos[1], pos[2], 
        0.0, 0.0, 0.0
    };
    state->location = std::make_shared<Viewpoint>(v);
    populateNavigable();
    if (renderingEnabled) {
        renderScene();
    }
    processTimer.Stop();
}

SimStatePtr Simulator::getState() {
    return this->state;
}

void Simulator::renderScene() {
    frames += 1;
    loadTimer.Start();
    auto& navGraph = NavGraph::getInstance(navGraphPath, datasetPath, preloadImages, renderDepth, randomSeed, cacheSize);
    std::pair<GLuint, GLuint> texIds = navGraph.cubemapTextures(state->scanId, state->location->ix);
    loadTimer.Stop();
    renderTimer.Start();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Scale and move the cubemap model into position
    Model = navGraph.cameraRotation(state->scanId,state->location->ix) * Scale;
    // Opengl camera looking down -z axis. Rotate around x by -90deg (now looking down +y). Add positive elevation to look up.
    RotateX = glm::rotate(glm::mat4(1.0f), -(float)M_PI / 2.0f + (float)state->elevation, glm::vec3(1.0f, 0.0f, 0.0f));
    // Rotate camera around z for heading, positive heading will turn right.
    View = glm::rotate(RotateX, (float)M_PI + (float)state->heading, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 M = Projection * View * Model;
    glUniformMatrix4fv(PVMM, 1, GL_FALSE, glm::value_ptr(M));
    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texIds.first);
    glDrawElements(GL_QUADS, sizeof(cube_indices)/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
    renderTimer.Stop();
    gpuReadTimer.Start();
    cv::Mat img = this->state->rgb;
    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
    //set length of one complete row in destination data (doesn't need to equal img.cols)
    glPixelStorei(GL_PACK_ROW_LENGTH, img.step/img.elemSize());
    glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    gpuReadTimer.Stop();
    assertOpenGLError("render RGB");
    if (renderDepth) {
        renderTimer.Start();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texIds.second);
        glDrawElements(GL_QUADS, sizeof(cube_indices)/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
        renderTimer.Stop();
        gpuReadTimer.Start();
        cv::Mat img = this->state->depth;
        //use fast 4-byte alignment (default anyway) if possible
        glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
        //set length of one complete row in destination data (doesn't need to equal img.cols)
        glPixelStorei(GL_PACK_ROW_LENGTH, img.step/img.elemSize());
        glReadPixels(0, 0, img.cols, img.rows, GL_RED, GL_UNSIGNED_SHORT, img.data);
        gpuReadTimer.Stop();
        assertOpenGLError("render Depth");
    }
}

void Simulator::makeAction(int index, double heading, double elevation) {
    processTimer.Start();
    // move
    if (!initialized || index < 0 || index >= state->navigableLocations.size() ){
        std::stringstream msg;
        msg << "MatterSim: Invalid action index: " << index;
        throw std::domain_error( msg.str() );
    }
    state->location = state->navigableLocations[index];
    state->location->rel_heading = 0.0;
    state->location->rel_elevation = 0.0;
    state->location->rel_distance = 0.0;
    state->step += 1;
    if (discretizeViews) {
        // Increments based on sign of input
        if (heading > 0.0) heading = M_PI*2.0/headingCount;
        if (heading < 0.0) heading = -M_PI*2.0/headingCount;
        if (elevation > 0.0) elevation =  elevationIncrement;
        if (elevation < 0.0) elevation = -elevationIncrement;
    }
    setHeadingElevation(state->heading + heading, state->elevation + elevation);
    populateNavigable();
    if (renderingEnabled) {
        renderScene();
    }
    processTimer.Stop();
}

void Simulator::close() {
    if (initialized) {
        if (renderingEnabled) {
            // release vertex and index buffer object
            glDeleteBuffers(1, &ibo_cube_indices);
            glDeleteBuffers(1, &vbo_cube_vertices);
            // detach shaders from program and release
            glDetachShader(glProgram, glShaderF);
            glDetachShader(glProgram, glShaderV);
            glDeleteShader(glShaderF);
            glDeleteShader(glShaderV);
            glDeleteProgram(glProgram);
#ifdef OSMESA_RENDERING
            free( buffer );
            buffer = NULL;
            OSMesaDestroyContext( ctx );
#elif defined (EGL_RENDERING)
            eglTerminate(eglDpy);
#else
            cv::destroyAllWindows();
#endif
        }
        initialized = false;
    }
}

void Simulator::resetTimers() {
    loadTimer.Reset();
    renderTimer.Reset();
    gpuReadTimer.Reset();
    processTimer.Reset();
    wallTimer.Reset();
}

std::string Simulator::timingInfo() {
    std::ostringstream oss;
    float f = static_cast<float>(frames);
    float pret = preloadTimer.Seconds() / 60.0;
    float wt = wallTimer.MilliSeconds();
    float pt = processTimer.MilliSeconds();
    float lt = loadTimer.MilliSeconds();
    float rt = renderTimer.MilliSeconds();
    float it = gpuReadTimer.MilliSeconds();
    oss << "Preloading images: " << pret << " minutes" << std::endl;
    oss << "Rendered " << f << " frames" << std::endl;
    oss << "Wall time: " << wt << " ms, (" << f/wt*1000 << " fps)" << std::endl;
    oss << "Process time: " << pt << " ms, (" << f/pt*1000 << " fps)" << std::endl;
    oss << "\tTexture loading: " << lt << " ms" << std::endl;
    oss << "\tRendering: " << rt << " ms" << std::endl;
    oss << "\tReading rendered image: " << it << " ms" << std::endl;
    return oss.str();
}
}
