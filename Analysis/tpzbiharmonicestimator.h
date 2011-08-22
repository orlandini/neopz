/**
 * @file
 * @brief Contains TPZBiharmonicEstimator class estimates error to biharmonic problem.
 */
/***************************************************************************
 *   Copyright (C) 2008 by joao,,,   *
 *   joao@joao-laptop   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TPZBIHARMONICESTIMATOR_H
#define TPZBIHARMONICESTIMATOR_H
#include "pzbiharmonic.h"

/**
 * @brief Estimates error to biharmonic problem. Also computes the contributions on elements and interfaces. \ref analysis "Analysis"
 * @author Joao Luis Goncalves
 * @since Maio 16, 2008
 * @ingroup material
 * @note This class seems as material class, must to be put into the material group (Jorge??)
 */
class TPZBiharmonicEstimator: public  TPZBiharmonic
{
private:
	/** @brief Attributes required for goal oriented error estimation validation */
	
	void (*fPrimalExactSol)(TPZVec<REAL> &loc,TPZVec<REAL> &val,TPZFMatrix &deriv);
	void (*fDualExactSol)(TPZVec<REAL> &loc,TPZVec<REAL> &val,TPZFMatrix &deriv);
	
public:
    TPZBiharmonicEstimator(int nummat, REAL f);
	
    ~TPZBiharmonicEstimator();
	
    void SetExactSolutions(void (*fp)(TPZVec<REAL> &loc,TPZVec<REAL> &val,TPZFMatrix &deriv),
                           void (*fd)(TPZVec<REAL> &locdual,TPZVec<REAL> &valdual,TPZFMatrix &derivdual));
	
	/**
	 * @brief Returns the number of norm errors. Default is 3: energy, L2 and H1.
	 */
	virtual int NEvalErrors() {return 4;}
	
	/** @brief Implements integration of the internal part of an error estimator.
	 * 
	 * It performs nk[0] += weight * ( residuo(u) *(Z1-Z) ); \n
	 * where u is the current solution and Z and Z1 are the dual solution.
	 */
	virtual void ContributeErrorsDual(TPZMaterialData &data,
									  REAL weight,
									  TPZVec<REAL> &nk);
	
	virtual void ContributeErrors(TPZMaterialData &data,
								  REAL weight,
								  TPZVec<REAL> &nk,
								  int &errorid)
	{
		if (errorid == 0) this->ContributeErrorsDual(data,weight,nk);
		if (errorid == 2) this->ContributeErrorsSimple(data,weight,nk);
	}
	
	/** @brief Implements integration of the interface part of an error estimator.
	 * 
	 * It performs \f$ nk[0] += weight * ( residuo(u )*(Z1-Z) ) \f$ ; \n
	 * where u is the current solution and Z and Z1 are the dual solution.
	 */
	virtual void ContributeInterfaceErrors(TPZMaterialData &data, REAL weight,
										   TPZVec<REAL> &nkL, TPZVec<REAL> &nkR,
										   int &errorid)
	{
		if (errorid == 0) this->ContributeInterfaceErrorsDual(data,weight,nkL,nkR);
		if (errorid == 2) this->ContributeInterfaceErrorsSimple(data,weight,nkL,nkR);
	}
	
	virtual void ContributeInterfaceErrorsDual(TPZMaterialData &data,
											   REAL weight,
											   TPZVec<REAL> &nkL, 
											   TPZVec<REAL> &nkR);
	
	/** @brief Implements integration of the boundary interface part of an error estimator. */
	/** 
	 * It performs \f$ nk[0] += weight * ( residuo(u ) * (Z1-Z) ) \f$ ; \n
	 * where u is the current solution and Z and Z1 are the dual solution.
	 */
	virtual void ContributeInterfaceBCErrorsDual(TPZMaterialData &data,
												 REAL weight,
												 TPZVec<REAL> &nk, 
												 TPZBndCond &bc);
	
	virtual void ContributeInterfaceBCErrors(TPZMaterialData &data,
											 REAL weight,
											 TPZVec<REAL> &nk,
											 TPZBndCond &bc,
											 int &errorid)
	{
		if (errorid == 0) this->ContributeInterfaceBCErrorsDual(data,weight,nk,bc);
		if (errorid == 2) this->ContributeInterfaceBCErrorsSimple(data,weight,nk,bc);
	}
	
	virtual void ContributeErrorsSimple(TPZMaterialData &data,
										REAL weight,
										TPZVec<REAL> &nk);
	
	
	virtual void ContributeInterfaceErrorsSimple(TPZMaterialData &data,
												 REAL weight,
												 TPZVec<REAL> &nkL, 
												 TPZVec<REAL> &nkR);
	
	
	virtual void ContributeInterfaceBCErrorsSimple(TPZMaterialData &data,
												   REAL weight,
												   TPZVec<REAL> &nk,
												   TPZBndCond &bc);

    /**
	 * @brief Compute the error due to the difference between the interpolated flux \n
	 * and the flux computed based on the derivative of the solution
	 */
	void Errors(TPZVec<REAL> &x,TPZVec<REAL> &u, TPZFMatrix &dudx,
				TPZFMatrix &axes, TPZVec<REAL> & /*flux*/ ,
				TPZVec<REAL> &u_exact,TPZFMatrix &du_exact,
				TPZVec<REAL> &values);
	
	void Psi(TPZVec<REAL> &x, TPZVec<REAL> &pisci);
	
	void OrderSolution(TPZMaterialData &data);
	void OrderSolution2(TPZMaterialData &data);
	void OrderSolutionRight(TPZMaterialData &data);
	void OrderSolutionLeft(TPZMaterialData &data);
	
};

#endif
