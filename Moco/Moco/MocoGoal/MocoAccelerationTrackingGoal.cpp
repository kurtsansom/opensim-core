/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoAccelerationTrackingGoal.cpp                             *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Nicholas Bianco                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "MocoAccelerationTrackingGoal.h"

#include "../MocoUtilities.h"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/StatesTrajectory.h>

using namespace OpenSim;
using SimTK::Vec3;

void MocoAccelerationTrackingGoal::initializeOnModelImpl(
        const Model& model) const {
    // Get the reference data.
    TimeSeriesTableVec3 accelerationTable;
    if (m_acceleration_table.getNumColumns() != 0 ||   // acceleration table or
            get_acceleration_reference_file() != "") { // reference file provided
        TimeSeriesTableVec3 accelerationTableToUse;
        // Should not be able to supply any two simultaneously.
        if (get_acceleration_reference_file() != "") { // acceleration ref file
            assert(m_acceleration_table.getNumColumns() == 0);
            accelerationTableToUse =
                    readTableFromFileT<Vec3>(get_acceleration_reference_file());

        } else { // acceleration table
            assert(get_acceleration_reference_file() == "");
            accelerationTableToUse = m_acceleration_table;
        }

        // If the frame_paths property is empty, use all frame paths specified
        // in the table's column labels. Otherwise, select only the columns
        // from the table that correspond with paths in frame_paths.
        if (!getProperty_frame_paths().empty()) {
            m_frame_paths = accelerationTableToUse.getColumnLabels();
            accelerationTable = accelerationTableToUse;
        } else {
            accelerationTable = TimeSeriesTableVec3(
                    accelerationTableToUse.getIndependentColumn());
            const auto& labels = accelerationTableToUse.getColumnLabels();
            for (int i = 0; i < getProperty_frame_paths().size(); ++i) {
                const auto& path = get_frame_paths(i);
                OPENSIM_THROW_IF_FRMOBJ(
                    std::find(labels.begin(), labels.end(), path) == 
                        labels.end(),
                    Exception,
                    format("Expected frame_paths to match one of the "
                       "column labels in the acceleration reference, but frame "
                       "path '%s' not found in the reference labels.", path));
                m_frame_paths.push_back(path);
                accelerationTable.appendColumn(path, 
                    accelerationTableToUse.getDependentColumn(path));
            }
        }

    }

    // Check that there are no redundant columns in the reference data.
    checkRedundantLabels(accelerationTable.getColumnLabels());

    // Cache the model frames and acceleration weights based on the order of the
    // acceleration table.
    for (int i = 0; i < (int)m_frame_paths.size(); ++i) {
        const auto& path = m_frame_paths[i];
        const auto& frame = model.getComponent<Frame>(path);
        m_model_frames.emplace_back(&frame);

        double weight = 1.0;
        if (get_acceleration_weights().contains(path)) {
            weight = get_acceleration_weights().get(path).getWeight();
        }
        m_acceleration_weights.push_back(weight);
    }

    m_ref_splines = GCVSplineSet(accelerationTable.flatten(
        {"/acceleration_x", "/acceleration_y", "/acceleration_z"}));

    setNumIntegralsAndOutputs(1, 1);
}

void MocoAccelerationTrackingGoal::calcIntegrandImpl(const SimTK::State& state, 
        double& integrand) const {
    const auto& time = state.getTime();
    getModel().realizeAcceleration(state);
    SimTK::Vector timeVec(1, time);

    integrand = 0;
    Vec3 acceleration_ref(0.0);
    for (int iframe = 0; iframe < (int)m_model_frames.size(); ++iframe) {
        const auto& acceleration_model =
                m_model_frames[iframe]->getLinearAccelerationInGround(state);

        // Compute acceleration error.
        for (int ia = 0; ia < acceleration_ref.size(); ++ia) {
            acceleration_ref[ia] =
                    m_ref_splines[3*iframe + ia].calcValue(timeVec);
        }
        Vec3 error = acceleration_model - acceleration_ref;

        // Add this frame's acceleration error to the integrand.
        const double& weight = m_acceleration_weights[iframe];
        integrand += weight * error.normSqr();
    }
}

void MocoAccelerationTrackingGoal::printDescriptionImpl(
        std::ostream& stream) const {
    stream << "        ";
    stream << "acceleration reference file: " 
           << get_acceleration_reference_file()
           << std::endl;
    for (int i = 0; i < (int)m_frame_paths.size(); i++) {
        stream << "        ";
        stream << "frame " << i << ": " << m_frame_paths[i] << ", ";
        stream << "weight: " << m_acceleration_weights[i] << std::endl;
    }
}