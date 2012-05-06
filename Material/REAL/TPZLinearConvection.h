/**
 * \file
 * @brief Contains the TPZLinearConvection class which implements a linear scalar convection equation.
 */

template<class TVar>
class TPZFMatrix;
class TPZBndCond;

#ifndef TPZLINEARCONVECTION_H
#define TPZLINEARCONVECTION_H
#include "pzmaterial.h"

/**
 * @ingroup material
 * @brief Implements a linear scalar convection equation with modified SUPG difusion
 */
class TPZLinearConvection : public TPZMaterial {
public:
	/** @brief Copy constructor */
    TPZLinearConvection(TPZLinearConvection & copy);
	/** @brief Constructor for given convection */
    TPZLinearConvection(int id,TPZVec<REAL> &conv) ;

    /** @brief Returns the integrable dimension of the material */
    virtual int Dimension() ;
	
    /** @brief Returns the number of state variables associated with the material */
    virtual int NStateVariables()  ;
	
    /** @brief Returns the number of components which form the flux function */
    virtual int NFluxes() {return 2;}

	/**
	 * @name Contribute methods (weak formulation)
	 * @{
	 */
	 
    virtual void Contribute(TPZMaterialData &data, REAL weight,
							TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef);
	
	
    virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc);
	
    virtual void Contribute(TPZMaterialData &data, REAL weight,
							TPZFMatrix<REAL> &ef)
	{
		TPZMaterial::Contribute(data,weight,ef);
	}
	
	
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
    REAL fConvect[2];

};

#endif //TPZLINEARCONVECTION_H