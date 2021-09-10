// KRATOS ___ ___  _  ___   __   ___ ___ ___ ___
//       / __/ _ \| \| \ \ / /__|   \_ _| __| __|
//      | (_| (_) | .` |\ V /___| |) | || _|| _|
//       \___\___/|_|\_| \_/    |___/___|_| |_|  APPLICATION
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Ruben Zorrilla
//

#if !defined(KRATOS_LAPLACIAN_SHIFTED_BOUNDARY_SPLIT_ELEMENT_H_INCLUDED )
#define  KRATOS_LAPLACIAN_SHIFTED_BOUNDARY_SPLIT_ELEMENT_H_INCLUDED

// System includes

// External includes

// Project includes
#include "includes/define.h"
#include "includes/element.h"

// Application includes
#include "laplacian_element.h"

namespace Kratos
{

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

template<std::size_t TDim>
class LaplacianShiftedBoundarySplitElement : public LaplacianElement
{
public:
    ///@name Type Definitions
    ///@{

    /// Counted pointer of LaplacianShiftedBoundarySplitElement
    KRATOS_CLASS_INTRUSIVE_POINTER_DEFINITION(LaplacianShiftedBoundarySplitElement);

    typedef LaplacianElement BaseType;

    static constexpr std::size_t NumNodes = TDim + 1;

    ///@}
    ///@name Life Cycle
    ///@{

    /// Default constructor.
    LaplacianShiftedBoundarySplitElement(
        IndexType NewId,
        GeometryType::Pointer pGeometry);

    LaplacianShiftedBoundarySplitElement(
        IndexType NewId,
        GeometryType::Pointer pGeometry,
        PropertiesType::Pointer pProperties);

    /// Destructor.
    virtual ~LaplacianShiftedBoundarySplitElement();

    ///@}
    ///@name Operators
    ///@{


    ///@}
    ///@name Operations
    ///@{

    Element::Pointer Create(
        IndexType NewId,
        NodesArrayType const& ThisNodes,
        PropertiesType::Pointer pProperties) const override;

    Element::Pointer Create(
        IndexType NewId,
        GeometryType::Pointer pGeom,
        PropertiesType::Pointer pProperties) const override;

    void CalculateLocalSystem(
        MatrixType& rLeftHandSideMatrix,
        VectorType& rRightHandSideVector,
        const ProcessInfo& rCurrentProcessInfo) override;

    void CalculateLeftHandSide(
        MatrixType& rLeftHandSideMatrix,
        const ProcessInfo& rCurrentProcessInfo) override;

    void CalculateRightHandSide(
        VectorType& rRightHandSideVector,
        const ProcessInfo& rCurrentProcessInfo) override;

    int Check(const ProcessInfo& rCurrentProcessInfo) const override;

    ///@}
    ///@name Access
    ///@{


    ///@}
    ///@name Inquiry
    ///@{


    ///@}
    ///@name Input and output
    ///@{


    ///@}
    ///@name Friends
    ///@{


    ///@}

protected:
    ///@name Protected static Member Variables
    ///@{


    ///@}
    ///@name Protected member Variables
    ///@{


    ///@}
    ///@name Protected Operators
    ///@{


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

    // Protected default constructor necessary for serialization
    LaplacianShiftedBoundarySplitElement() : LaplacianElement()
    {
    }

    ///@}

private:
    ///@name Static Member Variables
    ///@{


    ///@}
    ///@name Member Variables
    ///@{


    ///@}
    ///@name Serialization
    ///@{

    friend class Serializer;

    void save(Serializer& rSerializer) const override
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS(rSerializer, LaplacianElement);
    }

    void load(Serializer& rSerializer) override
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS(rSerializer, LaplacianElement);
    }

    ///@}
    ///@name Private Operators
    ///@{


    ///@}
    ///@name Private Operations
    ///@{

    std::vector<std::size_t> GetSurrogateFacesIds();

    ///@}
    ///@name Private  Access
    ///@{


    ///@}
    ///@name Private Inquiry
    ///@{


    ///@}
    ///@name Un accessible methods
    ///@{

    /// Assignment operator.
    //LaplacianShiftedBoundarySplitElement& operator=(const LaplacianShiftedBoundarySplitElement& rOther);

    /// Copy constructor.
    //LaplacianShiftedBoundarySplitElement(const LaplacianShiftedBoundarySplitElement& rOther);

    ///@}

}; // Class LaplacianShiftedBoundarySplitElement

///@}

///@name Type Definitions
///@{


///@}
///@name Input and output
///@{


/// input stream function
/*  inline std::istream& operator >> (std::istream& rIStream,
				    LaplacianShiftedBoundarySplitElement& rThis);
*/
/// output stream function
/*  inline std::ostream& operator << (std::ostream& rOStream,
				    const LaplacianShiftedBoundarySplitElement& rThis)
    {
      rThis.PrintInfo(rOStream);
      rOStream << std::endl;
      rThis.PrintData(rOStream);

      return rOStream;
    }*/
///@}

}  // namespace Kratos.

#endif // KRATOS_LAPLACIAN_SHIFTED_BOUNDARY_SPLIT_ELEMENT_H_INCLUDED  defined
