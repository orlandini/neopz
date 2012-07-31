/**
 * @file
 * @brief Contains the TPZMatPoisson3d class.
 */

#ifndef MATPOISSON3DH
#define MATPOISSON3DH

#include <iostream>
#include "pzdiscgal.h"
#include "pzfmatrix.h"


#ifdef _AUTODIFF
#include "fadType.h"
#endif

/**
 * @ingroup material
 * @brief DESCRIBE PLEASE
 */
/**
 * \f$ -fK Laplac(u) + fC * div(fConvDir*u) = - fXf  \f$
 */
class TPZMatPoisson3d : public TPZDiscontinuousGalerkin {
	
	protected :
	
	/** @brief Forcing function value */
	STATE fXf;
	
	/** @brief Problem dimension */
	int fDim;
	
	/** @brief Coeficient which multiplies the Laplacian operator. */
	STATE fK;
	
	/** @brief Coeficient which multiplies the Laplacian operator associated to right neighbour element of the interface */
	/** 
	 * The coefficient of left neighbour is fK. \n
	 * It is for use with discontinuous Galerkin method. Default value for fRightK is fK.
	 */
	STATE fRightK;
	
	/** @brief Variable which multiplies the convection term of the equation */
	REAL fC;
	
	/** @brief Direction of the convection operator */
	REAL fConvDir[3];
	
	/** @brief Symmetry coefficient of elliptic term */
	/** 
	 * Symmetrical formulation - Global element method - has coefficient = -1. \n
	 * Non-symmetrical formulation - Baumann's formulation - has coefficient = +1.
	 */
	REAL fSymmetry;
	
	/** @brief Multiplication value for the streamline diffusion term */
	STATE fSD;
	
	/** @brief Enumerate for penalty term definitions */
	enum EPenaltyType {ENoPenalty = 0, EFluxPenalty = 1, ESolutionPenalty, EBoth};
	
	/** @brief Penalty term definition */
	EPenaltyType fPenaltyType;
	
public:
	
	/** @brief Constant multiplyer of penalty term, when required is set. */
	REAL fPenaltyConstant;
	
	/** @brief Defines no penalty terms in ContributeInterface */
	void SetNoPenalty(){ this->fPenaltyType = ENoPenalty;}
	
	/** @brief Defines flux penalty terms in ContributeInterface */
	void SetFluxPenalty(){ this->fPenaltyType = EFluxPenalty; }
	
	/** @brief Defines solution penalty terms in ContributeInterface */
	void SetSolutionPenalty(){ this->fPenaltyType = ESolutionPenalty; }
	
	/** @brief Defines solution and flux penalty terms in ContributeInterface */
	void SetBothPenalty(){ this->fPenaltyType = EBoth; }
	
	/** @brief Using in InterfaceErrors */
	static STATE gAlfa;
	
	TPZMatPoisson3d(int nummat, int dim);
	
	TPZMatPoisson3d();
	
	TPZMatPoisson3d(const TPZMatPoisson3d &copy);
	
	virtual ~TPZMatPoisson3d();
	
	TPZMatPoisson3d &operator=(const TPZMatPoisson3d &copy);
	
	/** @brief Set material elliptic term as the global element method, i.e. the symmetrical formulation */
	void SetSymmetric(){
		this->fSymmetry = -1.0;
	}
	
	/** @brief Set material elliptic term as the Baumann's formulation, i.e. the non-symmetrical formulation */
	void SetNonSymmetric() {
		this->fSymmetry = +1.0;
	}
	
	bool IsSymetric(){
		if (fSymmetry == -1.0) return true;
		if (fSymmetry == +1.0) return false;
		PZError << __PRETTY_FUNCTION__ << "\n Comparacao de numeros reais da errado\n";
		return false;
	}
	
	virtual TPZMaterial * NewMaterial(){
		return new TPZMatPoisson3d(*this);
	}
	
	int Dimension() { return fDim;}
	
	int NStateVariables();
	
	void SetParameters(STATE diff, REAL conv, TPZVec<REAL> &convdir);
	
	void GetParameters(STATE &diff, REAL &conv, TPZVec<REAL> &convdir);
	
	void SetInternalFlux(STATE flux)
	{
		fXf = flux;
	}
	
	void SetSD(STATE sd)
	{
		fSD = sd;
	}
	
	/**
	 * @brief Define fK of right neighbour element. \n 
	 * It is used with discontinuous Galerkin on the computation of ContributeInterface methods */
	/**
	 * Attention that method SetParameters override the modifications of this method. \n
	 * Then call it after SetParameters and never before or it will have no effect.
	 */
	void SetRightK(STATE rightK) {
		this->fRightK = rightK;
	}
	STATE GetRightK(){
		return this->fRightK;
	}
	
	virtual void Print(std::ostream & out);
	
	virtual std::string Name() { return "TPZMatPoisson3d"; }

	/**
	 * @name Contribute methods (weak formulation)
	 * @{
	 */
	 
	virtual void Contribute(TPZMaterialData &data,REAL weight,
							TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef);
	virtual void ContributeBCHDiv(TPZMaterialData &data,REAL weight,
								  TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc);
	virtual void ContributeHDiv(TPZMaterialData &data,REAL weight,TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef);
	
	virtual void Contribute(TPZMaterialData &data,REAL weight,
							TPZFMatrix<STATE> &ef)
	{
		TPZDiscontinuousGalerkin::Contribute(data,weight,ef);
	}
#ifdef _AUTODIFF
	/** @brief Computes contribution to the energy at an integration point */
	void ContributeEnergy(TPZVec<REAL> &x,
						  TPZVec<FADFADREAL> &sol,
						  TPZVec<FADFADREAL> &dsol,
						  FADFADREAL &U,
						  REAL weight);
#endif
	
	virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc);
	
	virtual void ContributeBC(TPZMaterialData &data,REAL weight,
							  TPZFMatrix<STATE> &ef,TPZBndCond &bc)
	{
		TPZDiscontinuousGalerkin::ContributeBC(data,weight,ef,bc);
	}

	virtual void ContributeBCInterface(TPZMaterialData &data, TPZMaterialData &dataleft, REAL weight,TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc);
	
	virtual void ContributeInterface(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, REAL weight,
									 TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef);
	
	virtual void ContributeBCInterface(TPZMaterialData &data,TPZMaterialData &dataleft,REAL weight,TPZFMatrix<STATE> &ef,TPZBndCond &bc)
	{
		TPZDiscontinuousGalerkin::ContributeBCInterface(data,dataleft,weight,ef,bc);
	}
	
	virtual void ContributeInterface(TPZMaterialData &data,TPZMaterialData &dataleft,TPZMaterialData &dataright,REAL weight,
									 TPZFMatrix<STATE> &ef)
	{
		TPZDiscontinuousGalerkin::ContributeInterface(data,dataleft,dataright,weight,ef);
	}

#ifdef _AUTODIFF
	
	virtual void ContributeBCEnergy(TPZVec<REAL> & x,
									TPZVec<FADFADREAL> & sol, FADFADREAL &U,
									REAL weight, TPZBndCond &bc);
	
#endif

	/** @} */
	
	virtual int VariableIndex(const std::string &name);
	
	virtual int NSolutionVariables(int var);
	
	virtual int NFluxes(){ return 3;}
	
protected:
	virtual void Solution(TPZVec<STATE> &Sol,TPZFMatrix<STATE> &DSol,TPZFMatrix<REAL> &axes,int var,TPZVec<REAL> &Solout);
public:
	
	virtual void Solution(TPZMaterialData &data, int var, TPZVec<REAL> &Solout);
	
	virtual void Flux(TPZVec<REAL> &x, TPZVec<STATE> &Sol, TPZFMatrix<STATE> &DSol, TPZFMatrix<REAL> &axes, TPZVec<STATE> &flux);
	
	void Errors(TPZVec<REAL> &x,TPZVec<STATE> &u,
				TPZFMatrix<STATE> &dudx, TPZFMatrix<REAL> &axes, TPZVec<STATE> &flux,
				TPZVec<STATE> &u_exact,TPZFMatrix<STATE> &du_exact,TPZVec<REAL> &values);
	void ErrorsHdiv(TPZMaterialData &data,TPZVec<STATE> &u_exact,TPZFMatrix<STATE> &du_exact,TPZVec<REAL> &values);
	
	
	virtual int NEvalErrors() {return 5;}
	
	/**
	 * @brief Compute square of residual of the differential equation at one integration point.
	 * @param X is the point coordinate (x,y,z)
	 * @param sol is the solution vector
	 * @param dsol is the solution derivative with respect to x,y,z as computed in TPZShapeDisc::Shape2DFull
	 */   
	virtual REAL ComputeSquareResidual(TPZVec<REAL>& X, TPZVec<STATE> &sol, TPZFMatrix<STATE> &dsol);
	
	
	void InterfaceErrors(TPZVec<REAL> &/*x*/,
						 TPZVec<STATE> &leftu, TPZFMatrix<STATE> &leftdudx, /* TPZFMatrix<REAL> &leftaxes,*/
						 TPZVec<STATE> &rightu, TPZFMatrix<STATE> &rightdudx, /* TPZFMatrix<REAL> &rightaxes,*/
						 TPZVec<STATE> &/*flux*/,
						 TPZVec<STATE> &u_exact,TPZFMatrix<STATE> &du_exact,TPZVec<REAL> &values,
						 TPZVec<STATE> normal, STATE elsize);
	
	/** 
	 * @brief Computes interface jump from element to Dirichlet boundary condition
	 * @return Returns sol-u_dirichlet
	 * @since Mar 08, 2006
	 */
	virtual void BCInterfaceJump(TPZVec<REAL> &x, TPZSolVec &leftu,TPZBndCond &bc,TPZSolVec & jump);
	
	virtual int IsInterfaceConservative(){ return 1;}
	
	virtual int ClassId() const;
	
	virtual void Write(TPZStream &buf, int withclassid);
	
	virtual void Read(TPZStream &buf, void *context);
	
};

#endif