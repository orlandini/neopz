/**
   DIFUS�O <-> DIFFUSION

do Lat. diffusione

s. f., acto ou efeito de difundir ou difundir-se;
    derramamento de fluido;
    dissemina��o;
    propaga��o;
    prolixidade;
    falta de concis�o;
Qu�m., mistura de gases de diferentes densidades.
*/

#ifndef DIFFUSIONFORCONSLAWHH
#define DIFFUSIONFORCONSLAWHH


#include "pzvec.h"
#include "pzfmatrix.h"

/**
 * This classroom adds to the term of diffusion to the variacional formularization
 * of the differential equation partial compressible of Euler (hyperbolic). This
 * term is introduced to stabilize the numerical method of approach.
 */
class TPZDiffusionConsLaw {


 public:
  /**
   * ratio between specific heat is constant and the specific heat the constant
   * volume of a polytropic gas 
   */
  static REAL fGamma;

  /**
   * coefficient of size ratio of the element in the diffusion term
   */
  static REAL fDelta;

  /**
   * par�metro that it limits the condition of stability of the numerical approach
   */
  static REAL fCFL;

  /**
   * Term that adds stability to the numerical method of approach
   * SUPG
   * LS
   * Bornhaus
   */
  static char *fArtificialDiffusion;

 private:
  /*
   * matrix computation derivatives of fluxes: dFx/dx, dFy/dy, dFz/dz
   */
  TPZFMatrix fA,fB,fC;

  /*
   * problem dimension
   */
  int fDimension;

 public:
  
  TPZDiffusionConsLaw();

  TPZDiffusionConsLaw(TPZVec<REAL> U,REAL gamma,int dim,char *diff);

  ~TPZDiffusionConsLaw();

  //static void SetDelta(REAL delta){TPZDiffusionConsLaw::fDelta = delta;}

  void GradientOfTheFlow(TPZFMatrix &DF1,TPZFMatrix &DF2,TPZFMatrix &DF3);

  REAL DeltaOtimo();

  REAL Delta();

  REAL CFL(int degree);

  //static void SetGamma(REAL gamma){TPZDiffusionConsLaw::fGamma = gamma;}

  //static void SetArtificialDiffusion(char *type){TPZDiffusionConsLaw::fArtificialDiffusion = type;}

  //void DiffusionTerm(TPZFMatrix &dphi,TPZFMatrix &diff_term);

  /**
   * Jacobiano of the tensor flux of Euler 
   */
  static void JacobFlux(TPZVec<REAL> U,TPZFMatrix &A,TPZFMatrix &B,TPZFMatrix &C);

  void Divergence(TPZVec<REAL> &dphi,TPZFMatrix &diverg);

  /**
   * operation product point in the diffusion term 
   */
  void PointOperator(TPZVec<REAL> &dphi,TPZFMatrix &diff_term);

  void Tau(TPZFMatrix &Tx,TPZFMatrix &Ty,TPZFMatrix &Tz);
  
  void SUPG(TPZFMatrix &Tx,TPZFMatrix &Ty,TPZFMatrix &Tz);
  
  void LS(TPZFMatrix &Tx,TPZFMatrix &Ty,TPZFMatrix &Tz);
  
  void Bornhaus(TPZFMatrix &Tx,TPZFMatrix &Ty,TPZFMatrix &Tz);
  
  /**
   * Flux of Roe (MOUSE program) 
   */
  static void Roe_Flux(REAL rho_f, REAL rhou_f, REAL rhov_f, REAL rhow_f, REAL rhoE_f, 
		       REAL rho_t, REAL rhou_t, REAL rhov_t,REAL rhow_t, REAL rhoE_t, 
		       REAL nx, REAL ny, REAL nz, REAL gam, REAL & flux_rho, REAL & flux_rhou, 
		       REAL & flux_rhov,REAL & flux_rhow, REAL & flux_rhoE);
  
  static void Roe_Flux(REAL rho_f, REAL rhou_f, REAL rhov_f, REAL rhoE_f,
				   REAL rho_t, REAL rhou_t, REAL rhov_t, REAL rhoE_t,
				   REAL nx, REAL ny, REAL gam,
				   REAL &flux_rho, REAL &flux_rhou,
				   REAL &flux_rhov, REAL &flux_rhoE);
};

#endif
