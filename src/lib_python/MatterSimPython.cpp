#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "MatterSim.hpp"
#include "cbf.h"

namespace py = pybind11;

namespace mattersim {

    void cbf(py::buffer depth, py::buffer intensity, py::buffer mask, py::buffer result) {
        double spaceSigmas[3] = {12, 5, 8};
        double rangeSigmas[3] = {0.2, 0.08, 0.02};
        py::buffer_info d_info = depth.request();
        py::buffer_info i_info = intensity.request();
        py::buffer_info m_info = mask.request();
        py::buffer_info r_info = result.request();
        cbf::cbf(d_info.shape[0], d_info.shape[1],
            static_cast<uint8_t*>(d_info.ptr),
            static_cast<uint8_t*>(i_info.ptr),
            static_cast<uint8_t*>(m_info.ptr),
            static_cast<uint8_t*>(r_info.ptr),
            3, &spaceSigmas[0], &rangeSigmas[0]);
    }

}

using namespace mattersim;

PYBIND11_MODULE(MatterSim, m) {
    m.def("cbf", &mattersim::cbf, "Cross Bilateral Filter");
    py::class_<Viewpoint, ViewpointPtr>(m, "ViewPoint")
        .def_readonly("viewpointId", &Viewpoint::viewpointId)
        .def_readonly("ix", &Viewpoint::ix)
        .def_readonly("x", &Viewpoint::x)
        .def_readonly("y", &Viewpoint::y)
        .def_readonly("z", &Viewpoint::z)
        .def_readonly("rel_heading", &Viewpoint::rel_heading)
        .def_readonly("rel_elevation", &Viewpoint::rel_elevation)
        .def_readonly("rel_distance", &Viewpoint::rel_distance);
    py::class_<cv::Mat>(m, "Mat", pybind11::buffer_protocol())
        .def_buffer([](cv::Mat& im) -> pybind11::buffer_info {
            ssize_t item_size = im.elemSize() / im.channels();
            std::string format = pybind11::format_descriptor<unsigned char>::format();
            if (item_size == 2) { // handle 16bit data from depth maps
                format = pybind11::format_descriptor<unsigned short>::format();
            }
            return pybind11::buffer_info(
                im.data, // Pointer to buffer
                item_size, // Size of one scalar
                format,
                3, // Number of dimensions (row, cols, channels)
                { im.rows, im.cols, im.channels() }, // Buffer dimensions
                {   // Strides (in bytes) for each index
                    item_size * im.channels() * im.cols,
                    item_size * im.channels(),
                    item_size
                }
            );
        });
    py::class_<SimState, SimStatePtr>(m, "SimState")
        .def_readonly("scanId", &SimState::scanId)
        .def_readonly("step", &SimState::step)
        .def_readonly("rgb", &SimState::rgb)
        .def_readonly("depth", &SimState::depth)
        .def_readonly("location", &SimState::location)
        .def_readonly("heading", &SimState::heading)
        .def_readonly("elevation", &SimState::elevation)
        .def_readonly("viewIndex", &SimState::viewIndex)
        .def_readonly("navigableLocations", &SimState::navigableLocations);
    py::class_<Simulator>(m, "Simulator")
        .def(py::init<>())
        .def("setDatasetPath", &Simulator::setDatasetPath)
        .def("setNavGraphPath", &Simulator::setNavGraphPath)
        .def("setRenderingEnabled", &Simulator::setRenderingEnabled)
        .def("setCameraResolution", &Simulator::setCameraResolution)
        .def("setCameraVFOV", &Simulator::setCameraVFOV)
        .def("setElevationLimits", &Simulator::setElevationLimits)
        .def("setDiscretizedViewingAngles", &Simulator::setDiscretizedViewingAngles)
        .def("setRestrictedNavigation", &Simulator::setRestrictedNavigation)
        .def("setPreloadingEnabled", &Simulator::setPreloadingEnabled)
        .def("setDepthEnabled", &Simulator::setDepthEnabled)
        .def("setBatchSize", &Simulator::setBatchSize)
        .def("setCacheSize", &Simulator::setCacheSize)
        .def("setSeed", &Simulator::setSeed)
        .def("initialize", &Simulator::initialize)
        .def("newEpisode", &Simulator::newEpisode)
        .def("newRandomEpisode", &Simulator::newRandomEpisode)
        .def("getState", &Simulator::getState, py::return_value_policy::take_ownership)
        .def("makeAction", &Simulator::makeAction)
        .def("close", &Simulator::close)
        .def("resetTimers", &Simulator::resetTimers)
        .def("timingInfo", &Simulator::timingInfo);
}
