/**
 * \file
 * @brief Contains the TPZSwelling class which implements a numerical model of swelling material coupling flow.
 */
#ifndef SWELLINGHPP
#define SWELLINGHPP

#include "pzmaterial.h"
#include "pzfmatrix.h"

#ifdef _AUTODIFF
#include "fadType.h"
#endif

/**
 * @ingroup material
 * @brief The TPZSwelling class implements a numerical model of swelling material coupling
 * flow through porous media with ionic transport
 */
class TPZSwelling : public TPZMaterial {
	
	/**
	 * computation mode :
	 * 0->residual wrt to time n
	 * 1->residual and tangent wrt to time n+1
	 */
	int fComputationMode;
	/**
	 * hydraulic permeability [mm^4 / (Ns)]
	 */
	TPZFMatrix fKperm;
	/**
	 * Compression modulus [N/mm^2]
	 */
	REAL fLambda;
	/**
	 * Shear modulus [N/mm^2]
	 */
	REAL fShear;
	/**
	 * Biot coupling coeficient (no dimension)
	 */
	REAL fAlfa;
	/**
	 * Storage modulus [N/mm^2]
	 */
	REAL fM;
	/**
	 * Osmotic coeficient (no dimension)
	 */
	REAL fGamma;
	/**
	 * Diffusion coeficient for cations [mm^2/s]
	 */
	REAL fDPlus;
	/**
	 * Diffusion coeficient for anions [mm^2/s]
	 */
	REAL fDMinus;
	/**
	 * Hindrance factor (no dimension)
	 */
	REAL frHinder;
	/**
	 * Initial deformation (assuming everything occurs isotropically, a constant is suficient (no dimension)
	 */
	REAL fInitDeform;
	/**
	 * Fixed charge density [mmoleq/mm^3]
	 */
	REAL fCfc;
	/**
	 * Initial fluid volume fraction (do dimension)
	 */
	REAL fNf0;
	/**
	 * Initial cation volume fraction (no dimension)
	 */
	REAL fNPlus0;
	/**
	 * Initial anion volume fraction (no dimension)
	 */
	REAL fNMinus0;
	
	/**
	 * Timestepping parameter theta (no dimension)
	 */
	REAL fTheta;
	/**
	 * Timestep [s]
	 */
	REAL fDelt;
	/**
	 * External concentration (used as reference value for pressure) [mmol/mm^3]
	 */
	static REAL gExtConc;
	/**
	 * Faraday constant [C/mmol]
	 */
	static REAL gFaraday;
	/**
	 * Molar volume cation [mm^3/mmol]
	 */
	static REAL gVPlus;
	/**
	 * Molar volume anions [mm^3/mmol]
	 */
	static REAL gVMinus;
	/**
	 * gas constant [Nmm/(mmol K)]
	 */
	static REAL gRGas;
	/**
	 * Absolute temperature [K]
	 */
	static REAL gTemp;
	/**
	 * Reference chemical potentials (order f,plus,minus) [mV]
	 */
	static REAL gMuRef[3];
	
	public :
	
	
	/**
	 * Constructor of the class, where the user needs to specify the most important parameters
	 * @param matindex index of material 
	 * @param lambda Compression modulus
	 * @param shear Shear modulus
	 * @param alfa Biot coupling coeficient
	 * @param M Storage modulus
	 * @param Gamma Osmotic coeficient
	 * @param Kperm hydraulic permeability (isotropic)
	 * @param DPlus Diffusion coeficient for cations
	 * @param DMinus Diffusion coeficient for anions
	 * @param rHinder Hindrance factor
	 * @param Cfc Fixed charge density
	 * @param Nf0 Initial fluid volume fraction
	 * @param NPlus0 Initial cation volume fraction
	 * @param NMinus0 Initial anion volume fraction
	 */
	TPZSwelling(int matindex, REAL lambda, REAL shear, REAL alfa, REAL M, REAL Gamma, REAL Kperm, REAL DPlus, REAL DMinus,
				REAL rHinder, REAL Cfc, REAL Nf0, REAL NPlus0, REAL NMinus0);
	
	virtual ~TPZSwelling();
	
	/**
	 * Dimension of the problem
	 */
	int Dimension() { return 3;}
	
	/**
	 * Number of state variables, in this case
	 * 3 displacements, 1 pressure, 3 eletrochemical potencials, 1 eletrical potencial
	 */
	int NStateVariables() { return 8;}
	
	virtual void Print(std::ostream & out);
	
	virtual std::string Name() { return "TPZSwelling"; }
	
	void SetComputationMode(int mode) {
		switch(mode) {
			case 0:
				fComputationMode = mode;
				break;
			case 1:
				fComputationMode = mode;
				break;
			default:
				std::cout << "ComputationMode illegal mode = " << mode << std::endl;
		}
	}
	
	/**
	 * This method computes the contribution to the residual vector and tangent matrix the traditional way
	 * THIS METHOD IS NOT IMPLEMENTED!!!
	 */
	virtual void Contribute(TPZMaterialData &data,
							REAL weight,
							TPZFMatrix &ek,
							TPZFMatrix &ef){
		std::cout << "TPZSwelling::Contribute not implemented\n";
	}
	virtual void Contribute(TPZMaterialData &data,
							REAL weight,
							TPZFMatrix &ef){
		std::cout << "TPZSwelling::Contribute not implemented\n";
	}
	
	virtual void ContributeBC(TPZMaterialData &data,
							  REAL weight,
							  TPZFMatrix &ek,
							  TPZFMatrix &ef,
							  TPZBndCond &bc);
	virtual void ContributeBC(TPZMaterialData &data,
							  REAL weight,
							  TPZFMatrix &ef,
							  TPZBndCond &bc)
	{
		TPZMaterial::ContributeBC(data,weight,ef,bc);
	}
	
	
#ifdef _AUTODIFF
	
	/**Compute contribution to the energy at an integration point*/
	virtual void ContributeElastEnergy(
									   TPZVec<FADFADREAL> &dsol,
									   FADFADREAL &U,
									   REAL weight);
	
	/**
	 * Computes the residual vector at an integration point and its tangent matrix by automatic differentiation
	 */
	virtual void ContributeResidual(TPZVec<REAL> & x,
									TPZVec<FADREAL> & sol,
									TPZVec<FADREAL> &dsol,
									TPZFMatrix &phi,
									TPZFMatrix &dphi,
									TPZVec<FADREAL> &RES,
									REAL weight);
	
	/**
	 * Computes the residual vector at an integration point and its tangent matrix by automatic differentiation
	 */
	virtual void ContributePrevResidual(TPZVec<REAL> & x,
										TPZVec<FADREAL> & sol,
										TPZVec<FADREAL> &dsol,
										TPZFMatrix &phi,
										TPZFMatrix &dphi,
										TPZVec<FADREAL> &RES,
										REAL weight);
	
	
	/**
	 * Compute contribution of BC to the Energy
	 */
	virtual void ContributeBCEnergy(TPZVec<REAL> & x,
									TPZVec<FADFADREAL> & sol, FADFADREAL &U,
									REAL weight, TPZBndCond &bc) {
		std::cout << "TPZSwelling::ContributeBCEnergy is not implemented\n";
	}
	
#endif
	
private:
	
	void ExactSolution(TPZVec<REAL> &mu, REAL ksi, REAL pres, TPZVec<REAL> &N);
	
	/**
	 * Computes the value of the N coeficients in function of ksi and mus, iterative method, inverting the Hessian of W
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeN(TPZVec<REAL> &N, TPZVec<REAL> &mu, REAL ksi);
	
	/**
	 * Computes the aproximate values of the pressure, ksi and N based on mu and J by direct inversion of
	 * the formulas.
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeInitialGuess(TPZVec<REAL> &mu, REAL J, REAL &pres, REAL &ksi, TPZVec<REAL> &N);
	
#ifdef _AUTODIFF
	/**
	 * Computes the mixing energy W and its first and second derivatives
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeW(FADFADREAL &W, TPZVec<REAL> &N);
	
#endif
	
	/**
	 * Computes the value of the N coeficients in function of ksi and mus, iterative method, inverting the Hessian of W
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeN(TPZVec<REAL> &mu, REAL ksi, REAL pressure, TPZVec<REAL> &N);
	
#ifdef _AUTODIFF
	/**
	 * Computes N and its partial derivatives by directly inverting the analytic expressions
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeN(TPZVec<FADREAL> &sol, TPZVec<FADREAL> &N);
	
	/**
	 * Computes N and its partial derivatives by directly inverting the analytic expressions
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void ComputeN(TPZVec<FADREAL> &sol, TPZVec<REAL> &N);
	
#endif
	/**
	 * Computes the residual and tangent vector of the system of equations which determines N
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void NResidual(TPZVec<REAL> &mu, REAL ksi, REAL pressure, TPZVec<REAL> &N, TPZFMatrix &res, TPZFMatrix &tangent);
	
#ifdef _AUTODIFF
	/**
	 * This method computes the numerical approximation of N by the Newton method and its derivatives
	 * with respect to the dependent variables
	 * This method has been superseded by the direct computation ExactSolution
	 */
	void NResidual(TPZVec<FADREAL> &sol, TPZVec<FADREAL> &N);
	
	/**
	 * This method performs a simple LU decomposition and inversion
	 * As the arguments are of type FADREAL, the solution of the system already carries the
	 * partial derivatives of the original system and right hand side
	 */
	static void Solve(TPZVec<TPZVec<FADREAL> > &tangent, TPZVec<FADREAL> &res);
public:
	
	static int main();
	
#endif
	/**
	 * Methods needed to perform convergence checks
	 */
	/**
	 * Number of cases which are considered for convergence checks
	 */
	int NumCases();
	/**
	 * Loads the state within the current object, to be used when computing the tangent matrix
	 */
	void LoadState(TPZFMatrix &state);
	/**
	 * Computes the tangent matrix for a given loadcase
	 */
	void ComputeTangent(TPZFMatrix &tangent,TPZVec<REAL> &coefs, int cases);
	/**
	 * Computes the residual for the given state variable
	 */
	void Residual(TPZFMatrix &res, int cases);
	/**
	 * Variables which holds the state variables used in the check convergence procedure
	 */
	static TPZFMatrix gState;
	/**
	 * Variables which holds the state variables used in the check convergence procedure
	 */
	
	static TPZFMatrix gphi,gdphi;
	
	
public:
	
	/**
	 * return the integer index corresponding to a post processing variable name
	 */
	virtual int VariableIndex(const std::string &name);
	/**
	 * return the number of solution variables associated with a variable index
	 * (e.g. 1 for scalar, 3 for vectorial, 9 for tensorial)
	 */
	virtual int NSolutionVariables(int var);
	
protected:
	/**
	 * computes a post-processed solution variable corresponding to the variable index
	 */
	virtual void Solution(TPZVec<REAL> &Sol,TPZFMatrix &DSol,TPZFMatrix &axes,int var,TPZVec<REAL> &Solout);
public:
	virtual void Solution(TPZMaterialData &data,int var,TPZVec<REAL> &Solout)
	{
		Solution(data.sol,data.dsol,data.axes,var,Solout);
	}
	
	
	
};

#endif
