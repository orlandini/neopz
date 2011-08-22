/**
 * \file
 * @brief Contains the TPZDiscontinuousGalerkin class which implements the interface for discontinuous Galerkin formulations.
 */
// $Id: pzdiscgal.h,v 1.20 2009-11-04 14:04:43 fortiago Exp $
#ifndef TPZDISCGALHPP
#define TPZDISCGALHPP

#include <iostream>
#include "pzmaterial.h"
#include "pzfmatrix.h"
#include "pzvec.h"

class TPZMaterialData;


/**
 * @ingroup material
 * @brief Defines the interface which material objects need to implement for discontinuous Galerkin formulations
 */
class TPZDiscontinuousGalerkin  : public TPZMaterial {
	
	
	public :
	/** @brief Simple constructor */
	TPZDiscontinuousGalerkin();
	/** @brief Constructor with the index of the material object within the vector */
	TPZDiscontinuousGalerkin(int nummat);
	
	/** @brief Copy constructor */
	TPZDiscontinuousGalerkin(const TPZDiscontinuousGalerkin &copy);
	/** @brief Destructor */
	virtual ~TPZDiscontinuousGalerkin();
	
	virtual std::string Name();
	
	/** 
	 * @brief Fill material data parameter with necessary requirements for the ContributeInterface method.
     * @since April 10, 2007
	 */
	/** 
	 * Here, in base class, all requirements are considered as necessary. \n
	 * Each derived class may optimize performance by selecting only the necessary data.
	 */
	virtual void FillDataRequirementsInterface(TPZMaterialData &data);
	
	/**
	 * @brief It computes a contribution to stiffness matrix and load vector at one integration point
	 * @param data [in]
	 * @param weight [in]
	 * @param ek [out] is the stiffness matrix
	 * @param ef [out] is the load vector
	 * @since April 16, 2007
	 */
	virtual void ContributeInterface(TPZMaterialData &data, REAL weight, TPZFMatrix &ek, TPZFMatrix &ef) = 0;
	
	/**
	 * @brief It computes a contribution to residual vector at one integration point
	 * @param data [in]
	 * @param weight [in]
	 * @param ef [out] is the load vector
	 * @since April 16, 2007
	 */
	virtual void ContributeInterface(TPZMaterialData &data, REAL weight, TPZFMatrix &ef);
	
	/**
	 * @brief It computes a contribution to stiffness matrix and load vector at one BC integration point
	 * @param data [in]
	 * @param weight [in]
	 * @param ek [out] is the stiffness matrix
	 * @param ef [out] is the load vector
	 * @param bc [in] is the boundary condition object
	 * @since April 16, 2007
	 */
	virtual void ContributeBCInterface(TPZMaterialData &data, REAL weight, TPZFMatrix &ek,TPZFMatrix &ef,TPZBndCond &bc) = 0;
	
	/**
	 * @brief It computes a contribution to residual vector at one BC integration point
	 * @param data [in]
	 * @param weight [in]
	 * @param ef [out] is the load vector
	 * @param bc [in] is the boundary condition object
	 * @since April 16, 2007
	 */
	virtual void ContributeBCInterface(TPZMaterialData &data, REAL weight, TPZFMatrix &ef,TPZBndCond &bc);
	
	/**
	 * @brief Dicontinuous galerkin materials implement contribution of discontinuous elements and interfaces.
	 * @since Feb 05, 2004
	 */
	/** 
	 * Interfaces may be conservative or not conservative. It is important to agglomeration techniques
	 * when using multigrid pre-conditioner. \n Conservative interfaces into agglomerate elements do not
	 * need to be computed. However non-conservative interfaces must be computed in all multigrid levels.\n
	 * Default is non-conservative, because of the computation of a conservative interface into an agglomerate
	 * does not ruin the solution.
	 */
	virtual int IsInterfaceConservative();
	
	/** 
	 * @brief Computes interface jump = leftu - rightu
	 * @since Feb 14, 2006
	 */
	virtual void InterfaceJump(TPZVec<REAL> &x, TPZVec<REAL> &leftu,TPZVec<REAL> &rightu,TPZVec<REAL> &jump);
	
	
	/** 
	 * @brief Computes interface jump from element to Dirichlet boundary condition.
	 * It has to reimplemented
	 * @since Mar 08, 2006
	 */
	virtual void BCInterfaceJump(TPZVec<REAL> &x, TPZVec<REAL> &leftu,TPZBndCond &bc,TPZVec<REAL> & jump);
	
	
	virtual int NStateVariables() = 0;
	
	
	virtual void ContributeInterfaceErrors(TPZMaterialData &data,
										   REAL weight,
										   TPZVec<REAL> &nkL,
										   TPZVec<REAL> &nkR,
										   int &errorid){
		PZError << "Method not implemented\n";
	}
	
	virtual void ContributeInterfaceBCErrors(TPZMaterialData &data,
											 REAL weight,
											 TPZVec<REAL> &nk,
											 TPZBndCond &bc,
											 int &errorid){
		PZError << "Method not implemented\n";
	}
	
	/** @brief Unique identifier for serialization purposes */
	virtual int ClassId() const;
	
	/** @brief Saves the element data to a stream */
	virtual void Write(TPZStream &buf, int withclassid);
	
	/** @brief Reads the element data from a stream */
	virtual void Read(TPZStream &buf, void *context);

};

#endif
