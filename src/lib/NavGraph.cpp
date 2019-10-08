#include <iostream>
#include <fstream>
#include <iterator>
#include <opencv2/opencv.hpp>

#include <json/json.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "NavGraph.hpp"

namespace mattersim {


NavGraph::Location::Location(const Json::Value& viewpoint, const std::string& skyboxDir, 
        bool preload, bool depth): skyboxDir(skyboxDir), im_loaded(false), 
                                   includeDepth(depth), cubemap_texture(0), depth_texture(0) {

    viewpointId = viewpoint["image_id"].asString();
    included = viewpoint["included"].asBool();

    float posearr[16];
    int i = 0;
    for (auto f : viewpoint["pose"]) {
        posearr[i++] = f.asFloat();
    }
    // glm uses column-major order. Inputs are in row-major order.
    rot = glm::transpose(glm::make_mat4(posearr));
    // glm access is col,row
    pos = glm::vec3{rot[3][0], rot[3][1], rot[3][2]};
    rot[3] = {0,0,0,1}; // remove translation component
    
    for (auto u : viewpoint["unobstructed"]) {
        unobstructed.push_back(u.asBool());
    }

    if (preload) {
        // Preload skybox images
        loadCubemapImages();
    }
};


void NavGraph::Location::loadCubemapImages() {
    cv::Mat rgb = cv::imread(skyboxDir + viewpointId + "_skybox_small.jpg");
    int w = rgb.cols/6;
    int h = rgb.rows;
    xpos = rgb(cv::Rect(2*w, 0, w, h));
    xneg = rgb(cv::Rect(4*w, 0, w, h));
    ypos = rgb(cv::Rect(0*w, 0, w, h));
    yneg = rgb(cv::Rect(5*w, 0, w, h));
    zpos = rgb(cv::Rect(1*w, 0, w, h));
    zneg = rgb(cv::Rect(3*w, 0, w, h));
    if (xpos.empty() || xneg.empty() || ypos.empty() || yneg.empty() || zpos.empty() || zneg.empty()) {
        throw std::invalid_argument( "MatterSim: Could not open skybox RGB files at: " + skyboxDir + viewpointId + "_skybox_small.jpg");
    }
    if (includeDepth) {
        // 16 bit grayscale images
        cv::Mat depth = cv::imread(skyboxDir + viewpointId + "_skybox_depth_small.png", CV_LOAD_IMAGE_ANYDEPTH);
        w = depth.cols/6;
        h = depth.rows;
        xposD = depth(cv::Rect(2*w, 0, w, h));
        xnegD = depth(cv::Rect(4*w, 0, w, h));
        yposD = depth(cv::Rect(0*w, 0, w, h));
        ynegD = depth(cv::Rect(5*w, 0, w, h));
        zposD = depth(cv::Rect(1*w, 0, w, h));
        znegD = depth(cv::Rect(3*w, 0, w, h));
        if (xposD.empty() || xnegD.empty() || yposD.empty() || ynegD.empty() || zposD.empty() || znegD.empty()) {
            throw std::invalid_argument( "MatterSim: Could not open skybox depth files at: " + skyboxDir + viewpointId + "_skybox_depth_small.png");
        }
    }
    im_loaded = true;
}


void NavGraph::Location::loadCubemapTextures() {
    // RGB texture
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glGenTextures(1, &cubemap_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
    assertOpenGLError("RGB texture");
    if (includeDepth) {
        // Depth Texture
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glGenTextures(1, &depth_texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depth_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        //use fast 4-byte alignment (default anyway) if possible
        glPixelStorei(GL_UNPACK_ALIGNMENT, (xnegD.step & 3) ? 1 : 4);
        //set length of one complete row in data (doesn't need to equal image.cols)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, xnegD.step/xnegD.elemSize());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RED, xposD.rows, xposD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, xposD.ptr());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RED, xnegD.rows, xnegD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, xnegD.ptr());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RED, yposD.rows, yposD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, yposD.ptr());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RED, ynegD.rows, ynegD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, ynegD.ptr());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RED, zposD.rows, zposD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, zposD.ptr());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RED, znegD.rows, znegD.cols, 0, GL_RED, GL_UNSIGNED_SHORT, znegD.ptr());
        assertOpenGLError("Depth texture");
    }
}


void NavGraph::Location::deleteCubemapTextures() {
    // no need to check existence, silently ignores errors
    glDeleteTextures(1, &cubemap_texture);
    glDeleteTextures(1, &depth_texture);
    cubemap_texture = 0;
    depth_texture = 0;
}


std::pair<GLuint, GLuint> NavGraph::Location::cubemapTextures() {
    if (glIsTexture(cubemap_texture)){
        return {cubemap_texture, depth_texture}; 
    }
    if (!im_loaded) {
        loadCubemapImages();
    }
    loadCubemapTextures();
    return {cubemap_texture, depth_texture};
}


NavGraph::NavGraph(const std::string& navGraphPath, const std::string& datasetPath, 
              bool preloadImages, bool renderDepth, int randomSeed, unsigned int cacheSize) : cache(cacheSize) {

    generator.seed(randomSeed);

    auto textFile = navGraphPath + "/scans.txt";
    std::ifstream scansFile(textFile);
    if (scansFile.fail()){
        throw std::invalid_argument( "MatterSim: Could not open list of scans at: " +
                textFile + ", is path valid?" );
    }
    std::vector<std::string> scanIds;
    std::copy(std::istream_iterator<std::string>(scansFile),
          std::istream_iterator<std::string>(),
          std::back_inserter(scanIds));

    #pragma omp parallel for
    for (unsigned int i=0; i<scanIds.size(); i++) {
        std::string scanId = scanIds.at(i);
        Json::Value root;
        auto navGraphFile =  navGraphPath + "/" + scanId + "_connectivity.json";
        std::ifstream ifs(navGraphFile, std::ifstream::in);
        if (ifs.fail()){
            throw std::invalid_argument( "MatterSim: Could not open navigation graph file: " +
                    navGraphFile + ", is path valid?" );
        }
        ifs >> root;
        auto skyboxDir = datasetPath + "/" + scanId + "/matterport_skybox_images/";
        #pragma omp critical
        {
            scanLocations.insert(std::pair<std::string, 
                    std::vector<LocationPtr> > (scanId, std::vector<LocationPtr>()));
        }
        for (auto viewpoint : root) {
            Location l(viewpoint, skyboxDir, preloadImages, renderDepth);
            #pragma omp critical
            {
                scanLocations[scanId].push_back(std::make_shared<Location>(l));
            }
        }
    }
}


NavGraph::~NavGraph() {
    // free all remaining textures
    for (auto scan : scanLocations) {
        for (auto loc : scan.second) {
            loc->deleteCubemapTextures();
        }
    }
}


NavGraph& NavGraph::getInstance(const std::string& navGraphPath, const std::string& datasetPath, 
                bool preloadImages, bool renderDepth, int randomSeed, unsigned int cacheSize){
    // magic static
    static NavGraph instance(navGraphPath, datasetPath, preloadImages, renderDepth, randomSeed, cacheSize);
    return instance;
}


const std::string& NavGraph::randomViewpoint(const std::string& scanId) {
    std::uniform_int_distribution<int> distribution(0,scanLocations.at(scanId).size()-1);
    int start_ix = distribution(generator);  // generates random starting index
    int ix = start_ix;
    while (!scanLocations.at(scanId).at(ix)->included) { // Don't start at an excluded viewpoint
        ix++;
        if (ix >= scanLocations.at(scanId).size()) ix = 0;
        if (ix == start_ix) {
            throw std::logic_error( "MatterSim: ScanId: " + scanId + " has no included viewpoints!");
        }
    }
    return scanLocations.at(scanId).at(ix)->viewpointId;
}


unsigned int NavGraph::index(const std::string& scanId, const std::string& viewpointId) const {
    int ix = -1;
    for (int i = 0; i < scanLocations.at(scanId).size(); ++i) {
        if (scanLocations.at(scanId).at(i)->viewpointId == viewpointId) {
            if (!scanLocations.at(scanId).at(i)->included) {
                throw std::invalid_argument( "MatterSim: ViewpointId: " +
                        viewpointId + ", is excluded from the connectivity graph." );
            }
            ix = i;
            break;
        }
    }
    if (ix < 0) {
        throw std::invalid_argument( "MatterSim: Could not find viewpointId: " +
                viewpointId + ", is viewpoint id valid?" );
    } else {
        return ix;
    }
}

const std::string& NavGraph::viewpoint(const std::string& scanId, unsigned int ix) const {
    return scanLocations.at(scanId).at(ix)->viewpointId;
}


const glm::mat4& NavGraph::cameraRotation(const std::string& scanId, unsigned int ix) const {
    return scanLocations.at(scanId).at(ix)->rot;
}


const glm::vec3& NavGraph::cameraPosition(const std::string& scanId, unsigned int ix) const {
    return scanLocations.at(scanId).at(ix)->pos;
}


std::vector<unsigned int> NavGraph::adjacentViewpointIndices(const std::string& scanId, unsigned int ix) const {
    std::vector<unsigned int> reachable;
    for (unsigned int i = 0; i < scanLocations.at(scanId).size(); ++i) {
        if (i == ix) {
            // Skip option to stay at the same viewpoint
            continue;
        }
        if (scanLocations.at(scanId).at(ix)->unobstructed[i] && scanLocations.at(scanId).at(i)->included) {
            reachable.push_back(i);
        }
    }
    return reachable;
}


std::pair<GLuint, GLuint> NavGraph::cubemapTextures(const std::string& scanId, unsigned int ix) {
    LocationPtr loc = scanLocations.at(scanId).at(ix);
    std::pair<GLuint, GLuint> textures = loc->cubemapTextures();
    cache.add(loc);
    return textures;
}


void NavGraph::deleteCubemapTextures(const std::string& scanId, unsigned int ix) {
    scanLocations.at(scanId).at(ix)->deleteCubemapTextures();
}


}
