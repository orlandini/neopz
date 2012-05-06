/**
 * \file
 * @brief Contains the TPZMultCamada class.
 */

class TPZMatPlaca2;
class TPZBndCond;

#ifndef TPZMULTCAMADA_H
#define TPZMULTCAMADA_H
#include "pzmaterial.h"
#include "pzstack.h"

/**
 * @ingroup material
 * @brief DESCRIBE PLEASE
 */
class TPZMultCamada : public TPZMaterial {
public:
	/** @brief Default constructor */	
	TPZMultCamada(int matindex) : TPZMaterial(matindex), fCamadas() {}

	/** @brief Add layer */
    void AddLayer(TPZMatPlaca2 * l) { fCamadas.Push(l); }

	/**
	 * @name Contribute methods (weak formulation)
	 * @{
	 */
	 
    /** @brief Computes contribution to the stiffness matrix and right hand side at an integration point */
    virtual void Contribute(TPZMaterialData &data,
							REAL weight,
							TPZFMatrix<REAL> & ek,
							TPZFMatrix<REAL> & ef);
	
    virtual void ContributeBC(TPZMaterialData &data,
							  REAL weight,
							  TPZFMatrix<REAL> & ek,
							  TPZFMatrix<REAL> & ef,
							  TPZBndCond & bc);
	
    virtual void Contribute(TPZMaterialData &data,
							REAL weight,
							TPZFMatrix<REAL> & ef)
	{
		TPZMaterial::Contribute(data,weight,ef);
    }
	
	virtual void ContributeBC(TPZMaterialData &data,
							  REAL weight,
							  TPZFMatrix<REAL> & ef,
							  TPZBndCond & bc)
	{
		TPZMaterial::ContributeBC(data,weight,ef,bc);
    }

	/** @} */
	
    virtual int VariableIndex(const std::string &name);
	
    virtual int NSolutionVariables(int var);

protected:
	/** @brief Returns the solution associated with the var index based on the finite element approximation */
	virtual void Solution(TPZVec < REAL > & Sol, TPZFMatrix<REAL> & DSol, TPZFMatrix<REAL> & axes, int var, TPZVec < REAL > & Solout);
public:
	virtual void Solution(TPZMaterialData &data, int var, TPZVec < REAL > & Solout)
	{
        int numbersol = data.sol.size();
        if (numbersol != 1) {
            DebugStop();
        }

        Solution(data.sol[0],data.dsol[0],data.axes,var,Solout);
    }
	
    /** @brief Returns the number of state variables associated with the material */
    virtual int NStateVariables();
	
	/** @brief Returns the integrable dimension of the material */
    virtual int Dimension() {return 2;}

private:
	/** @brief Vector of layers */
    TPZStack < TPZMatPlaca2 * > fCamadas;

};

#endif  //TPZMULTCAMADA_H