//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Ignasi de Pouplana
//

// Project includes
#include "central_differences_scheme.h"

namespace Kratos {

    void CentralDifferencesScheme::SetTranslationalIntegrationSchemeInProperties(Properties::Pointer pProp, bool verbose) const {
//         if(verbose) KRATOS_INFO("DEM") << "Assigning CentralDifferencesScheme to properties " << pProp->Id() << std::endl;
        pProp->SetValue(DEM_TRANSLATIONAL_INTEGRATION_SCHEME_POINTER, this->CloneShared());
    }

    void CentralDifferencesScheme::SetRotationalIntegrationSchemeInProperties(Properties::Pointer pProp, bool verbose) const {
//         if(verbose) KRATOS_INFO("DEM") << "Assigning CentralDifferencesScheme to properties " << pProp->Id() << std::endl;
        pProp->SetValue(DEM_ROTATIONAL_INTEGRATION_SCHEME_POINTER, this->CloneShared());
    }

    void CentralDifferencesScheme::UpdateTranslationalVariables(
            int StepFlag,
            Node < 3 >& i,
            array_1d<double, 3 >& coor,
            array_1d<double, 3 >& displ,
            array_1d<double, 3 >& delta_displ,
            array_1d<double, 3 >& vel,
            const array_1d<double, 3 >& initial_coor,
            const array_1d<double, 3 >& force,
            const double force_reduction_factor,
            const double mass,
            const double delta_t,
            const bool Fix_vel[3]) {

        const double& alpha = i.FastGetSolutionStepValue(RAYLEIGH_ALPHA);
        const double& beta = i.FastGetSolutionStepValue(RAYLEIGH_BETA);
        array_1d<double,3>& displ_old = i.FastGetSolutionStepValue(DISPLACEMENT_OLD);
        const array_1d<double,3>& internal_force = i.FastGetSolutionStepValue(INTERNAL_FORCE);
        const array_1d<double,3>& internal_force_old = i.FastGetSolutionStepValue(INTERNAL_FORCE_OLD);
        const array_1d<double,3>& external_force = i.FastGetSolutionStepValue(EXTERNAL_FORCE);

        double mass_inv = 1.0 / mass;
        for (int k = 0; k < 3; k++) {
            if (Fix_vel[k] == false) {
                delta_displ[k] = ( (2.0-delta_t*alpha)*mass*displ[k]
                                + (delta_t*alpha-1.0)*mass*displ_old[k]
                                - delta_t*(beta+delta_t)*internal_force[k]
                                + delta_t*beta*internal_force_old[k]
                                + delta_t*delta_t*external_force[k] ) * mass_inv - displ[k];
                displ_old[k] = displ[k];
                displ[k] = displ_old[k] + delta_displ[k];
                coor[k] = initial_coor[k] + displ[k];
                vel[k] = delta_displ[k]/delta_t;
            } else {
                delta_displ[k] = delta_t * vel[k];
                displ[k] += delta_displ[k];
                coor[k] = initial_coor[k] + displ[k];
            }
        } // dimensions
    }

    void CentralDifferencesScheme::CalculateNewRotationalVariablesOfSpheres(
                int StepFlag,
                Node < 3 >& i,
                const double moment_of_inertia,
                array_1d<double, 3 >& angular_velocity,
                array_1d<double, 3 >& torque,
                const double moment_reduction_factor,
                array_1d<double, 3 >& rotated_angle,
                array_1d<double, 3 >& delta_rotation,
                const double delta_t,
                const bool Fix_Ang_vel[3]) {

        const double& alpha = i.FastGetSolutionStepValue(RAYLEIGH_ALPHA);
        const double& beta = i.FastGetSolutionStepValue(RAYLEIGH_BETA);
        array_1d<double,3>& rotated_angle_old = i.FastGetSolutionStepValue(PARTICLE_ROTATION_ANGLE_OLD);
        const array_1d<double,3>& internal_torque = i.FastGetSolutionStepValue(PARTICLE_INTERNAL_MOMENT);
        const array_1d<double,3>& internal_torque_old = i.FastGetSolutionStepValue(PARTICLE_INTERNAL_MOMENT_OLD);
        const array_1d<double,3>& external_torque = i.FastGetSolutionStepValue(PARTICLE_EXTERNAL_MOMENT);

        double moment_of_inertia_inv = 1.0 / moment_of_inertia;
        for (int k = 0; k < 3; k++) {
            if (Fix_Ang_vel[k] == false) {
                delta_rotation[k] = ( (2.0-delta_t*alpha)*moment_of_inertia*rotated_angle[k]
                                    + (delta_t*alpha-1.0)*moment_of_inertia*rotated_angle_old[k]
                                    - delta_t*(beta+delta_t)*internal_torque[k]
                                    + delta_t*beta*internal_torque_old[k]
                                    + delta_t*delta_t*external_torque[k] ) * moment_of_inertia_inv - rotated_angle[k];
                rotated_angle_old[k] = rotated_angle[k];
                rotated_angle[k] = rotated_angle_old[k] + delta_rotation[k];
                angular_velocity[k] = delta_rotation[k]/delta_t;
            } else {
                delta_rotation[k] = delta_t * angular_velocity[k];
                rotated_angle[k] += delta_rotation[k];
            }
        } // dimensions
    }

    void CentralDifferencesScheme::CalculateNewRotationalVariablesOfRigidBodyElements(
                int StepFlag,
                Node < 3 >& i,
                const array_1d<double, 3 > moments_of_inertia,
                array_1d<double, 3 >& angular_velocity,
                array_1d<double, 3 >& torque,
                const double moment_reduction_factor,
                array_1d<double, 3 >& rotated_angle,
                array_1d<double, 3 >& delta_rotation,
                Quaternion<double  >& Orientation,
                const double delta_t,
                const bool Fix_Ang_vel[3]) {

        array_1d<double, 3 >& local_angular_velocity = i.FastGetSolutionStepValue(LOCAL_ANGULAR_VELOCITY);

        array_1d<double, 3 > local_angular_acceleration, local_torque, angular_acceleration;

        GeometryFunctions::QuaternionVectorGlobal2Local(Orientation, torque, local_torque);
        GeometryFunctions::QuaternionVectorGlobal2Local(Orientation, angular_velocity, local_angular_velocity);
        CalculateLocalAngularAccelerationByEulerEquations(local_angular_velocity, moments_of_inertia, local_torque, moment_reduction_factor, local_angular_acceleration);
        GeometryFunctions::QuaternionVectorLocal2Global(Orientation, local_angular_acceleration, angular_acceleration);

        // TODO: this is from symplectic euler...
        UpdateRotationalVariables(StepFlag, i, rotated_angle, delta_rotation, angular_velocity, angular_acceleration, delta_t, Fix_Ang_vel);

        double ang = DEM_INNER_PRODUCT_3(delta_rotation, delta_rotation);

        if (ang) {
            GeometryFunctions::UpdateOrientation(Orientation, delta_rotation);
        } //if ang
        GeometryFunctions::QuaternionVectorGlobal2Local(Orientation, angular_velocity, local_angular_velocity);
    }

    void CentralDifferencesScheme::UpdateRotationalVariables(
                int StepFlag,
                Node < 3 >& i,
                array_1d<double, 3 >& rotated_angle,
                array_1d<double, 3 >& delta_rotation,
                array_1d<double, 3 >& angular_velocity,
                array_1d<double, 3 >& angular_acceleration,
                const double delta_t,
                const bool Fix_Ang_vel[3]) {

        for (int k = 0; k < 3; k++) {
            if (Fix_Ang_vel[k] == false) {
                angular_velocity[k] += delta_t * angular_acceleration[k];
                delta_rotation[k] = angular_velocity[k] * delta_t;
                rotated_angle[k] += delta_rotation[k];
            } else {
                delta_rotation[k] = angular_velocity[k] * delta_t;
                rotated_angle[k] += delta_rotation[k];
            }
        }
    }

    void CentralDifferencesScheme::CalculateLocalAngularAccelerationByEulerEquations(
                const array_1d<double, 3 >& local_angular_velocity,
                const array_1d<double, 3 >& moments_of_inertia,
                const array_1d<double, 3 >& local_torque,
                const double moment_reduction_factor,
                array_1d<double, 3 >& local_angular_acceleration) {

        for (int j = 0; j < 3; j++) {
            local_angular_acceleration[j] = (local_torque[j] - (local_angular_velocity[(j + 1) % 3] * moments_of_inertia[(j + 2) % 3] * local_angular_velocity[(j + 2) % 3] - local_angular_velocity[(j + 2) % 3] * moments_of_inertia[(j + 1) % 3] * local_angular_velocity[(j + 1) % 3])) / moments_of_inertia[j];
            local_angular_acceleration[j] = local_angular_acceleration[j] * moment_reduction_factor;
        }
    }
} //namespace Kratos
