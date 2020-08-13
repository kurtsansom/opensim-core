#ifndef OPENSIM_SIMULATION_UTILITIES_H_
#define OPENSIM_SIMULATION_UTILITIES_H_
/* -------------------------------------------------------------------------- *
 *                     OpenSim:  SimulationUtilities.h                        *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2018 Stanford University and the Authors                *
 * Author(s): OpenSim Team                                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "osimSimulationDLL.h"

#include <SimTKcommon/internal/State.h>

#include <OpenSim/Common/Storage.h>

namespace OpenSim {

class Model;

/** Simulate a model from an initial state and return the final state.
    If the model's useVisualizer flag is true, the user is repeatedly prompted
    to either begin simulating or quit. The provided state is not updated but
    the final state is returned at the end of the simulation, when finalTime is
    reached. %Set saveStatesFile=true to save the states to a storage file as:
    "<model_name>_states.sto".
    @ingroup simulationutil */
OSIMSIMULATION_API SimTK::State simulate(Model& model,
    const SimTK::State& initialState,
    double finalTime,
    bool saveStatesFile = false);

/// Update a vector of state labels (in place) to use post-4.0 state paths
/// instead of pre-4.0 state names. For example, this converts labels as
/// follows:
///   - `pelvis_tilt` -> `/jointset/ground_pelvis/pelvis_tilt/value`
///   - `pelvis_tilt_u` -> `/jointset/ground_pelvis/pelvis_tilt/speed`
///   - `soleus.activation` -> `/forceset/soleus/activation`
///   - `soleus.fiber_length` -> `/forceset/soleus/fiber_length`
/// This can also be used to update the column labels of an Inverse
/// Kinematics Tool solution MOT file so that the data can be used as
/// states. If a label does not identify a state in the model, the column
/// label is not changed.
/// @throws Exception if labels are not unique.
/// @ingroup simulationutil
OSIMSIMULATION_API void updateStateLabels40(
        const Model& model, std::vector<std::string>& labels);

#ifndef SWIG
/** Not available through scripting. 
 @returns nullptr if no update is necessary.
 @ingroup simulationutil */
OSIMSIMULATION_API std::unique_ptr<Storage>
updatePre40KinematicsStorageFor40MotionType(const Model& pre40Model,
        const Storage &kinematics);
#endif // SWIG
    
/** This function can be used to upgrade MOT files generated with versions
    before 4.0 in which some data columns are associated with coordinates
    that were incorrectly marked as Rotational (rather than Coupled). Specific
    instances of the issue are the patella coordinate in the Rajagopal 2015 and
    leg6dof9musc models. In these cases, the patella will visualize incorrectly
    in the GUI when replaying the kinematics from the MOT file, and Static
    Optimization will yield incorrect results.
 
    The new files are written to the same directories as the original files,
    but with the provided suffix (before the file extension). To overwrite your
    original files, set the suffix to an emtpty string.
 
    If the file does not need to be updated, no new file is written.
 
    Conversion of the data only occurs for files in degrees ("inDegrees=yes"
    in the header).
 
    Do not use this function with MOT files generated by 4.0 or later; doing
    so will cause your data to be altered incorrectly. We do not detect whether
    or not your MOT file is pre-4.0.
 
    In OpenSim 4.0, MotionTypes for
    Coordinates are now determined strictly by the coordinates' owning Joint.
    In older models, the MotionType, particularly for CustomJoints, were user-
    specified. That entailed in some cases, incorrectly labeling a Coordinate
    as being Rotational, for example, when it is in fact Coupled. For the above
    models, for example, the patella Coordinate had been user-specified to be
    Rotational, but the angle of the patella about the Z-axis of the patella
    body, is a spline function (e.g. coupled function) of the patella
    Coordinate. Thus, the patella Coordinate is not an angle measurement
    and is not classified as Rotational. Use this utility to remove any unit
    conversions from Coordinates that were incorrectly labeled
    as Rotational in the past. For these Coordinates only, the utility will undo
    the incorrect radians to degrees conversion.
    @ingroup simulationutil */
OSIMSIMULATION_API
void updatePre40KinematicsFilesFor40MotionType(const Model& model,
        const std::vector<std::string>& filePaths,
        std::string suffix="_updated");

/** This function attempts to update the connectee path for any Socket anywhere
 * in the model whose connectee path does not point to an existing component.
 * The paths are updated by searching the model for a component with the
 * correct name. For example, a connectee path like
 * `../../some/invalid/path/to/foo` will be updated to `/bodyset/foo` if a Body
 * named `foo` exists in the Model's BodySet. If a socket specifies a Body `foo` and
 * more than one Body `foo` exists in the model, we emit a warning and the
 * socket that specified `foo` is not altered.
 *
 * This method is intended for use with models loaded from version-30516 XML
 * files to bring them up to date with the 4.0 interface.
 * @ingroup simulationutil
 * */
OSIMSIMULATION_API
void updateSocketConnecteesBySearch(Model& model);

} // end of namespace OpenSim

#endif // OPENSIM_SIMULATION_UTILITIES_H_
