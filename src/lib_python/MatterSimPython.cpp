#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "MatterSim.hpp"

namespace py = pybind11;
using namespace mattersim;

PYBIND11_MODULE(MatterSim, m) {
    py::class_<Viewpoint, ViewpointPtr>(m, "ViewPoint")
        .def_readonly("viewpointId", &Viewpoint::viewpointId)
        .def_readonly("ix", &Viewpoint::ix)
        .def_readonly("x", &Viewpoint::x)
        .def_readonly("y", &Viewpoint::y)
        .def_readonly("z", &Viewpoint::z)
        .def_readonly("rel_heading", &Viewpoint::rel_heading)
        .def_readonly("rel_elevation", &Viewpoint::rel_elevation)
        .def_readonly("rel_distance", &Viewpoint::rel_distance);
    py::class_<cv::Mat>(m, "Image", pybind11::buffer_protocol())
        .def_buffer([](cv::Mat& im) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                im.data, // Pointer to buffer
                sizeof(unsigned char), // Size of one scalar
                pybind11::format_descriptor<unsigned char>::format(),
                3, // Number of dimensions
                { im.rows, im.cols, im.channels() }, // Buffer dimensions
                {   // Strides (in bytes) for each index
                    sizeof(unsigned char) * im.channels() * im.cols,
                    sizeof(unsigned char) * im.channels(),
                    sizeof(unsigned char)
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
        .def("setCameraResolution", &Simulator::setCameraResolution)
        .def("setCameraVFOV", &Simulator::setCameraVFOV)
        .def("setRenderingEnabled", &Simulator::setRenderingEnabled)
        .def("setDiscretizedViewingAngles", &Simulator::setDiscretizedViewingAngles)
        .def("initialize", &Simulator::initialize)
        .def("setSeed", &Simulator::setSeed)
        .def("setElevationLimits", &Simulator::setElevationLimits)
        .def("newEpisode", &Simulator::newEpisode)
        .def("getState", &Simulator::getState, py::return_value_policy::reference)
        .def("makeAction", &Simulator::makeAction)
        .def("close", &Simulator::close)
        .def("resetTimers", &Simulator::resetTimers)
        .def("timingInfo", &Simulator::timingInfo);
}
