#include "rrtDemo.h"
#include "graph.h"
#include "rrt.h"
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // Automatic conversion for std::vector, std::tuple, etc.
#include <vector>
#include <tuple>
#include <algorithm>
#include <utility>

namespace py = pybind11;

namespace rrt
{
using pose_t = std::tuple<double, double, double, double>; // x, y, time, heading

VisRRT::VisRRT()
    : rrt_(new RRT(
        std::vector<RRT::occupancy_t>(), // empty occupancy map
        std::get<0>(pose_t(-5.0, -5.0, 0.0, 0.0)), std::get<1>(pose_t(-5.0, -5.0, 0.0, 0.0)),
        std::get<0>(pose_t(5.0, 5.0, 10.0, 0.0)), std::get<1>(pose_t(5.0, 5.0, 10.0, 0.0)),
        std::get<0>(pose_t(0.0, 0.0, 0.0, 0.7854)), std::get<1>(pose_t(0.0, 0.0, 0.0, 0.7854)),
        std::get<0>(pose_t(5.0, 5.0, 10.0, 0.0)), std::get<1>(pose_t(5.0, 5.0, 10.0, 0.0)),
        0.8, 1.0, 0.5, 2.0, // constraints
        10.0, // max_time
        true, // dim_3D
        10000, // node_limit
        1      // max_admissible (default value)
    ))
{}

VisRRT::VisRRT(std::vector<RRT::occupancy_t> _occupancy_map,
    pose_t _range_a, pose_t _range_b,
    pose_t _origin, pose_t _dest,
    double _max_angle_rad, double _max_dist,
    double _min_dist, double _max_interval,
    double _max_time, bool _dim_3D, int _iteration_limit,
    int _max_admissible)
    : rrt_(new RRT(_occupancy_map,
        _range_a, _range_b, _origin, _dest,
        _max_angle_rad, _max_dist, _min_dist, _max_interval,
        _max_time, _dim_3D, _iteration_limit, _max_admissible))
{
}

VisRRT::VisRRT(pose_t _range_a, pose_t _range_b,
    pose_t _origin, pose_t _dest,
    double _max_angle_rad, double _max_dist,
    double _min_dist, double _max_interval,
    double _max_time, bool _dim_3D, int _iteration_limit,
    int _max_admissible)
    : rrt_(new RRT(_range_a, _range_b, _origin, _dest,
        _max_angle_rad, _max_dist, _min_dist, _max_interval,
        _max_time, _dim_3D, _iteration_limit, _max_admissible))
{
}

VisRRT::~VisRRT() = default;

void VisRRT::buildRRT() {
    rrt_->buildRRT();
}

bool VisRRT::stepRRT() {
    return rrt_->stepRRT();
}

// Optimized: pose_t-based initializeRRT (12 args; extracts x/y for time horizon, includes initial_heading)
void VisRRT::initializeRRT(
    pose_t _range_a, pose_t _range_b,
    pose_t _origin, pose_t _dest,
    double _max_angle_rad, double _max_dist,
    double _min_dist, double _max_interval,
    double _max_time, bool _dim_3D, int _iteration_limit,
    double _initial_heading)
{
    // Extract x/y for boundaries + use _max_time as horizon
    double range_a_x = std::get<0>(_range_a);
    double range_a_y = std::get<1>(_range_a);
    double range_b_x = std::get<0>(_range_b);
    double range_b_y = std::get<1>(_range_b);
    rrt_->setBoundaries(range_a_x, range_a_y, range_b_x, range_b_y, _max_time);
    rrt_->setOrigin(std::get<0>(_origin), std::get<1>(_origin), std::get<2>(_origin));
    rrt_->updateDestination(std::get<0>(_dest), std::get<1>(_dest), std::get<2>(_dest));
    rrt_->updateConstraints(_max_angle_rad, _max_dist, _min_dist, _max_interval);
    rrt_->setDim3D(_dim_3D);
    rrt_->setIterationLimit(_iteration_limit);
}

void VisRRT::setBoundaries(pose_t _range_a, pose_t _range_b)
{
    rrt_->setBoundaries(_range_a, _range_b);
}

void VisRRT::setOrigin(pose_t _origin)
{
    rrt_->setOrigin(std::get<0>(_origin), std::get<1>(_origin), std::get<2>(_origin));
}

void VisRRT::updateDestination(pose_t _dest)
{
    rrt_->updateDestination(std::get<0>(_dest), std::get<1>(_dest), std::get<2>(_dest));
}

void VisRRT::updateConstraints(double _max_angle_rad, double _max_dist,
    double _min_dist, double _max_interval)
{
    rrt_->updateConstraints(_max_angle_rad, _max_dist, _min_dist, _max_interval);
}

void VisRRT::setDim3D(bool _dim_3D)
{
    rrt_->setDim3D(_dim_3D);
}

void VisRRT::setIterationLimit(int _iteration_limit)
{
    rrt_->setIterationLimit(_iteration_limit);
}

void VisRRT::setOccupancyMap(
    std::vector<std::vector<double>> _occp_coords,
    std::vector<double> _occp_widths,
    std::vector<double> _occp_interval)
{
    std::vector<RRT::occupancy_t> occupancy_map;
    for (size_t i = 0; i < _occp_coords.size(); ++i) {
        RRT::occupancy_t occ;
        occ.first = std::make_tuple(_occp_coords[i][0], _occp_coords[i][1], _occp_interval[i], 0.0);
        occ.second = _occp_widths[i];
        occupancy_map.push_back(occ);
    }
    rrt_->setOccupancyMap(occupancy_map);
}

int VisRRT::getNodeCount()
{
    return rrt_->adjacencyList_.size();
}

double VisRRT::getNodeX(int idx)
{
    Node* n = getNodeAt(idx);
    if(n) return n->xCrdnt();
    return 0.0;
}

double VisRRT::getNodeY(int idx)
{
    Node* n = getNodeAt(idx);
    if(n) return n->yCrdnt();
    return 0.0;
}

double VisRRT::getNodeTime(int idx)
{
    Node* n = getNodeAt(idx);
    if(n) return n->time();
    return 0.0;
}

bool VisRRT::isComplete()
{
    return rrt_->isComplete();
}

bool VisRRT::isAdmissible(Node* node)
{
    return rrt_->isAdmissible();
}

void VisRRT::updateInitialHeading(double _initial_heading)
{
    rrt_->updateInitialHeading(_initial_heading);
}

Node* VisRRT::getNodeAt(int idx)
{
    if (idx < static_cast<int>(rrt_->adjacencyList_.size())) {
        return rrt_->adjacencyList_[idx];
    }
    return nullptr;
}

// UPDATED: Return py::list for seamless Python conversion
py::list VisRRT::getForwardIndices(int idx)
{
    Node* node = getNodeAt(idx);
    if (!node) {
        return py::list();
    }
    py::list py_list;
    const std::vector<Node*>& fwd_nodes = node->getFwdNodes();
    for (Node* fwd : fwd_nodes) {
        int fwd_idx = rrt_->getIndex(fwd);
        if (fwd_idx >= 0) { // Assuming getIndex returns -1 or similar for invalid
            py_list.append(fwd_idx);
        }
    }
    return py_list;
}

} // namespace rrt

PYBIND11_MODULE(rrtDemo, m) {
    using namespace rrt;

    py::class_<Node, std::unique_ptr<Node, py::nodelete>>(m, "Node")  // Use py::nodelete for raw pointers
        .def("xCrdnt", &Node::xCrdnt)
        .def("yCrdnt", &Node::yCrdnt)
        .def("heading", &Node::heading)
        .def("time", &Node::time)
        .def("backEdgeWeight", &Node::backEdgeWeight);

    py::class_<VisRRT>(m, "RRT")
        .def(py::init<>()) // Default
        .def(py::init<std::vector<RRT::occupancy_t>, pose_t, pose_t, pose_t, pose_t, double, double, double, double, double, bool, int, int>())
        .def(py::init<pose_t, pose_t, pose_t, pose_t, double, double, double, double, double, bool, int, int>())
        .def("buildRRT", &VisRRT::buildRRT)
        .def("stepRRT", &VisRRT::stepRRT)
        .def("initializeRRT", &VisRRT::initializeRRT)
        .def("setBoundaries", &VisRRT::setBoundaries)
        .def("setOrigin", &VisRRT::setOrigin)
        .def("updateDestination", &VisRRT::updateDestination)
        .def("updateConstraints", &VisRRT::updateConstraints)
        .def("setDim3D", &VisRRT::setDim3D)
        .def("setIterationLimit", &VisRRT::setIterationLimit)
        .def("setOccupancyMap", &VisRRT::setOccupancyMap)
        .def("isComplete", &VisRRT::isComplete)
        .def("getNodeCount", &VisRRT::getNodeCount)
        .def("getNodeAt", &VisRRT::getNodeAt, py::return_value_policy::reference)
        .def("getForwardIndices", &VisRRT::getForwardIndices)
        .def("getNodeX", &VisRRT::getNodeX)
        .def("getNodeY", &VisRRT::getNodeY)
        .def("getNodeTime", &VisRRT::getNodeTime)
        .def("isAdmissible", &VisRRT::isAdmissible)
        .def("updateInitialHeading", &VisRRT::updateInitialHeading);
}