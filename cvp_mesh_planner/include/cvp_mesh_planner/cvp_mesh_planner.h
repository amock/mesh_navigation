/*
 *  Copyright 2020, Sebastian Pütz
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  authors:
 *    Sebastian Pütz <spuetz@uni-osnabrueck.de>
 *
 */

#ifndef MESH_NAVIGATION__MESH_PLANNER_H
#define MESH_NAVIGATION__MESH_PLANNER_H

#include <mbf_mesh_core/mesh_planner.h>
#include <mbf_msgs/action/get_path.hpp>
#include <mesh_map/mesh_map.h>
#include <nav_msgs/msg/path.hpp>
#include <rclcpp/rclcpp.hpp>

namespace cvp_mesh_planner
{
class CVPMeshPlanner : public mbf_mesh_core::MeshPlanner
{
public:
  typedef std::shared_ptr<cvp_mesh_planner::CVPMeshPlanner> Ptr;

  /**
   * @brief Constructor
   */
  CVPMeshPlanner();

  /**
   * @brief Destructor
   */
  virtual ~CVPMeshPlanner();

  /**
   * @brief Compute a continuous vector field and geodesic path on the mesh's surface
   * @param start The start pose
   * @param goal The goal pose
   * @param tolerance The goal tolerance, TODO is currently not used
   * @param plan The computed plan
   * @param cost The computed cost for the plan
   * @param message a detailed outcome message
   * @return result outcome code, see the GetPath action definition
   */
  virtual uint32_t makePlan(const geometry_msgs::msg::PoseStamped& start, const geometry_msgs::msg::PoseStamped& goal,
                            double tolerance, std::vector<geometry_msgs::msg::PoseStamped>& plan, double& cost,
                            std::string& message) override;

  /**
   * @brief Requests the planner to cancel, e.g. if it takes too much time.
   * @return true if cancel has been successfully requested, false otherwise
   */
  virtual bool cancel() override;

  /**
   * @brief Initializes the planner plugin with a user configured name and a shared pointer to the mesh map
   * @param name The user configured name, which is used as namespace for parameters, etc.
   * @param mesh_map_ptr A shared pointer to the mesh map instance to access attributes and helper functions, etc.
   * @return true if the plugin has been initialized successfully
   */
  virtual bool initialize(const std::string& plugin_name, const std::shared_ptr<mesh_map::MeshMap>& mesh_map_ptr, const rclcpp::Node::SharedPtr& node) override;

protected:

  /**
   * @brief Computes a wavefront propagation from the start until it reached the goal
   * @param start The seed of the wave, i.e. the robot's goal pose
   * @param goal The goal of the wavefront, where it will stop propagating
   * @param path The backtracked path
   * @param message String with additional information wrt. the outcome. Will be transmitted to the action caller.
   * @return a ExePath action related outcome code
   */
  uint32_t waveFrontPropagation(const mesh_map::Vector& start, const mesh_map::Vector& goal,
                                std::list<std::pair<mesh_map::Vector, lvr2::FaceHandle>>& path,
                                std::string& message);

  /**
   *
   * @brief Computes a wavefront propagation from the start until it reached the goal
   * @param start The seed of the wave, i.e. the robot's goal pose
   * @param goal The goal of the wavefront, where it will stop propagating
   * @param edge_weights The edge weights map to use for vertex distances in a triangle
   * @param costs The combined vertex costs to use during the propagation
   * @param path The backtracked path
   * @param message String with additional information wrt. the outcome. Will be transmitted to the action caller.
   * @param distances The computed distances
   * @param predecessors The backtracked predecessors
   * @return a ExePath action related outcome code
   */
  uint32_t waveFrontPropagation(const mesh_map::Vector& start, const mesh_map::Vector& goal,
                                const lvr2::DenseEdgeMap<float>& edge_weights, const lvr2::DenseVertexMap<float>& costs,
                                std::list<std::pair<mesh_map::Vector, lvr2::FaceHandle>>& path, std::string& message,
                                lvr2::DenseVertexMap<float>& distances,
                                lvr2::DenseVertexMap<lvr2::VertexHandle>& predecessors);

  /**
   * Single source update step using the Hesse normal form to determine if the direction vector is cutting the current triangle
   * @param distances Distance map to the goal which stores the current state of all distances to the goal
   * @param edge_weights Distances assigned to each edge
   * @param v1 The first vertex of the triangle
   * @param v2 The second vertex of the triangle
   * @param v3 The thrid vertex of the triangle
   * @return true if the newly computed distance is shorter than before and if the current triangle is cut
   */
  inline bool waveFrontUpdateWithS(lvr2::DenseVertexMap<float>& distances,
                                   const lvr2::DenseEdgeMap<float>& edge_weights, const lvr2::VertexHandle& v1,
                                   const lvr2::VertexHandle& v2, const lvr2::VertexHandle& v3);

  /**
   * Fast Marching Method update step using the Law of Cosines to determine if the direction vector is cutting the current triangle
   * @param distances Distance map to the goal which stores the current state of all distances to the goal
   * @param edge_weights Distances assigned to each edge
   * @param v1 The first vertex of the triangle
   * @param v2 The second vertex of the triangle
   * @param v3 The thrid vertex of the triangle
   * @return true if the newly computed distance is shorter than before and if the current triangle is cut
   */
  inline bool waveFrontUpdateFMM(lvr2::DenseVertexMap<float>& distances, const lvr2::DenseEdgeMap<float>& edge_weights,
                              const lvr2::VertexHandle& v1, const lvr2::VertexHandle& v2, const lvr2::VertexHandle& v3);

  /**
   * Single source update step using the Law of Cosines to determine if the direction vector is cutting the current triangle
   * @param distances Distance map to the goal which stores the current state of all distances to the goal
   * @param edge_weights Distances assigned to each edge
   * @param v1 The first vertex of the triangle
   * @param v2 The second vertex of the triangle
   * @param v3 The thrid vertex of the triangle
   * @return true if the newly computed distance is shorter than before and if the current triangle is cut
   */
  inline bool waveFrontUpdate(lvr2::DenseVertexMap<float>& distances, const lvr2::DenseEdgeMap<float>& edge_weights,
                              const lvr2::VertexHandle& v1, const lvr2::VertexHandle& v2, const lvr2::VertexHandle& v3);

  /**
   * @brief Computes the vector field in a post processing. It rotates the predecessor edges by the stored angles
   */
  void computeVectorMap();

  /**
   * @brief gets called on new incoming reconfigure parameters
   *
   * @param cfg new configuration
   * @param level level
   * @brief gets called whenever the node's parameters change
   
   * @param parameters vector of changed parameters.
   *                   Note that this vector will also contain parameters not related to the cvp mesh planner
   */
  rcl_interfaces::msg::SetParametersResult reconfigureCallback(std::vector<rclcpp::Parameter> parameters);

private:

  //! shared pointer to the mesh map
  mesh_map::MeshMap::Ptr mesh_map_;

  //! the user defined plugin name
  std::string name_;

  //! pointer to the node in which this plugin is running
  rclcpp::Node::SharedPtr node_;

  //! flag if cancel has been requested
  std::atomic_bool cancel_planning_;

  //! publisher for the backtracked path
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;

  //! the map coordinate frame / system id
  std::string map_frame_;

  // handle of callback for changing parameters dynamically
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr reconfiguration_callback_handle_;
  struct {
    //! whether to publish the vector field or not
    bool publish_vector_field = false;
    //! whether to also publish direction vectors at the triangle centers
    bool publish_face_vectors = false;
    //! an offset that determines how far beyond the goal (robot's position) is propagated.
    double goal_dist_offset = 0.3;
    //! Defines the vertex cost limit with which it can be accessed.
    double cost_limit = 1.0;
    //! The vector field back tracking step width.
    double step_width = 0.4;
  } config_;

  //! theta angles to the source of the wave front propagation
  lvr2::DenseVertexMap<float> direction_;

  //! predecessors while wave propagation
  lvr2::DenseVertexMap<lvr2::VertexHandle> predecessors_;

  //! the face which is cut by the computed line to the source
  lvr2::DenseVertexMap<lvr2::FaceHandle> cutting_faces_;

  //! stores the current vector map containing vectors pointing to the seed
  lvr2::DenseVertexMap<mesh_map::Vector> vector_map_;

  //! potential field / scalar distance field to the seed
  lvr2::DenseVertexMap<float> potential_;
};

}  // namespace cvp_mesh_planner

#endif  // MESH_NAVIGATION__CVP_MESH_PLANNER_H
