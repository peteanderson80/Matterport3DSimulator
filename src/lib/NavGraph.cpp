#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

#include <json/json.h>
#include "NavGraph.hpp"

namespace mattersim {


NavGraph::Location::Location(const Json::Value& viewpoint, const std::string& skyboxDir, 
        bool preload): skyboxDir(skyboxDir), im_loaded(false), cubemap_texture(0) {

    viewpointId = viewpoint["image_id"].asString();
    included = viewpoint["included"].asBool();

    float posearr[16];
    int i = 0;
    for (auto f : viewpoint["pose"]) {
        posearr[i++] = f.asFloat();
    }
    // glm uses column-major order. Inputs are in row-major order.
    glm::mat4 mattPose = glm::transpose(glm::make_mat4(posearr));
    // glm access is col,row
    pos = glm::vec3{mattPose[3][0], mattPose[3][1], mattPose[3][2]};
    mattPose[3] = {0,0,0,1}; // remove translation component
    // Matterport camera looks down z axis. Opengl camera looks down -z axis. Rotate around x by 180 deg.
    rot = glm::rotate(mattPose, (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
    
    for (auto u : viewpoint["unobstructed"]) {
        unobstructed.push_back(u.asBool());
    }

    if (preload) {
        // Preload skybox images
        loadCubemapImages();
    }
};


void NavGraph::Location::loadCubemapImages() {
    xpos = cv::imread(skyboxDir + viewpointId + "_skybox2_sami.jpg");
    xneg = cv::imread(skyboxDir + viewpointId + "_skybox4_sami.jpg");
    ypos = cv::imread(skyboxDir + viewpointId + "_skybox0_sami.jpg");
    yneg = cv::imread(skyboxDir + viewpointId + "_skybox5_sami.jpg");
    zpos = cv::imread(skyboxDir + viewpointId + "_skybox1_sami.jpg");
    zneg = cv::imread(skyboxDir + viewpointId + "_skybox3_sami.jpg");
    if (xpos.empty() || xneg.empty() || ypos.empty() || yneg.empty() || zpos.empty() || zneg.empty()) {
        throw std::invalid_argument( "MatterSim: Could not open skybox files at: " + skyboxDir + viewpointId + "_skybox*_sami.jpg");
    }
    im_loaded = true;
}


void NavGraph::Location::loadCubemapTexture() {
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
}


void NavGraph::Location::deleteCubemapTexture() {
    glDeleteTextures(1, &cubemap_texture); // no need to check existence, silently ignores errors
}


GLuint NavGraph::Location::cubemapTexture() {
    if (glIsTexture(cubemap_texture)){
        return cubemap_texture; 
    }
    if (!im_loaded) {
        loadCubemapImages();
    }
    loadCubemapTexture();
    return cubemap_texture;
}


NavGraph::NavGraph(const std::string& navGraphPath, const std::string& datasetPath, 
                    bool preloadImages, int randomSeed) {

    generator.seed(randomSeed);

    auto textFile = navGraphPath + "/scans.txt";
    std::ifstream scansFile(textFile);
    if (scansFile.fail()){
        throw std::invalid_argument( "MatterSim: Could not open list of scans at: " +
                textFile + ", is path valid?" );
    }
    std::string scanId;
    while (std::getline(scansFile, scanId)){
        Json::Value root;
        auto navGraphFile =  navGraphPath + "/" + scanId + "_connectivity.json";
        std::ifstream ifs(navGraphFile, std::ifstream::in);
        if (ifs.fail()){
            throw std::invalid_argument( "MatterSim: Could not open navigation graph file: " +
                    navGraphFile + ", is path valid?" );
        }
        scanLocations.insert(std::pair<std::string, 
                std::vector<LocationPtr> > (scanId, std::vector<LocationPtr>()));
        ifs >> root;
        auto skyboxDir = datasetPath + "/" + scanId + "/matterport_skybox_images/";
        for (auto viewpoint : root) {
            Location l(viewpoint, skyboxDir, preloadImages);
            scanLocations[scanId].push_back(std::make_shared<Location>(l));
        }
    }
}


NavGraph::~NavGraph() {
    // free all remaining textures
    for (auto scan : scanLocations) {
        for (auto loc : scan.second) {
            loc->deleteCubemapTexture();
        }
    }
}


NavGraph& NavGraph::getInstance(const std::string& navGraphPath, const std::string& datasetPath, 
                bool preloadImages, int randomSeed){
    // magic static
    static NavGraph instance(navGraphPath, datasetPath, preloadImages, randomSeed);
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


GLuint NavGraph::cubemapTexture(const std::string& scanId, unsigned int ix) {
    return scanLocations.at(scanId).at(ix)->cubemapTexture();
}


void NavGraph::deleteCubemapTexture(const std::string& scanId, unsigned int ix) {
    scanLocations.at(scanId).at(ix)->deleteCubemapTexture();
}


}
