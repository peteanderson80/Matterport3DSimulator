#include <pybind11/pybind11.h>
#include <numpy/ndarrayobject.h>
#include <numpy/npy_math.h>
#include <iostream>
#include "MatterSim.hpp"

namespace py = pybind11;

namespace mattersim {
    class ViewPointPython {
    public:
        ViewPointPython(ViewpointPtr locptr) {
            viewpointId = locptr->viewpointId;
            ix = locptr->ix;
            point.append(locptr->point.x);
            point.append(locptr->point.y);
            point.append(locptr->point.z);
            rel_heading = locptr->rel_heading;
            rel_elevation = locptr->rel_elevation;
            rel_distance = locptr->rel_distance;
        }
        std::string viewpointId;
        unsigned int ix;
        py::list point;
        double rel_heading;
        double rel_elevation;
        double rel_distance;
    };

    class SimStatePython {
    public:
        SimStatePython(SimStatePtr state, bool renderingEnabled)
            : step{state->step},
              viewIndex{state->viewIndex},
              location{state->location},
              heading{state->heading},
              elevation{state->elevation} {
            if (renderingEnabled) {
                npy_intp colorShape[3] {state->rgb.rows, state->rgb.cols, 3};
                rgb = matToNumpyArray(3, colorShape, NPY_UBYTE, (void*)state->rgb.data);
            }
            scanId = state->scanId;
            for (auto viewpoint : state->navigableLocations) {
                navigableLocations.append(ViewPointPython{viewpoint});
            }
        }
        std::string scanId;
        unsigned int step;
        unsigned int viewIndex;
        py::object rgb;
        ViewPointPython location;
        double heading;
        double elevation;
        py::list navigableLocations;
    private:
        py::object matToNumpyArray(int dims, npy_intp *shape, int type, void *data) {
            //colorDims, this->colorShape, NPY_UBYTE, this->state->screenBuffer->data());
            PyObject *pyArray = PyArray_SimpleNewFromData(dims, shape, type, data);
            /* This line makes a copy: */
            PyObject *pyArrayCopied = PyArray_FROM_OTF(pyArray, type, NPY_ARRAY_ENSURECOPY | NPY_ARRAY_ENSUREARRAY);
            /* And this line gets rid of the old object which caused a memory leak: */
            Py_DECREF(pyArray);

            py::handle numpyArrayHandle = py::handle(pyArrayCopied);
            py::object numpyArray = py::reinterpret_steal<py::object>(numpyArrayHandle);

            return numpyArray;
        }
    };
    #if PY_MAJOR_VERSION >= 3
        void* init_numpy() {
            import_array();
            return nullptr;
        }
    #else
        void init_numpy() {
            import_array();
        }
    #endif
    class SimulatorPython {
    public:
        SimulatorPython() {
    init_numpy();
        }
        void setDatasetPath(std::string path) {
            sim.setDatasetPath(path);
        }
        void setNavGraphPath(std::string path) {
            sim.setNavGraphPath(path);
        }
        void setCameraResolution(int width, int height) {
            sim.setCameraResolution(width, height);
        }
        void setCameraVFOV(double vfov) {
            sim.setCameraVFOV(vfov);
        }
        void setRenderingEnabled(bool value){
            sim.setRenderingEnabled(value);
        }
        void setDiscretizedViewingAngles(bool value){
            sim.setDiscretizedViewingAngles(value);
        }
        void init() {
            sim.init();
        }
        void setSeed(int seed) {
            sim.setSeed(seed);
        }
        bool setElevationLimits(double min, double max) {
            return sim.setElevationLimits(min, max);
        }
        void newEpisode(const std::string& scanId, const std::string& viewpointId=std::string(), 
              double heading=0, double elevation=0) {
            sim.newEpisode(scanId, viewpointId, heading, elevation);
        }
        SimStatePython *getState() {
            return new SimStatePython(sim.getState(), sim.renderingEnabled);
        }
        void makeAction(int index, double heading, double elevation) {
            sim.makeAction(index, heading, elevation);
        }
        void close() {
            sim.close();
        }
    private:
        Simulator sim;
    };
}

using namespace mattersim;

PYBIND11_MODULE(MatterSim, m) {
    py::class_<ViewPointPython>(m, "ViewPoint")
        .def_readonly("viewpointId", &ViewPointPython::viewpointId)
        .def_readonly("ix", &ViewPointPython::ix)
        .def_readonly("point", &ViewPointPython::point)
        .def_readonly("rel_heading", &ViewPointPython::rel_heading)
        .def_readonly("rel_elevation", &ViewPointPython::rel_elevation)
        .def_readonly("rel_distance", &ViewPointPython::rel_distance);
    py::class_<SimStatePython>(m, "SimState")
        .def_readonly("scanId", &SimStatePython::scanId)
        .def_readonly("step", &SimStatePython::step)
        .def_readonly("rgb", &SimStatePython::rgb)
        .def_readonly("location", &SimStatePython::location)
        .def_readonly("heading", &SimStatePython::heading)
        .def_readonly("elevation", &SimStatePython::elevation)
        .def_readonly("viewIndex", &SimStatePython::viewIndex)
        .def_readonly("navigableLocations", &SimStatePython::navigableLocations);
    py::class_<SimulatorPython>(m, "Simulator")
        .def(py::init<>())
        .def("setDatasetPath", &SimulatorPython::setDatasetPath)
        .def("setNavGraphPath", &SimulatorPython::setNavGraphPath)
        .def("setCameraResolution", &SimulatorPython::setCameraResolution)
        .def("setCameraVFOV", &SimulatorPython::setCameraVFOV)
        .def("setRenderingEnabled", &SimulatorPython::setRenderingEnabled)
        .def("setDiscretizedViewingAngles", &SimulatorPython::setDiscretizedViewingAngles)
        .def("init", &SimulatorPython::init)
        .def("setSeed", &SimulatorPython::setSeed)
        .def("setElevationLimits", &SimulatorPython::setElevationLimits)
        .def("newEpisode", &SimulatorPython::newEpisode)
        .def("getState", &SimulatorPython::getState, py::return_value_policy::take_ownership)
        .def("makeAction", &SimulatorPython::makeAction)
        .def("close", &SimulatorPython::close);
}


