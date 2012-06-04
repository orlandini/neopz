//
//  mixedpoisson.h
//  PZ
//
//  Created by Agnaldo Farias on 5/28/12.
//  Copyright (c) 2012 LabMec-Unicamp. All rights reserved.
//

#ifndef PZ_mixedpoisson_h
#define PZ_mixedpoisson_h

#include "pzdiscgal.h"

/**
 * @ingroup material
 * @author Agnaldo Farias
 * @since 5/28/2012
 * @brief Material to solve a mixed poisson 2d
 */
/**
 * \f$ Q = -k*grad(p)  ==> Int{Q.q}dx - k*Int{p*div(q)}dx + k*Int{p*Dq.n}ds = 0  (Eq. 1)  \f$ 
 *
 * \f$ div(Q) = f  ==> Int{div(Q)*v}dx = Int{f*v}dx (Eq. 2) \f$ 
 *
 * \f$ p = pD in Dirichlet boundary and Q.n = qN in Neumann boundary\f$
 */


class TPZMixedPoisson : public TPZDiscontinuousGalerkin {
    
protected:
	/** @brief Forcing function value */
	REAL ff;
	
	/** @brief Coeficient which multiplies the gradient operator */
	REAL fk;
    
    /** @brief Problem dimension */
	int fDim;
    
public:
    TPZMixedPoisson();
    
    TPZMixedPoisson(int matid);
    
    ~ TPZMixedPoisson();
    
    virtual void Print(std::ostream & out);
	
	virtual std::string Name() { return "TPZMixedPoisson"; }
    
    virtual int NStateVariables();
	
	void SetParameters(REAL dif){
		fk = dif;
	}
	
	void GetParameters(REAL &dif){
		dif = fk;
	}
	
	void SetInternalFlux(REAL flux){
		ff = flux;
	}
    
    /**
     * @brief It computes a contribution to the stiffness matrix and load vector at one integration point to multiphysics simulation.
     * @param datavec [in] stores all input data
     * @param weight [in] is the weight of the integration rule
     * @param ek [out] is the stiffness matrix
     * @param ef [out] is the load vector
     */
	virtual void Contribute(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<REAL> &ek, TPZFMatrix<REAL> &ef);
	
	//virtual void ContributeBC(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc);
	
};

#endif