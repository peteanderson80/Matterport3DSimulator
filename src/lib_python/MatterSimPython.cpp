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
            id = locptr->id;
            location.append(locptr->location.x);
            location.append(locptr->location.y);
            location.append(locptr->location.z);
        }
        unsigned int id;
        py::list location;
    };

    class SimStatePython {
    public:
        SimStatePython(SimStatePtr state) : step{state->step},
                                            location{state->location},
                                            heading{state->heading},
                                            elevation{state->elevation} {
            npy_intp colorShape[3] {state->rgb.rows, state->rgb.cols, 3};
            rgb = matToNumpyArray(3, colorShape, NPY_UBYTE, (void*)state->rgb.data);

            for (auto viewpoint : state->navigableLocations) {
                navigableLocations.append(ViewPointPython{viewpoint});
            }
        }
        unsigned int step;
        py::object rgb;
        ViewPointPython location;
        float heading;
        float elevation;
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
        void setCameraFOV(float vfov) {
            sim.setCameraFOV(vfov);
        }
        void init() {
            sim.init();
        }
        void setSeed(int seed) {
            sim.setSeed(seed);
        }
        void setElevationLimits(float min, float max) {
            sim.setElevationLimits(min, max);
        }
        void newEpisode(const std::string& scanId, const std::string& viewpointId=std::string(), 
              float heading=0, float elevation=0) {
            sim.newEpisode(scanId, viewpointId, heading, elevation);
        }
        SimStatePython *getState() {
            return new SimStatePython(sim.getState());
        }
        void makeAction(int index, float heading, float elevation) {
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
        .def_readonly("id", &ViewPointPython::id)
        .def_readonly("location", &ViewPointPython::location);
    py::class_<SimStatePython>(m, "SimState")
        .def_readonly("step", &SimStatePython::step)
        .def_readonly("rgb", &SimStatePython::rgb)
        .def_readonly("location", &SimStatePython::location)
        .def_readonly("heading", &SimStatePython::heading)
        .def_readonly("elevation", &SimStatePython::elevation)
        .def_readonly("navigableLocations", &SimStatePython::navigableLocations);
    py::class_<SimulatorPython>(m, "Simulator")
        .def(py::init<>())
        .def("setDatasetPath", &SimulatorPython::setDatasetPath)
        .def("setNavGraphPath", &SimulatorPython::setNavGraphPath)
        .def("setCameraResolution", &SimulatorPython::setCameraResolution)
        .def("setCameraFOV", &SimulatorPython::setCameraFOV)
        .def("init", &SimulatorPython::init)
        .def("setSeed", &SimulatorPython::setSeed)
        .def("setElevationLimits", &SimulatorPython::setElevationLimits)
        .def("newEpisode", &SimulatorPython::newEpisode)
        .def("getState", &SimulatorPython::getState, py::return_value_policy::take_ownership)
        .def("makeAction", &SimulatorPython::makeAction)
        .def("close", &SimulatorPython::close);
}


