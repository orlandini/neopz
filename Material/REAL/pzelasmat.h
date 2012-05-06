/**
 * @file
 * @brief Contains the TPZElasticityMaterial class which implements a two dimensional elastic material in plane stress or strain.
 */

#ifndef ELASMATHPP
#define ELASMATHPP

#include <iostream>

#include "pzmaterial.h"
#include "pzdiscgal.h"

/**
 * @ingroup material
 * @brief This class implements a two dimensional elastic material in plane stress or strain
 */
class TPZElasticityMaterial : public TPZDiscontinuousGalerkin {
	
	public :

	/** @brief Default constructor */
	TPZElasticityMaterial();
	/** 
	 * @brief Creates an elastic material with:
	 * @param num material id
	 * @param E elasticity modulus
	 * @param nu poisson coefficient
	 * @param fx forcing function \f$ -x = fx \f$ 
	 * @param fy forcing function \f$ -y = fy \f$
	 * @param plainstress \f$ plainstress = 1 \f$ indicates use of plainstress
	 */
	TPZElasticityMaterial(int num, REAL E, REAL nu, REAL fx, REAL fy, int plainstress = 1);
	
	/** @brief Copies the data of one TPZElasticityMaterial object to another */
	TPZElasticityMaterial(const TPZElasticityMaterial &copy);
	
	/** @brief Creates a new material from the current object   ??*/
	virtual TPZMaterial * NewMaterial() { return new TPZElasticityMaterial(*this);}
	
	/** @brief Default destructor */
	virtual ~TPZElasticityMaterial();
	
	/** @brief Returns the model dimension */
	int Dimension() { return 2;}
	
	/** @brief Returns the number of state variables associated with the material */
	virtual  int NStateVariables();
	
	/** @brief Print the material data*/
	virtual void Print(std::ostream & out = std::cout);
	
	/** @brief Returns the material name*/
	std::string Name() { return "TPZElasticityMaterial"; }
	
	/** @brief Returns the number of components which form the flux function */
	virtual short NumberOfFluxes(){return 3;}
	
	/** @brief Returns the number of components which form the flux function */
	virtual int NFluxes(){ return 3;}
		
	/** @name Contribute methods */
	/** @{ */
	
	/** @brief Calculates the element stiffness matrix */
	virtual void Contribute(TPZMaterialData &data, REAL weight,TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef);
	
	/** @brief Calculates the element stiffness matrix */
	virtual void Contribute(TPZMaterialData &data, REAL weight,TPZFMatrix<STATE> &ef)
	{
		TPZDiscontinuousGalerkin::Contribute(data,weight,ef);
	}
	
	/** @brief Applies the element boundary conditions */
	virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc);
	
	/** @brief Applies the element boundary conditions */
	virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<STATE> &ef,TPZBndCond &bc)
	{
		TPZDiscontinuousGalerkin::ContributeBC(data,weight,ef,bc);
	}
	
	virtual void ContributeInterface(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, REAL weight, TPZFMatrix<STATE> &ek, TPZFMatrix<STATE> &ef){
		PZError << "\nFATAL ERROR - Method not implemented: " << __PRETTY_FUNCTION__ << "\n";
	}
	
	virtual void ContributeBCInterface(TPZMaterialData &data, TPZMaterialData &dataleft, REAL weight, TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc){
		PZError << "\nFATAL ERROR - Method not implemented: " << __PRETTY_FUNCTION__ << "\n";
	}
	
	virtual void ContributeInterface(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, REAL weight, TPZFMatrix<STATE> &ef){
		PZError << "\nFATAL ERROR - Method not implemented: " << __PRETTY_FUNCTION__ << "\n";
	}
	
	virtual void ContributeBCInterface(TPZMaterialData &data, TPZMaterialData &left, REAL weight, TPZFMatrix<STATE> &ef,TPZBndCond &bc){
		PZError << "\nFATAL ERROR - Method not implemented: " << __PRETTY_FUNCTION__ << "\n";
	}
	
	/** @} */
	
	/** @brief Returns the variable index associated with the name */
	virtual int VariableIndex(const std::string &name);
	
	/** 
	 * @brief Returns the number of variables associated with the variable indexed by var.
	 */
	virtual int NSolutionVariables(int var);
	
protected:
	virtual void Solution(TPZVec<STATE> &Sol,TPZFMatrix<STATE> &DSol,TPZFMatrix<REAL> &axes,int var,TPZVec<STATE> &Solout);
public:

    /** @brief Returns the solution associated with the var index based on the finite element approximation */
	virtual void SolutionDisc(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, int var, TPZVec<STATE> &Solout)
	{
		TPZDiscontinuousGalerkin::SolutionDisc(data,dataleft,dataright,var,Solout);
	}

	/** @brief Computes the value of the flux function to be used by ZZ error estimator */
	virtual void Flux(TPZVec<REAL> &x, TPZVec<STATE> &Sol, TPZFMatrix<STATE> &DSol, TPZFMatrix<REAL> &axes, TPZVec<STATE> &flux);
	
	/** 
	 * @brief Computes the error due to the difference between the interpolated flux \n
	 * and the flux computed based on the derivative of the solution
	 */
	void Errors(TPZVec<REAL> &x,TPZVec<STATE> &u,
				TPZFMatrix<STATE> &dudx, TPZFMatrix<REAL> &axes, TPZVec<STATE> &flux,
				TPZVec<STATE> &u_exact,TPZFMatrix<STATE> &du_exact,TPZVec<REAL> &values);//Cedric
	
	/** @brief Returns the elasticity modulus E */
	REAL E() {return fE;}
	
	/** @brief Returns the poison coefficient modulus E */
	REAL Nu() {return fnu;}
	
	/** @brief Set PresStress Tensor */
	void SetPreStress(REAL Sigxx, REAL Sigyy, REAL Sigxy);
	
	virtual int ClassId() const;
	
	virtual void Read(TPZStream &buf, void *context);
	
	virtual void Write(TPZStream &buf, int withclassid);
	
	
	
private:
	/** @brief Elasticity modulus */
	REAL fE;
	
	/** @brief Poison coeficient */
	REAL fnu;
	
	/** @brief Forcing vector */
	REAL ff[3];
	
	/** @brief \f$ G = E/2(1-nu) \f$ */
	REAL fEover21PlusNu;
	
	/** @brief \f$ E/(1-nu) \f$ */
	REAL fEover1MinNu2;
	
	/** @brief Pre Stress Tensor - Sigma XX */
	REAL fPreStressXX;
	
	/** @brief Pre Stress Tensor - Sigma YY */
	REAL fPreStressYY;
	
	/** @brief Pre Stress Tensor - Sigma XY */
	REAL fPreStressXY;
	
	/** @brief Uses plain stress */
	int fPlaneStress;
};

#endif