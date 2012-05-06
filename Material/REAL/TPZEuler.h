/**
 * \file
 * @brief Contains the TPZEuler class which implements a a linear scalar convection equation.
 */

template<class TVar>
class TPZFMatrix;
class TPZBndCond;
class TPZEuler;

#ifndef TPZEULER_H
#define TPZEULER_H
#include "pzmaterial.h"
#include "eulerdif.h"

/**
 * @ingroup material
 * @brief This class implements a linear scalar convection equation with modified 
 * SUPG difusion
 */
class TPZEuler : public TPZMaterial {
public:  
	/** @brief Copy constructor */
	TPZEuler(TPZEuler & copy);
	/** @brief Simple constructor */
    TPZEuler(int id, REAL deltat) ;
	
	/** 
	 * @brief Set the state of the Euler material
	 * @param state \f$ state = 0 \f$ -> L2 projection. \f$ state = 1 \f$ -> Euler time stepping
	 */
	void SetState(int state) {
		fState = state;
	}
	
	
    /** @brief Returns the integrable dimension of the material */
    virtual int Dimension() ;
	
    /** @brief Returns the number of state variables associated with the material */
    virtual int NStateVariables()  ;
	
    /** @brief Return the number of components which form the flux function */
    virtual int NFluxes() {return 2;}
	
	/**
	 * @name Contribute methods (weak formulation)
	 * @{
	 */
	 
    /** @brief Computes contribution to the stiffness matrix and right hand side at an integration point */
	virtual void Contribute(TPZMaterialData &data, REAL weight,
							TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef) ;
    /** @brief Computes contribution to the right hand side at an integration point */
	virtual void Contribute(TPZMaterialData &data, REAL weight,
							TPZFMatrix<REAL> &ef)
	{
        TPZMaterial::Contribute(data,weight,ef);
    }

    virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc);
	virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<REAL> &ef,TPZBndCond &bc)
	{
		TPZMaterial::ContributeBC(data,weight,ef,bc);
    }

	/** @} */
	
    /** @brief Print out the data associated with the material */
    virtual void Print(std::ostream &out = std::cout);
	
    /** @brief Returns the variable index associated with the name */
    virtual int VariableIndex(const std::string &name);
	
    virtual int NSolutionVariables(int var);
protected:
	virtual void Solution(TPZVec<REAL> &Sol,TPZFMatrix<REAL> &DSol,TPZFMatrix<REAL> &axes,int var,TPZVec<REAL> &Solout);
public:
	virtual void Solution(TPZMaterialData &data,int var,TPZVec<REAL> &Solout)
	{
        int numbersol = data.sol.size();
        if (numbersol != 1) {
            DebugStop();
        }

		Solution(data.sol[0],data.dsol[0],data.axes,var,Solout);
    }
	
	
    virtual void Flux(TPZVec<REAL> &x, TPZVec<REAL> &Sol, TPZFMatrix<REAL> &DSol, TPZFMatrix<REAL> &axes, TPZVec<REAL> &flux) {}
	
    /** @brief To create another material of the same type */
    virtual TPZMaterial * NewMaterial();
	
    /** @brief Reads data of the material from a istream (file data) */
    virtual void SetData(std::istream &data);
	
private:    
	
	static TEulerDiffusivity gEul;
	REAL fDeltaT;
	int fState;
};

#endif