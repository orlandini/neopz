/**
 * @file
 */

#ifndef TPZPlasticStepPV_H
#define TPZPlasticStepPV_H


#include "TPZTensor.h"
#include "pzreal.h"
#include "pzfmatrix.h"
#include "TPZPlasticState.h"
#include "TPZPlasticIntegrMem.h"
#include "TPZPlasticStep.h"
#include "pzlog.h"
#include "pzstepsolver.h"
#include <set>
#include <ostream>

// Metodos para deixar o prog mais "encapsulado"
REAL NormVecOfMat(TPZFNMatrix <9> mat);
REAL InnerVecOfMat(TPZFMatrix<REAL> &m1,TPZFMatrix<REAL> &m2);
TPZFMatrix<REAL> ProdT(TPZFMatrix<REAL> &m1,TPZFMatrix<REAL> &m2);
TPZFNMatrix <6> FromMatToVoight(TPZFNMatrix <9> mat);

template <class TVar>
class TPZFMatrix;

/*
 
 enum EElastoPlastic
 {
 EAuto = 0,
 EForceElastic = 1,
 EForcePlastic = 2
 };
 */

/**
 * @brief Classe que efetua avanco de um passo de plastificacao utilizando o metodo de Newton
 */
template <class YC_t, class ER_t>
class TPZPlasticStepPV : public TPZPlasticBase
{
public:
	
	/**
	 * @brief Constructor which Initialize the plastic material damage variable only
	 *
	 * @param[in] alpha damage variable
	 */
	
  TPZPlasticStepPV(REAL alpha=0.):fYC(), fER(), fResTol(1.e-12), fMaxNewton(30)
	{ 
		fN.fAlpha = alpha; 
	}
	
	/**
	 * @brief Copy Constructor
	 *
	 * @param[in] source of copy
	 */
	TPZPlasticStepPV(const TPZPlasticStepPV & source)
	{
		fYC			= source.fYC;
		fER			= source.fER;
		fResTol		= source.fResTol;
		fMaxNewton = source.fMaxNewton;
		fN			= source.fN;
	}
	
	/**
	 * @brief Operator =
	 *
	 * @param[in] source of copy
	 */
	TPZPlasticStepPV & operator=(const TPZPlasticStepPV & source)
	{
		fYC			= source.fYC;
		fER			= source.fER;
		fResTol		= source.fResTol;
		fMaxNewton = source.fMaxNewton;
		fN			= source.fN;

		return *this;
	}
	
	/**
	 * @brief Name of the class ina string
	 */
	virtual const char * Name() const
	{
		return "TPZPlasticStepPV";	
	}
	
	virtual void Print(std::ostream & out) const
	{
		out << "\n" << this->Name();
		out << "\n YC_t:";
		//fYC.Print(out); FAZER O PRINT
		out << "\n ER_t:";
		fER.Print(out);
		out << "\nTPZPlasticStepPV Internal members:";
		out << "\n fResTol = " << fResTol;
		out << "\n fMaxNewton = " << fMaxNewton;
		out << "\n fN = "; // PlasticState
		fN.Print(out);
	}
		
	typedef YC_t fNYields;
	
	/**
	 * Imposes the specified strain tensor, evaluating the plastic integration if necessary.
	 *
	 * @param[in] epsTotal Imposed total strain tensor
	 */
    virtual void ApplyStrain(const TPZTensor<REAL> &epsTotal);
    
	/**
	 * Imposes the specified strain tensor and returns the correspondent stress state.
	 *
	 * @param[in] epsTotal Imposed total strain tensor
	 * @param[out] sigma Resultant stress
	 */	
	virtual void ApplyStrainComputeSigma(const TPZTensor<REAL> &epsTotal, TPZTensor<REAL> &sigma);
    
    
    
    //virtual void ApplySigmaComputeStrain(const TPZTensor<REAL> &sigma, TPZTensor<REAL> &epsTotal);
	
	/**
	 * Imposes the specified strain tensor and returns the corresp. stress state and tangent
	 * stiffness matrix.
	 *
	 * @param[in] epsTotal Imposed total strain tensor
	 * @param[out] sigma Resultant stress
	 * @param[out] Dep Incremental constitutive relation
	 */
	virtual void ApplyStrainComputeDep(const TPZTensor<REAL> &epsTotal, TPZTensor<REAL> &sigma, TPZFMatrix<REAL> &Dep);
    
    /**
	 * Attempts to compute an epsTotal value in order to reach an imposed stress state sigma.
	 * This method should be used only for test purposes because it isn't fully robust. Some
	 * materials, specially those perfectly plastic and with softening, may fail when applying
	 * the Newton Method on ProcessLoad.
	 *
	 * @param[in] sigma stress tensor
	 * @param[out] epsTotal deformation tensor
	 */
    virtual void ApplyLoad(const TPZTensor<REAL> & sigma, TPZTensor<REAL> &epsTotal);
    
    virtual TPZPlasticState<REAL> GetState() const;
    /**
     * @brief Return the value of the yield functions for the given strain
     * @param[in] epsTotal strain tensor (total strain)
     * @param[out] phi vector of yield functions
	 */
	virtual void Phi(const TPZTensor<REAL> &epsTotal, TPZVec<REAL> &phi) const;

    
    
    /**
	 * @brief Update the damage values
	 * @param[in] state Plastic state proposed
	 */
    virtual void SetState(const TPZPlasticState<REAL> &state);
    
    /** @brief Sets the tolerance allowed in the pde integration */
    virtual void SetIntegrTol(REAL integrTol);
    
    virtual void SetTensionSign(int sign);
    
    
    void CopyFromFNMatrixToTensor(TPZFNMatrix<6> FNM,TPZTensor<STATE> &copy);
    

    void CopyFromTensorToFNMatrix(TPZTensor<STATE> tensor,TPZFNMatrix<6> &copy);
    
    int SignCorrection() const;
    
    
    virtual void Read(TPZStream &buf);
    
	
	/**
	 * Does the TaylorCheck of the tangent matrix
	 *
	 * @param[in] epsTotal Imposed total strain tensor
	 * @param[out] sigma Resultant stress
	 * @param[out] Dep Incremental constitutive relation
	 */
	void TaylorCheck(TPZTensor<REAL> &EpsIni, TPZTensor<REAL> &deps, REAL kprev, TPZVec<REAL> &conv);
	
	REAL ComputeNFromTaylorCheck(REAL alpha1, REAL alpha2, TPZFMatrix<REAL> &error1Mat, TPZFMatrix<REAL> &error2Mat);
	
	/**
	 * Defines whether the tension/extension sign is desired by the external user.
	 *
	 * @param s [in] sign (1 or -1)
	 */
	//void SetTensionSign(int s);
	
	
protected:
	

	
public:
	/**
	 * @brief Return the value of the yield functions for the given strain
	 * @param[in] epsTotal strain tensor (total strain)
	 * @param[out] phi vector of yield functions
	 */
	//virtual void Phi(const TPZTensor<REAL> &epsTotal, TPZVec<REAL> &phi) const;
	
		
	/**
	 * @brief Update the damage values
	 * @param[in] state Plastic state proposed
	 */
	//virtual void SetState(const TPZPlasticState<REAL> &state);

	
	/** @brief Retrieve the plastic state variables */	
	//virtual const TPZPlasticState<REAL> GetState()const;
	
	/** @brief Return the number of plastic steps in the last load step. Zero indicates elastic loading. */
	virtual int IntegrationSteps() const
	{
		return 1;
	}
	

protected:	
	
	/**
	 * @brief Updates the damage values - makes no interface sign checks
	 * @param[in] state Plastic state proposed
	 */
	//virtual void SetState_Internal(const TPZPlasticState<REAL> &state);




public:
	
	void SetResidualTolerance(STATE tol)
	{
		fResTol = tol;
	}
	
	//virtual void Write(TPZStream &buf) const;
	
	//virtual void Read(TPZStream &buf);
public:
	
	/** @brief Object which represents the yield criterium */
	YC_t fYC;

	/** @brief Object representing the elastic response */
	ER_t fER;
	
	
protected:
	
	/** @brief Residual tolerance accepted in the plastic loop processes */
	REAL fResTol;
	
	/** @brief Maximum number of Newton interations allowed in the nonlinear solvers */
	int fMaxNewton;	// COLOCAR = 30 (sugestao do erick!)
    
    /** @brief The tension sign in the convention used in the implementation of the material */
	int fMaterialTensionSign=-1;
	
	/** @brief The tension sign in the convention defined by the external user */
	int fInterfaceTensionSign;
	
	
public:
	
	/** @brief Plastic State Variables (EpsT, EpsP, Alpha) at the current time step */	
	TPZPlasticState<REAL> fN;
	
	/**
	 * @brief Stores the plastic evolution in the last evaluated PlasticIntegration call,
	 * It includes the N-1 data, the elastic step until yield when it exists,
	 * the plastic substeppings and the N step.
	 */
	//TPZStack< TPZPlasticIntegrMem<REAL, YC_t::NYield> > fPlasticMem;

	
	//ofstream fOutfile(string &str);
    
    
    
	
	
};

#endif //TPZPlasticStepPV_H
