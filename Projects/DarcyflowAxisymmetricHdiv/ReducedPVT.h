//
//  ReducedPVT.h
//  PZ
//
//  Created by Omar on 4/24/15.
//
//

#ifndef __PZ__ReducedPVT__
#define __PZ__ReducedPVT__

#include "tpzautopointer.h"
#include "pzfmatrix.h"
#include <math.h>


class ReducedPVT

{
    
private:
    
    /** @brief Temperature @ reservoir conditions  - F */
    REAL fReservoirTemperature;
    
    /** @brief Characteristic Pressure - Pa */
    REAL fPRef;
    
    /** @brief Characteristic Density - kg/m3 */
    REAL fRhoRef;
    
    /** @brief Characteristic viscosity - Pa s */
    REAL fMuRef;
    
    /** @brief Density - kg/m3  $\rho_{g}$ */
    REAL fRho;
    
    /** @brief viscosity - Pa s  $\mu_{g}$ */
    REAL fMu;
    
    /** @brief Compressibility - 1/pa $c_{g}$ */
    REAL fc;
    
    
public:
    
    /** @brief Default constructor $ */
    ReducedPVT();
    
    /** @brief Default desconstructor $ */
    ~ReducedPVT();
    
    /** @brief Set the temperature @ reservoir conditions  - F */
    virtual void SetResT(REAL ResT);
    
    /** @brief Get the temperature @ reservoir conditions  - F */
    virtual REAL ResT();
    
    /** @brief Set the characteristic Pressure - Pa */
    virtual void SetPRef(REAL Pref);
    
    /** @brief Characteristic Pressure - Pa */
    virtual REAL PRef();
    
    /** @brief Set the characteristic Density - kg/m3 */
    virtual void RhoRef(REAL Rhoref);
    
    /** @brief Characteristic Density - kg/m3 */
    virtual REAL RhoRef();
    
    /** @brief Set the characteristic viscosity - Pa s */
    virtual void SetMuRef(REAL Muref);
    
    /** @brief Characteristic viscosity - Pa s */
    virtual REAL MuRef();
    
    /** @brief Density - kg/m3  $\rho$ */
    virtual void Density(TPZVec<REAL> &rho, TPZVec<REAL> state_vars);
    
    /** @brief viscosity - Pa s  $\mu$ */
    virtual void Viscosity(TPZVec<REAL> &mu, TPZVec<REAL> state_vars);
    
    /** @brief Compressibility - 1/pa $c$ */
    virtual void Compressibility(TPZVec<REAL> &c, TPZVec<REAL> state_vars);
    
    /** @brief Set Density - kg/m3  $\rho$ */
    virtual void SetRho(REAL Rho){this->fRho = Rho;}
    
    /** @brief Get Density - kg/m3  $\rho$ */
    virtual REAL GetRho(){return this->fRho ;}
    
    /** @brief Set viscosity - pa s  $\mu$ */
    virtual void SetMu(REAL Mu){this->fMu = Mu;}
    
    /** @brief Get viscosity - pa s  $\mu$ */
    virtual REAL GetMu(){return this->fMu ;}
    
    /** @brief Set Compressibility - 1/pa   $C$ */
    virtual void Setc(REAL c){this->fc = c;}
    
    /** @brief Get Compressibility - 1/pa   $C_$ */
    virtual REAL Getc(){return this->fc ;}
    
};


#endif /* defined(__PZ__ReducedPVT__) */