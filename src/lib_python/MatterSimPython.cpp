#include <pybind11/pybind11.h>
#include <numpy/ndarrayobject.h>
#include <numpy/npy_math.h>
#include <iostream>
#include "MatterSim.hpp"

namespace py = pybind11;

namespace mattersim {
    class ViewPointPython {
    public:
        unsigned int id;
        //cv::Point3f location;
    };

    class SimStatePython {
    public:
        SimStatePython(SimStatePtr state) {
            npy_intp colorShape[3] {state->rgb.rows, state->rgb.cols, 3};

            rgb = matToNumpyArray(3, colorShape, NPY_UBYTE, (void*)state->rgb.data);
            for (auto viewpoint : state->navigableLocations) {
                navigableLocations.append(ViewPointPython{viewpoint->id});
            }
        }
        py::object rgb;
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
        void setScreenResolution(int width, int height) {
            sim.setScreenResolution(width, height);
        }
        void setScanId(std::string id) {
            sim.setScanId(id);
        }
        void init() {
            sim.init();
        }
        void newEpisode() {
            sim.newEpisode();
        }
        bool isEpisodeFinished() {
            return sim.isEpisodeFinished();
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
    py::class_<ViewPointPython>(m, "ViewPoint");
    py::class_<SimStatePython>(m, "SimState")
        .def_readonly("rgb", &SimStatePython::rgb)
        .def_readonly("navigableLocations", &SimStatePython::navigableLocations);
    py::class_<SimulatorPython>(m, "Simulator")
        .def(py::init<>())
        .def("setDatasetPath", &SimulatorPython::setDatasetPath)
        .def("setNavGraphPath", &SimulatorPython::setNavGraphPath)
        .def("setScreenResolution", &SimulatorPython::setScreenResolution)
        .def("setScanId", &SimulatorPython::setScanId)
        .def("init", &SimulatorPython::init)
        .def("newEpisode", &SimulatorPython::newEpisode)
        .def("isEpisodeFinished", &SimulatorPython::isEpisodeFinished)
        .def("getState", &SimulatorPython::getState, py::return_value_policy::take_ownership)
        .def("makeAction", &SimulatorPython::makeAction)
        .def("close", &SimulatorPython::close);
}


