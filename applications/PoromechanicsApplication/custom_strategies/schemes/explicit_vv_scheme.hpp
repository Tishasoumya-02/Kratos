//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license:
//kratos/license.txt
//
//  Main authors:    Ignasi de Pouplana
//
//

#if !defined(KRATOS_EXPLICIT_VV_SCHEME_HPP_INCLUDED)
#define KRATOS_EXPLICIT_VV_SCHEME_HPP_INCLUDED

/* External includes */

/* Project includes */
#include "solving_strategies/schemes/scheme.h"
#include "utilities/variable_utils.h"

// Application includes
#include "poromechanics_application_variables.h"

namespace Kratos {

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}

///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/**
 * @class ExplicitVVScheme
 * @ingroup StructuralMechanicsApplciation
 * @brief An explicit forward euler scheme with a split of the inertial term
 * @author Ignasi de Pouplana
 */
template <class TSparseSpace,
          class TDenseSpace //= DenseSpace<double>
          >
class ExplicitVVScheme
    : public Scheme<TSparseSpace, TDenseSpace> {

public:
    ///@name Type Definitions
    ///@{

    /// The definition of the base type
    typedef Scheme<TSparseSpace, TDenseSpace> BaseType;

    /// Some definitions related with the base class
    typedef typename BaseType::DofsArrayType DofsArrayType;
    typedef typename BaseType::TSystemMatrixType TSystemMatrixType;
    typedef typename BaseType::TSystemVectorType TSystemVectorType;
    typedef typename BaseType::LocalSystemVectorType LocalSystemVectorType;

    /// The arrays of elements and nodes
    typedef ModelPart::ElementsContainerType ElementsArrayType;
    typedef ModelPart::NodesContainerType NodesArrayType;

    /// Definition of the size type
    typedef std::size_t SizeType;

    /// Definition of the index type
    typedef std::size_t IndexType;

    /// Definition fo the node iterator
    typedef typename ModelPart::NodeIterator NodeIterator;

    /// The definition of the numerical limit
    static constexpr double numerical_limit = std::numeric_limits<double>::epsilon();

    /// Counted pointer of ExplicitVVScheme
    KRATOS_CLASS_POINTER_DEFINITION(ExplicitVVScheme);

    ///@}
    ///@name Life Cycle
    ///@{

    /**
     * @brief Default constructor.
     * @details The ExplicitVVScheme method
     */
    ExplicitVVScheme()
        : Scheme<TSparseSpace, TDenseSpace>()
    {

    }

    /** Destructor.
    */
    virtual ~ExplicitVVScheme() {}

    ///@}
    ///@name Operators
    ///@{


    /**
     * @brief This function is designed to be called once to perform all the checks needed
     * on the input provided.
     * @details Checks can be "expensive" as the function is designed
     * to catch user's errors.
     * @param rModelPart The model of the problem to solve
     * @return Zero means  all ok
     */
    int Check(const ModelPart& rModelPart) const override
    {
        KRATOS_TRY;

        BaseType::Check(rModelPart);

        KRATOS_ERROR_IF(rModelPart.GetBufferSize() < 2) << "Insufficient buffer size for Forward Euler Scheme. It has to be >= 2" << std::endl;

        KRATOS_ERROR_IF_NOT(rModelPart.GetProcessInfo().Has(DOMAIN_SIZE)) << "DOMAIN_SIZE not defined on ProcessInfo. Please define" << std::endl;

        return 0;

        KRATOS_CATCH("");
    }

    /**
     * @brief This is the place to initialize the Scheme. This is intended to be called just once when the strategy is initialized
     * @param rModelPart The model of the problem to solve
     */
    void Initialize(ModelPart& rModelPart) override
    {
        KRATOS_TRY

        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

        // Preparing the time values for the first step (where time = initial_time +
        // dt)
        // mTime.Current = r_current_process_info[TIME] + r_current_process_info[DELTA_TIME];
        mDeltaTime = r_current_process_info[DELTA_TIME];
        mAlpha = r_current_process_info[RAYLEIGH_ALPHA];
        mBeta = r_current_process_info[RAYLEIGH_BETA];
        mTheta1 = r_current_process_info[THETA_1];

        /// Working in 2D/3D (the definition of DOMAIN_SIZE is check in the Check method)
        const SizeType dim = r_current_process_info[DOMAIN_SIZE];

        // Initialize scheme
        if (!BaseType::SchemeIsInitialized())
            InitializeExplicitScheme(rModelPart, dim);
        else
            SchemeCustomInitialization(rModelPart, dim);

        BaseType::SetSchemeIsInitialized();

        KRATOS_CATCH("")
    }

    /**
     * @brief It initializes time step solution. Only for reasons if the time step solution is restarted
     * @param rModelPart The model of the problem to solve
     * @param rA LHS matrix
     * @param rDx Incremental update of primary variables
     * @param rb RHS Vector
     * @todo I cannot find the formula for the higher orders with variable time step. I tried to deduce by myself but the result was very unstable
     */
    void InitializeSolutionStep(
        ModelPart& rModelPart,
        TSystemMatrixType& rA,
        TSystemVectorType& rDx,
        TSystemVectorType& rb
        ) override
    {
        KRATOS_TRY

        BaseType::InitializeSolutionStep(rModelPart, rA, rDx, rb);

        KRATOS_CATCH("")
    }

    /**
     * @brief It initializes the non-linear iteration
     * @param rModelPart The model of the problem to solve
     * @param rA LHS matrix
     * @param rDx Incremental update of primary variables
     * @param rb RHS Vector
     * @todo I cannot find the formula for the higher orders with variable time step. I tried to deduce by myself but the result was very unstable
     */
    void InitializeNonLinIteration(
        ModelPart& rModelPart,
        TSystemMatrixType& rA,
        TSystemVectorType& rDx,
        TSystemVectorType& rb
        ) override
    {
        KRATOS_TRY;

        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

        const auto it_elem_begin = rModelPart.ElementsBegin();
        #pragma omp parallel for schedule(guided,512)
        for(int i=0; i<static_cast<int>(rModelPart.Elements().size()); ++i) {
            auto it_elem = it_elem_begin + i;
            it_elem->InitializeNonLinearIteration(r_current_process_info);
        }

        const auto it_cond_begin = rModelPart.ConditionsBegin();
        #pragma omp parallel for schedule(guided,512)
        for(int i=0; i<static_cast<int>(rModelPart.Conditions().size()); ++i) {
            auto it_elem = it_cond_begin + i;
            it_elem->InitializeNonLinearIteration(r_current_process_info);
        }

        KRATOS_CATCH( "" );
    }

    /**
     * @brief This method initializes some rutines related with the explicit scheme
     * @param rModelPart The model of the problem to solve
     * @param DomainSize The current dimention of the problem
     */
    virtual void InitializeExplicitScheme(
        ModelPart& rModelPart,
        const SizeType DomainSize = 3
        )
    {
        KRATOS_TRY

        /// The array of ndoes
        NodesArrayType& r_nodes = rModelPart.Nodes();

        // The first iterator of the array of nodes
        const auto it_node_begin = rModelPart.NodesBegin();

        /// Initialise the database of the nodes
        #pragma omp parallel for schedule(guided,512)
        for (int i = 0; i < static_cast<int>(r_nodes.size()); ++i) {
            auto it_node = (it_node_begin + i);
            it_node->SetValue(NODAL_MASS, 0.0);
            // TODO: Set Nodal AntiCompressibility to zero for mass-balance equation (C=1/Q, with Q being the compressibility coeff.)
            array_1d<double, 3>& r_force_residual = it_node->FastGetSolutionStepValue(FORCE_RESIDUAL);
            double& r_flux_residual = it_node->FastGetSolutionStepValue(FLUX_RESIDUAL);
            array_1d<double, 3>& r_external_force = it_node->FastGetSolutionStepValue(EXTERNAL_FORCE);
            array_1d<double, 3>& r_internal_force = it_node->FastGetSolutionStepValue(INTERNAL_FORCE);
            array_1d<double, 3>& r_damping_force = it_node->FastGetSolutionStepValue(DAMPING_FORCE);
            noalias(r_force_residual) = ZeroVector(3);
            r_flux_residual = 0.0;
            noalias(r_external_force) = ZeroVector(3);
            noalias(r_internal_force) = ZeroVector(3);
            noalias(r_damping_force) = ZeroVector(3);
        }

        KRATOS_CATCH("")
    }

     void Predict(
        ModelPart& rModelPart,
        DofsArrayType& rDofSet,
        TSystemMatrixType& A,
        TSystemVectorType& Dx,
        TSystemVectorType& b
    ) override
    {
        KRATOS_TRY;

        InitializeResidual(rModelPart);

        this->CalculateAndAddRHS(rModelPart);

        // The current process info
        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

        // The array of nodes
        NodesArrayType& r_nodes = rModelPart.Nodes();

        /// Working in 2D/3D (the definition of DOMAIN_SIZE is check in the Check method)
        const SizeType dim = r_current_process_info[DOMAIN_SIZE];

        // Step Update
        mDeltaTime = r_current_process_info[DELTA_TIME];

        // The iterator of the first node
        const auto it_node_begin = rModelPart.NodesBegin();

        // Getting dof position
        const IndexType disppos = it_node_begin->GetDofPosition(DISPLACEMENT_X);

        #pragma omp parallel for schedule(guided,512)
        for (int i = 0; i < static_cast<int>(r_nodes.size()); ++i) {
            // Current step information "N+1" (before step update).
            this->PredictTranslationalDegreesOfFreedom(it_node_begin + i, disppos, dim);
        } // for Node parallel

        InitializeResidual(rModelPart);

        this->CalculateAndAddRHS(rModelPart);

        KRATOS_CATCH("")
    }

    /**
     * @brief This method initializes the residual in the nodes of the model part
     * @param rModelPart The model of the problem to solve
     */
    virtual void InitializeResidual(ModelPart& rModelPart)
    {
        KRATOS_TRY

        // The array of nodes
        NodesArrayType& r_nodes = rModelPart.Nodes();

        // Auxiliar values
        const array_1d<double, 3> zero_array = ZeroVector(3);
        double zero_double = 0.0;
        // Initializing the variables
        VariableUtils().SetVariable(FORCE_RESIDUAL, zero_array,r_nodes);
        VariableUtils().SetVariable(FLUX_RESIDUAL, zero_double,r_nodes);
        VariableUtils().SetVariable(EXTERNAL_FORCE, zero_array,r_nodes);
        VariableUtils().SetVariable(INTERNAL_FORCE, zero_array,r_nodes);
        VariableUtils().SetVariable(DAMPING_FORCE, zero_array,r_nodes);

        KRATOS_CATCH("")
    }

    /**
     * @brief This method updates the translation DoF
     * @param itCurrentNode The iterator of the current node
     * @param DisplacementPosition The position of the displacement dof on the database
     * @param DomainSize The current dimention of the problem
     */
    virtual void PredictTranslationalDegreesOfFreedom(
        NodeIterator itCurrentNode,
        const IndexType DisplacementPosition,
        const SizeType DomainSize = 3
        )
    {
        array_1d<double, 3>& r_current_displacement = itCurrentNode->FastGetSolutionStepValue(DISPLACEMENT);
        array_1d<double, 3>& r_current_velocity = itCurrentNode->FastGetSolutionStepValue(VELOCITY);

        const double nodal_mass = itCurrentNode->GetValue(NODAL_MASS);

        const array_1d<double, 3>& r_external_forces = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE);
        const array_1d<double, 3>& r_previous_external_forces = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE,1);
        const array_1d<double, 3>& r_current_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE);
        const array_1d<double, 3>& r_previous_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE,1);
        const array_1d<double, 3>& r_current_damping_force = itCurrentNode->FastGetSolutionStepValue(DAMPING_FORCE);

        std::array<bool, 3> fix_displacements = {false, false, false};
        fix_displacements[0] = (itCurrentNode->GetDof(DISPLACEMENT_X, DisplacementPosition).IsFixed());
        fix_displacements[1] = (itCurrentNode->GetDof(DISPLACEMENT_Y, DisplacementPosition + 1).IsFixed());
        if (DomainSize == 3)
            fix_displacements[2] = (itCurrentNode->GetDof(DISPLACEMENT_Z, DisplacementPosition + 2).IsFixed());

        // Solution of the explicit equation:
        if (nodal_mass > numerical_limit){
            for (IndexType j = 0; j < DomainSize; j++) {
                if (fix_displacements[j] == false) {
                    r_current_displacement[j] += r_current_velocity[j]*mDeltaTime + 0.5 * (mTheta1*r_external_forces[j]+(1.0-mTheta1)*r_previous_external_forces[j]
                                                                                           - (mTheta1*r_current_internal_force[j]+(1.0-mTheta1)*r_previous_internal_force[j])
                                                                                           - r_current_damping_force[j])/nodal_mass * mDeltaTime * mDeltaTime;
                    r_current_velocity[j] += 0.5 * mDeltaTime * (mTheta1*r_external_forces[j]+(1.0-mTheta1)*r_previous_external_forces[j]
                                                                 - (mTheta1*r_current_internal_force[j]+(1.0-mTheta1)*r_previous_internal_force[j])
                                                                 - r_current_damping_force[j])/nodal_mass;
                }
            }
        }
        else {
            noalias(r_current_displacement) = ZeroVector(3);
            noalias(r_current_velocity) = ZeroVector(3);
        }
    }

    /**
     * @brief Performing the update of the solution
     * @param rModelPart The model of the problem to solve
     * @param rDofSet Set of all primary variables
     * @param rA LHS matrix
     * @param rDx incremental update of primary variables
     * @param rb RHS Vector
     */
    void Update(
        ModelPart& rModelPart,
        DofsArrayType& rDofSet,
        TSystemMatrixType& rA,
        TSystemVectorType& rDx,
        TSystemVectorType& rb
        ) override
    {
        KRATOS_TRY
        // The current process info
        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();

        // The array of nodes
        NodesArrayType& r_nodes = rModelPart.Nodes();

        /// Working in 2D/3D (the definition of DOMAIN_SIZE is check in the Check method)
        const SizeType dim = r_current_process_info[DOMAIN_SIZE];

        // Step Update
        mDeltaTime = r_current_process_info[DELTA_TIME];

        // The iterator of the first node
        const auto it_node_begin = rModelPart.NodesBegin();

        // Getting dof position
        const IndexType disppos = it_node_begin->GetDofPosition(DISPLACEMENT_X);

        #pragma omp parallel for schedule(guided,512)
        for (int i = 0; i < static_cast<int>(r_nodes.size()); ++i) {
            // Current step information "N+1" (before step update).
            this->UpdateTranslationalDegreesOfFreedom(it_node_begin + i, disppos, dim);
        } // for Node parallel

        KRATOS_CATCH("")
    }

    /**
     * @brief This method updates the translation DoF
     * @param itCurrentNode The iterator of the current node
     * @param DisplacementPosition The position of the displacement dof on the database
     * @param DomainSize The current dimention of the problem
     */
    virtual void UpdateTranslationalDegreesOfFreedom(
        NodeIterator itCurrentNode,
        const IndexType DisplacementPosition,
        const SizeType DomainSize = 3
        )
    {
        array_1d<double, 3>& r_current_velocity = itCurrentNode->FastGetSolutionStepValue(VELOCITY);

        double& r_current_water_pressure = itCurrentNode->FastGetSolutionStepValue(WATER_PRESSURE);
        double& r_current_dt_water_pressure = itCurrentNode->FastGetSolutionStepValue(DT_WATER_PRESSURE);

        const double nodal_mass = itCurrentNode->GetValue(NODAL_MASS);

        const array_1d<double, 3>& r_external_forces = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE);
        const array_1d<double, 3>& r_previous_external_forces = itCurrentNode->FastGetSolutionStepValue(EXTERNAL_FORCE,1);
        const array_1d<double, 3>& r_current_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE);
        const array_1d<double, 3>& r_previous_internal_force = itCurrentNode->FastGetSolutionStepValue(INTERNAL_FORCE,1);
        const array_1d<double, 3>& r_current_damping_force = itCurrentNode->FastGetSolutionStepValue(DAMPING_FORCE);

        std::array<bool, 3> fix_displacements = {false, false, false};
        fix_displacements[0] = (itCurrentNode->GetDof(DISPLACEMENT_X, DisplacementPosition).IsFixed());
        fix_displacements[1] = (itCurrentNode->GetDof(DISPLACEMENT_Y, DisplacementPosition + 1).IsFixed());
        if (DomainSize == 3)
            fix_displacements[2] = (itCurrentNode->GetDof(DISPLACEMENT_Z, DisplacementPosition + 2).IsFixed());

        // Solution of the explicit equation:
        if (nodal_mass > numerical_limit){
            for (IndexType j = 0; j < DomainSize; j++) {
                if (fix_displacements[j] == false) {
                    r_current_velocity[j] += 0.5 * mDeltaTime * (mTheta1*r_external_forces[j]+(1.0-mTheta1)*r_previous_external_forces[j]
                                                                 - (mTheta1*r_current_internal_force[j]+(1.0-mTheta1)*r_previous_internal_force[j])
                                                                 - r_current_damping_force[j])/nodal_mass;
                }
            }
        }
        else {
            noalias(r_current_velocity) = ZeroVector(3);
        }
        // Solution of the darcy_equation
        if( itCurrentNode->IsFixed(WATER_PRESSURE) == false ) {
            // TODO: this is on standby
            r_current_water_pressure = 0.0;
            r_current_dt_water_pressure = 0.0;
        }

        const array_1d<double, 3>& r_previous_velocity = itCurrentNode->FastGetSolutionStepValue(VELOCITY,1);
        array_1d<double, 3>& r_current_acceleration = itCurrentNode->FastGetSolutionStepValue(ACCELERATION);

        noalias(r_current_acceleration) = (1.0/mDeltaTime) * (r_current_velocity - r_previous_velocity);
    }

    /**
     * @brief This method performs some custom operations to initialize the scheme
     * @param rModelPart The model of the problem to solve
     * @param DomainSize The current dimention of the problem
     */
    virtual void SchemeCustomInitialization(
        ModelPart& rModelPart,
        const SizeType DomainSize = 3
        )
    {
        KRATOS_TRY

        KRATOS_CATCH("")
    }

    void CalculateAndAddRHS(ModelPart& rModelPart)
    {
        KRATOS_TRY

        const ProcessInfo& r_current_process_info = rModelPart.GetProcessInfo();
        ConditionsArrayType& r_conditions = rModelPart.Conditions();
        ElementsArrayType& r_elements = rModelPart.Elements();

        LocalSystemVectorType RHS_Contribution = LocalSystemVectorType(0);
        Element::EquationIdVectorType equation_id_vector_dummy; // Dummy

        #pragma omp parallel for firstprivate(RHS_Contribution, equation_id_vector_dummy), schedule(guided,512)
        for (int i = 0; i < static_cast<int>(r_conditions.size()); ++i) {
            auto it_cond = r_conditions.begin() + i;
            CalculateRHSContribution(*it_cond, RHS_Contribution, equation_id_vector_dummy, r_current_process_info);
        }

        #pragma omp parallel for firstprivate(RHS_Contribution, equation_id_vector_dummy), schedule(guided,512)
        for (int i = 0; i < static_cast<int>(r_elements.size()); ++i) {
            auto it_elem = r_elements.begin() + i;
            CalculateRHSContribution(*it_elem, RHS_Contribution, equation_id_vector_dummy, r_current_process_info);
        }

        KRATOS_CATCH("")
    }

    /**
     * @brief This function is designed to calculate just the RHS contribution
     * @param rCurrentElement The element to compute
     * @param RHS_Contribution The RHS vector contribution
     * @param EquationId The ID's of the element degrees of freedom
     * @param rCurrentProcessInfo The current process info instance
     */
    void CalculateRHSContribution(
        Element& rCurrentElement,
        LocalSystemVectorType& RHS_Contribution,
        Element::EquationIdVectorType& EquationId,
        const ProcessInfo& rCurrentProcessInfo
        ) override
    {
        KRATOS_TRY

        // this->TCalculateRHSContribution(rCurrentElement, RHS_Contribution, rCurrentProcessInfo);
        // rCurrentElement.CalculateRightHandSide(RHS_Contribution, rCurrentProcessInfo);
        rCurrentElement.AddExplicitContribution(RHS_Contribution, RESIDUAL_VECTOR, DAMPING_FORCE, rCurrentProcessInfo);

        KRATOS_CATCH("")
    }

    /**
     * @brief Functions that calculates the RHS of a "condition" object
     * @param rCurrentCondition The condition to compute
     * @param RHS_Contribution The RHS vector contribution
     * @param EquationId The ID's of the condition degrees of freedom
     * @param rCurrentProcessInfo The current process info instance
     */
    void CalculateRHSContribution(
        Condition& rCurrentCondition,
        LocalSystemVectorType& RHS_Contribution,
        Element::EquationIdVectorType& EquationId,
        const ProcessInfo& rCurrentProcessInfo
        ) override
    {
        KRATOS_TRY

        // this->TCalculateRHSContribution(rCurrentCondition, RHS_Contribution, rCurrentProcessInfo);
        rCurrentCondition.CalculateRightHandSide(RHS_Contribution, rCurrentProcessInfo);
        rCurrentCondition.AddExplicitContribution(RHS_Contribution, RESIDUAL_VECTOR, FORCE_RESIDUAL, rCurrentProcessInfo);

        KRATOS_CATCH("")
    }

    void FinalizeSolutionStep(
        ModelPart& rModelPart,
        TSystemMatrixType& rA,
        TSystemVectorType& rDx,
        TSystemVectorType& rb) override
    {
        KRATOS_TRY

        if(rModelPart.GetProcessInfo()[NODAL_SMOOTHING] == true)
        {
            unsigned int Dim = rModelPart.GetProcessInfo()[DOMAIN_SIZE];

            const int NNodes = static_cast<int>(rModelPart.Nodes().size());
            ModelPart::NodesContainerType::iterator node_begin = rModelPart.NodesBegin();

            // Clear nodal variables
            #pragma omp parallel for
            for(int i = 0; i < NNodes; i++)
            {
                ModelPart::NodesContainerType::iterator itNode = node_begin + i;

                itNode->FastGetSolutionStepValue(NODAL_AREA) = 0.0;
                Matrix& rNodalStress = itNode->FastGetSolutionStepValue(NODAL_EFFECTIVE_STRESS_TENSOR);
                if(rNodalStress.size1() != Dim)
                    rNodalStress.resize(Dim,Dim,false);
                noalias(rNodalStress) = ZeroMatrix(Dim,Dim);
                itNode->FastGetSolutionStepValue(NODAL_DAMAGE_VARIABLE) = 0.0;
                itNode->FastGetSolutionStepValue(NODAL_JOINT_AREA) = 0.0;
                itNode->FastGetSolutionStepValue(NODAL_JOINT_WIDTH) = 0.0;
                itNode->FastGetSolutionStepValue(NODAL_JOINT_DAMAGE) = 0.0;
            }

            BaseType::FinalizeSolutionStep(rModelPart, rA, rDx, rb);

            // Compute smoothed nodal variables
            #pragma omp parallel for
            for(int n = 0; n < NNodes; n++)
            {
                ModelPart::NodesContainerType::iterator itNode = node_begin + n;

                const double& NodalArea = itNode->FastGetSolutionStepValue(NODAL_AREA);
                if (NodalArea>1.0e-20)
                {
                    const double InvNodalArea = 1.0/NodalArea;
                    Matrix& rNodalStress = itNode->FastGetSolutionStepValue(NODAL_EFFECTIVE_STRESS_TENSOR);
                    for(unsigned int i = 0; i<Dim; i++)
                    {
                        for(unsigned int j = 0; j<Dim; j++)
                        {
                            rNodalStress(i,j) *= InvNodalArea;
                        }
                    }
                    double& NodalDamage = itNode->FastGetSolutionStepValue(NODAL_DAMAGE_VARIABLE);
                    NodalDamage *= InvNodalArea;
                }

                const double& NodalJointArea = itNode->FastGetSolutionStepValue(NODAL_JOINT_AREA);
                if (NodalJointArea>1.0e-20)
                {
                    const double InvNodalJointArea = 1.0/NodalJointArea;
                    double& NodalJointWidth = itNode->FastGetSolutionStepValue(NODAL_JOINT_WIDTH);
                    NodalJointWidth *= InvNodalJointArea;
                    double& NodalJointDamage = itNode->FastGetSolutionStepValue(NODAL_JOINT_DAMAGE);
                    NodalJointDamage *= InvNodalJointArea;
                }
            }
        }
        else
        {
            BaseType::FinalizeSolutionStep(rModelPart, rA, rDx, rb);
        }

        KRATOS_CATCH("")
    }

    ///@}
    ///@name Operations
    ///@{

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Friends
    ///@{

    ///@}

protected:

    ///@}
    ///@name Protected Structs
    ///@{


    ///@name Protected static Member Variables
    ///@{


    ///@}
    ///@name Protected member Variables
    ///@{

    double mDeltaTime;
    double mAlpha;
    double mBeta;
    double mTheta1;

    ///@}
    ///@name Protected Operators
    ///@{

    /**
    * @brief Functions that calculates the RHS of a "TObjectType" object
    * @param rCurrentEntity The TObjectType to compute
    * @param RHS_Contribution The RHS vector contribution
    * @param rCurrentProcessInfo The current process info instance
    */
    // template <typename TObjectType>
    // void TCalculateRHSContribution(
    //     TObjectType& rCurrentEntity,
    //     LocalSystemVectorType& RHS_Contribution,
    //     const ProcessInfo& rCurrentProcessInfo
    //     )
    // {
    //     rCurrentEntity.CalculateRightHandSide(RHS_Contribution, rCurrentProcessInfo);
    //     rCurrentEntity.AddExplicitContribution(RHS_Contribution, RESIDUAL_VECTOR, FORCE_RESIDUAL, rCurrentProcessInfo);
    // }

    ///@}
    ///@name Protected Operations
    ///@{

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}

private:
    ///@name Static Member Variables
    ///@{

    ///@}
    ///@name Member Variables
    ///@{

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    ///@}
    ///@name Private  Access
    ///@{

    ///@}
    ///@name Private Inquiry
    ///@{

    ///@}
    ///@name Un accessible methods
    ///@{

    ///@}

}; /* Class ExplicitVVScheme */

///@}

///@name Type Definitions
///@{

///@}

} /* namespace Kratos.*/

#endif /* KRATOS_EXPLICIT_VV_SCHEME_HPP_INCLUDED  defined */
