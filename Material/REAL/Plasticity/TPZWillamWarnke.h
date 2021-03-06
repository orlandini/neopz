/*
 *  TPZWillamWarnke.h
 *  ElastoPlasticModels
 *
 *  Created by Diogo Cecilio on 12/14/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


/* Generated by Together */// $Id: TPZDruckerPrager.h,v 1.24 2009-09-24 04:19:52 Diogo Exp $

#ifndef TZPWILLAMWARNKE_H
#define TZPWILLAMWARNKE_H

#include "pzlog.h"
#include "TPZPlasticStep.h"
#include "TPZYCWillamWarnke.h"
#include "TPZThermoForceA.h"
#include "TPZElasticResponse.h"
#include "pzvec_extras.h"
#include "pzsave.h"
#include "TPZPlasticStepID.h"


#define WILLAMWARNKEPARENT TPZPlasticStep<TPZYCWillamWarnke, TPZThermoForceA, TPZElasticResponse>


class TPZWillamWarnke : public WILLAMWARNKEPARENT  {
	
public:
	
	enum {NYield = TPZYCWillamWarnke::NYield};
	
public:
	
    TPZWillamWarnke():WILLAMWARNKEPARENT(), faPa(0.), fInitialEps()
    {
		fMaterialTensionSign  = 1; // internally in this material tension is negative
		fInterfaceTensionSign =  1; // by default
    }
	
    TPZWillamWarnke(const TPZWillamWarnke & source):WILLAMWARNKEPARENT(source)
    {
		faPa		= source.faPa;
		fInitialEps = source.fInitialEps;
    }
	
    TPZWillamWarnke & operator=(const TPZWillamWarnke & source)
    {
		WILLAMWARNKEPARENT::operator=(source);
		faPa		= source.faPa;
		fInitialEps = source.fInitialEps;
		
		return *this;
    }
	
	virtual const char * Name() const
	{
		return "TPZWillamWarnke";	
	}
	
    /**
	 SetUp feeds all the parameters necessary to the method, distributing its values
	 inside the aggregation hierarchy and computing the correct initial plasticity 
	 parameter to ensure the irreversibility effect of plastic deformations.
	 Elastic Mudulus:    poisson, M,    lambda
	 Failure Criterion:  a,       m,    neta1
	 Plastic Potential:  ksi2,    mu
	 Hardening Function: C,       p
	 Yield Function:     h,       alpha
	 Atmospheric pression pa - to dimensionalize/adim. the stresses
	 */
	//	void SetUp()
	//	{
	//		
	//		REAL pi = 3.141592653589793;
	//		DRUCKERPARENT::fYC.SetUp(/*phi=20*/ 20./180. * pi ,/*innerMCFit*/0);
	//		DRUCKERPARENT::fTFA.SetUp(/*yield- coesao inicial correspondeno a fck igual 32 Mpa */ 9.2376, /*k Modulo de hardening da coesao equivante 1 Mpa a cada 0.1% de deformacao */1000.);
	//		DRUCKERPARENT::fER.SetUp(/*young*/ 20000., /*poisson*/ 0.2);
	//		TPZTensor<REAL> nullSigma,epsA;
	//		fInitialEps = DRUCKERPARENT::GetState();
	//		
	//	}
	
	virtual void Print(std::ostream & out) const
	{
		out << "\n" << this->Name();
		out << "\n Base Class Data:\n";
		WILLAMWARNKEPARENT::Print(out);
		out << "\nTPZDruckerPrager internal members:";
		out << "\n a*Pa = " << faPa;
		out << "\n InitialEps = " << fInitialEps;
		
	}
	
	virtual int ClassId() const
	{
		return TPZWILLAMWARNKEPARENT_ID;	
	}
	
	virtual void Write(TPZStream &buf) const
	{
		
		buf. Write(&faPa, 1);
        fInitialEps.Write(buf);
//		buf. Write(&fInitialEps.fEpsT.fData[0], 6);
//		buf. Write(&fInitialEps.fEpsP.fData[0], 6);
//		buf. Write(&fInitialEps.fAlpha, 1);			
		
//		fPlasticMem.Resize(0);
	}
	
	virtual void Read(TPZStream &buf)
	{		
		buf. Read(&faPa, 1);
        fInitialEps.Read(buf);
//		buf. Read(&fInitialEps.fEpsT.fData[0], 6);
//		buf. Read(&fInitialEps.fEpsP.fData[0], 6);
//		buf. Read(&fInitialEps.fAlpha, 1);			
		
        //		fPlasticMem.Resize(0);
	}
	
	
private:
	
	

    REAL faPa;
	

    TPZPlasticState<STATE> fInitialEps;
	
	
};


#endif //TPZDruckerPrager_H