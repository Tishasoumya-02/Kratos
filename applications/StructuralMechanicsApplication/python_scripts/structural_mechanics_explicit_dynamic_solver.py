# Importing the Kratos Library
import KratosMultiphysics

# Import applications
import KratosMultiphysics.StructuralMechanicsApplication as StructuralMechanicsApplication

# Import base class file
from KratosMultiphysics.StructuralMechanicsApplication.structural_mechanics_solver import MechanicalSolver

def CreateSolver(model, custom_settings):
    return ExplicitMechanicalSolver(model, custom_settings)

class ExplicitMechanicalSolver(MechanicalSolver):
    """The structural mechanics explicit dynamic solver.

    This class creates the mechanical solvers for explicit dynamic analysis.

    See structural_mechanics_solver.py for more information.
    """
    def __init__(self, model, custom_settings):
        # Construct the base solver.
        super().__init__(model, custom_settings)
        # Lumped mass-matrix is necessary for explicit analysis
        self.main_model_part.ProcessInfo[KratosMultiphysics.COMPUTE_LUMPED_MASS_MATRIX] = True
        self.delta_time_refresh_counter = self.settings["delta_time_refresh"].GetInt()
        KratosMultiphysics.Logger.PrintInfo("::[ExplicitMechanicalSolver]:: Construction finished")

    @classmethod
    def GetDefaultParameters(cls):
        this_defaults = KratosMultiphysics.Parameters("""{
            "time_integration_method"    : "explicit",
            "scheme_type"                : "central_differences",
            "time_step_prediction_level" : 0,
            "delta_time_refresh"         : 1000,
            "max_delta_time"             : 1.0e0,
            "fraction_delta_time"        : 0.333333333333333333333333333333333333,
            "l2_rel_tolerance"           : 1.0e-4,
            "l2_abs_tolerance"           : 1.0e-9,
            "rayleigh_alpha"             : 0.0,
            "rayleigh_beta"              : 0.0,
            "theta_1"                    : 1.0,
            "theta_2"                    : 0.0,
            "theta_3"                    : 0.0,
            "delta"                      : 1.0,
            "calculate_alpha_beta"       : false,
            "xi_1"                       : 1.0,
            "xi_n"                       : 1.0,
            "omega_1"                    : 1.0,
            "omega_n"                    : 1.0
        }""")
        this_defaults.AddMissingParameters(super().GetDefaultParameters())
        return this_defaults

    def AddVariables(self):
        super().AddVariables()
        self._add_dynamic_variables()

        scheme_type = self.settings["scheme_type"].GetString()
        if(scheme_type == "central_differences"):
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.MIDDLE_VELOCITY)
            if (self.settings["rotation_dofs"].GetBool()):
                self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.MIDDLE_ANGULAR_VELOCITY)
        if(scheme_type == "multi_stage"):
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.FRACTIONAL_ACCELERATION)
            if (self.settings["rotation_dofs"].GetBool()):
                self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.FRACTIONAL_ANGULAR_ACCELERATION)
        if(scheme_type == "cd" or scheme_type == "ocd" or scheme_type == "vv" or scheme_type == "ovv" or scheme_type == "omdp" or scheme_type == "cd_fic"):
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.NODAL_INERTIA) # K*a: internal forces
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.MIDDLE_VELOCITY) # H1Ka
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.MIDDLE_ANGULAR_VELOCITY) # H1Ca
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.FRACTIONAL_ACCELERATION) # H1f
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.NODAL_DISPLACEMENT_STIFFNESS) # b: aux impulse


        self.main_model_part.AddNodalSolutionStepVariable(KratosMultiphysics.NODAL_MASS)
        self.main_model_part.AddNodalSolutionStepVariable(KratosMultiphysics.FORCE_RESIDUAL) # f: external forces
        self.main_model_part.AddNodalSolutionStepVariable(KratosMultiphysics.RESIDUAL_VECTOR)

        if (self.settings["rotation_dofs"].GetBool()):
            self.main_model_part.AddNodalSolutionStepVariable(StructuralMechanicsApplication.NODAL_INERTIA)
            self.main_model_part.AddNodalSolutionStepVariable(KratosMultiphysics.MOMENT_RESIDUAL)

        KratosMultiphysics.Logger.PrintInfo("::[ExplicitMechanicalSolver]:: Variables ADDED")

    def AddDofs(self):
        super().AddDofs()
        self._add_dynamic_dofs()
        KratosMultiphysics.Logger.PrintInfo("::[ExplicitMechanicalSolver]:: DOF's ADDED")

    def ComputeDeltaTime(self):
        if self.settings["time_step_prediction_level"].GetInt() > 1:
            if self.delta_time_refresh_counter >= self.settings["delta_time_refresh"].GetInt():
                self.delta_time = StructuralMechanicsApplication.CalculateDeltaTime(self.GetComputingModelPart(), self.delta_time_settings)
                self.delta_time_refresh_counter = 0
            else:
                self.delta_time_refresh_counter += 1
        return self.delta_time

    def Initialize(self):
        # Using the base Initialize
        super().Initialize()

        # Initilize delta_time
        self.delta_time_settings = KratosMultiphysics.Parameters("""{}""")
        self.delta_time_settings.AddValue("time_step_prediction_level", self.settings["time_step_prediction_level"])
        self.delta_time_settings.AddValue("max_delta_time", self.settings["max_delta_time"])
        if self.settings["time_step_prediction_level"].GetInt() > 0:
            self.delta_time = StructuralMechanicsApplication.CalculateDeltaTime(self.GetComputingModelPart(), self.delta_time_settings)
        else:
            self.delta_time = self.settings["time_stepping"]["time_step"].GetDouble()

    #### Specific internal functions ####
    def _create_solution_scheme(self):
        scheme_type = self.settings["scheme_type"].GetString()
        # print(scheme_type)
        # paraaa

        # Setting the Rayleigh damping parameters
        process_info = self.main_model_part.ProcessInfo
        # process_info[StructuralMechanicsApplication.RAYLEIGH_ALPHA] = self.settings["rayleigh_alpha"].GetDouble()
        # process_info[StructuralMechanicsApplication.RAYLEIGH_BETA] = self.settings["rayleigh_beta"].GetDouble()
        if self.settings["calculate_alpha_beta"].GetBool():
            xi_1 = self.settings["xi_1"].GetDouble()
            xi_n = self.settings["xi_n"].GetDouble()
            omega_1 = self.settings["omega_1"].GetDouble()
            omega_n = self.settings["omega_n"].GetDouble()
            beta = 2.0*(xi_n*omega_n-xi_1*omega_1)/(omega_n*omega_n-omega_1*omega_1)
            alpha = 2.0*xi_1*omega_1-beta*omega_1*omega_1
            print('Alpha and Beta input:')
            print('omega_1: ',omega_1)
            print('omega_n: ',omega_n)
            print('xi_1: ',xi_1)
            print('xi_n: ',xi_n)
            print('Alpha and Beta output:')
            print('alpha: ',alpha)
            print('beta: ',beta)
        else:
            alpha = self.settings["rayleigh_alpha"].GetDouble()
            beta = self.settings["rayleigh_beta"].GetDouble()

        process_info.SetValue(StructuralMechanicsApplication.RAYLEIGH_ALPHA, alpha)
        process_info.SetValue(StructuralMechanicsApplication.RAYLEIGH_BETA, beta)
        process_info.SetValue(StructuralMechanicsApplication.THETA_1, self.settings["theta_1"].GetDouble())
        process_info.SetValue(StructuralMechanicsApplication.THETA_2, self.settings["theta_2"].GetDouble())
        process_info.SetValue(StructuralMechanicsApplication.THETA_3, self.settings["theta_3"].GetDouble())
        process_info.SetValue(StructuralMechanicsApplication.LOAD_FACTOR, self.settings["delta"].GetDouble())
        process_info.SetValue(KratosMultiphysics.ERROR_RATIO, self.settings["l2_rel_tolerance"].GetDouble())
        process_info.SetValue(KratosMultiphysics.ERROR_INTEGRATION_POINT, self.settings["l2_abs_tolerance"].GetDouble())
        process_info.SetValue(KratosMultiphysics.DELTA_TIME, self.settings["time_stepping"]["time_step"].GetDouble())

        # Setting the time integration schemes
        if(scheme_type == "central_differences"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitCentralDifferencesScheme(self.settings["max_delta_time"].GetDouble(),
                                                                             self.settings["fraction_delta_time"].GetDouble(),
                                                                             self.settings["time_step_prediction_level"].GetDouble())
        elif(scheme_type == "multi_stage"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitMultiStageKimScheme(self.settings["fraction_delta_time"].GetDouble())
        elif(scheme_type == "cd"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitCDScheme()
        elif(scheme_type == "ocd"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitOCDScheme()
        elif(scheme_type == "omdp"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitOMDPScheme()
        elif(scheme_type == "cd_fic"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitCDFICScheme()
        elif(scheme_type == "vv"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitVVScheme()
        elif(scheme_type == "ovv"):
            mechanical_scheme = StructuralMechanicsApplication.ExplicitOVVScheme()

        else:
            err_msg =  "The requested scheme type \"" + scheme_type + "\" is not available!\n"
            err_msg += "Available options are: \"central_differences\", \"multi_stage\", \"cd\", \"omdp\", \"ocd\", \"vv\", \"ovv\""
            raise Exception(err_msg)
        return mechanical_scheme

    def _create_mechanical_solution_strategy(self):
        computing_model_part = self.GetComputingModelPart()
        mechanical_scheme = self.get_solution_scheme()

        mechanical_solution_strategy = StructuralMechanicsApplication.MechanicalExplicitStrategy(computing_model_part,
                                            mechanical_scheme,
                                            self.settings["compute_reactions"].GetBool(),
                                            self.settings["reform_dofs_at_each_step"].GetBool(),
                                            self.settings["move_mesh_flag"].GetBool())

        mechanical_solution_strategy.SetRebuildLevel(0)
        return mechanical_solution_strategy

    #### Private functions ####

