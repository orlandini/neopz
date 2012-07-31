//
//  TPZJIntegral.cpp
//  PZ
//
//  Created by Labmec on 25/04/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>

#include "TPZJIntegral.h"

#include "adapt.h"
#include "TPZPlaneFracture.h"
#include "pzelast3d.h"
#include "pzcompel.h"
#include "pzinterpolationspace.h"

const double Pi = 3.1415926535897932384626433832795;

const int dim3D = 3;


//--------------------------------------------------------class JPath


Path::Path()
{
    fOrigin.Resize(0);
    fNormalDirection.Resize(0);
    
    fr_int = 0.;
    fr_ext = 0.;
    
    fInitial2DElementId = 0;
    
    fcmesh = NULL;
}


Path::Path(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, double r_int, double r_ext)
{
    fOrigin = Origin;
    fNormalDirection = normalDirection;
    
    #ifdef DEBUG
    if(fabs(normalDirection[1]) > 1.E-8)
    {
        std::cout << "\nThe normal direction of J integral arc must be in XZ plane!!!\n";
        DebugStop();
    }
    #endif
    
    fr_int = r_int;
    fr_ext = r_ext;
    
    fInitial2DElementId = 0;
    
    fcmesh = cmesh;
}


Path::~Path()
{
    fOrigin.Resize(0);
    fNormalDirection.Resize(0);
    
    fr_int = 0.;
    fr_ext = 0.;
}

TPZVec<REAL> Path::Func(double t)
{
    TPZVec<REAL> xt(dim3D), dxdt(dim3D), nt(dim3D);
    REAL DETdxdt;
    this->X(t,xt);
    this->dXdt(t, dxdt, DETdxdt);
    this->normalVec(t, nt);

    TPZVec<REAL> qsi(dim3D,0.);
    TPZGeoEl * geoEl = TPZPlaneFracture::PointElementOnFullMesh(xt, qsi, fInitial2DElementId, this->fcmesh->Reference());
    
    #ifdef DEBUG
    bool isInsideDomain = geoEl->ComputeXInverse(xt, qsi);
    if(!isInsideDomain)
    {
        std::cout << "ComputeXInverse doesn't work on " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    #endif
    
    TPZCompEl * compEl = geoEl->Reference();
    
    #ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
    #endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);

    TPZElasticity3D * elast3D = dynamic_cast<TPZElasticity3D *>(compEl->Material());
    
    #ifdef DEBUG
    if(!elast3D)
    {
        std::cout << "This material might be TPZElastMat3D type!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
    #endif
    
    TPZFMatrix<REAL> Sigma, GradU = data.dsol[0];
    elast3D->ComputeStressTensor(Sigma, data);
    
    
    double lambda = elast3D->GetLambda();
    double mu = elast3D->GetMU();
    
    double W = mu*(Sigma(0,0)*Sigma(0,0) + Sigma(0,1)*Sigma(0,1) + Sigma(0,2)*Sigma(0,2) + Sigma(1,0)*Sigma(1,0) 
                   + Sigma(1,1)*Sigma(1,1) + Sigma(1,2)*Sigma(1,2) + Sigma(2,0)*Sigma(2,0) + Sigma(2,1)*Sigma(2,1) + Sigma(2,2)*Sigma(2,2)) + 
    (lambda*(Sigma(0,0)*Sigma(0,0) + Sigma(1,1)*Sigma(1,1) + 4.*Sigma(1,1)*Sigma(2,2) + Sigma(2,2)*Sigma(2,2) + 4.*Sigma(0,0)*(Sigma(1,1) + Sigma(2,2))))/2.;
    
    TPZFMatrix<REAL> W_I(dim3D,dim3D,0.);
    W_I.PutVal(0, 0, W);
    W_I.PutVal(1, 1, W);
    W_I.PutVal(2, 2, W);
    GradU.Transpose();
    TPZFMatrix<REAL> GradUtranspose_Sigma(dim3D,dim3D,0.);
    GradU.Multiply(Sigma, GradUtranspose_Sigma);
    
    TPZFMatrix<REAL> W_I_minus_GradUtranspose_Sigma(dim3D,dim3D,0.);
    W_I_minus_GradUtranspose_Sigma = W_I - GradUtranspose_Sigma;
    
    TPZVec<REAL> W_I_minus_GradUtranspose_Sigma__n(dim3D,0.);
    for(int r = 0; r < dim3D; r++)
    {
        for(int c = 0; c < dim3D; c++)
        {
            W_I_minus_GradUtranspose_Sigma__n[r] += (W_I_minus_GradUtranspose_Sigma(r,c)*nt[c]) * DETdxdt;
        }
    }   
    
    return W_I_minus_GradUtranspose_Sigma__n;
}


void Path::X(double t, TPZVec<REAL> & xt)
{
    std::cout << "The code should not enter here, but in derivated class!\n";
    DebugStop();
}


void Path::dXdt(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt)
{
    std::cout << "The code should not enter here, but in derivated class!\n";
    DebugStop();
}


void Path::normalVec(double t, TPZVec<REAL> & n)
{
    std::cout << "The code should not enter here, but in derivated class!\n";
    DebugStop();
}

//------------

void Path::X_line(double t, TPZVec<REAL> & xt)
{
    xt.Resize(dim3D,0.);
    
    TPZVec<REAL> initialPoint(dim3D);
    TPZVec<REAL> finalPoint(dim3D);
    
    if(__defaultDirection == EClockwise)
    {
        this->X_arc(+1., initialPoint, EInternalArcPath);
        this->X_arc(-1., finalPoint, EExternalArcPath);
    }
    else if(__defaultDirection == ECounterclockwise)
    {
        this->X_arc(+1., initialPoint, EExternalArcPath);
        this->X_arc(-1., finalPoint, EInternalArcPath);
    }
    
    for(int c = 0; c < dim3D; c++)
    {
        xt[c] = (1.-t)/2.*initialPoint[c] + (1.+t)/2.*finalPoint[c];
    }
}


void Path::dXdt_line(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt)
{
    dxdt.Resize(dim3D,0.);
    
    TPZVec<REAL> x0(dim3D);
    TPZVec<REAL> x1(dim3D);
    
    
    if(__defaultDirection == EClockwise)
    {
        this->X_arc(+1., x0, EInternalArcPath);
        this->X_arc(-1., x1, EExternalArcPath);
    }
    else if(__defaultDirection == ECounterclockwise)
    {
        this->X_arc(+1., x0, EExternalArcPath);
        this->X_arc(-1., x1, EInternalArcPath);
    }
    
    DETdxdt = 0.;
    for(int c = 0; c < dim3D; c++)
    {
        dxdt[c] = (x1[c]-x0[c])/2.;
        DETdxdt += dxdt[c]*dxdt[c];
    }
    DETdxdt = sqrt(DETdxdt);
}


void Path::X_arc(double t, TPZVec<REAL> & xt, pathType pathT)
{
    xt.Resize(dim3D,0.);
    
    if(__defaultDirection == EClockwise)
    {
        if(pathT == EInternalArcPath)
        {
            xt[0] = (fOrigin[0] - fr_int*sin((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
            xt[1] = fr_int*cos((Pi*t)/2.);
            xt[2] = (fOrigin[2] + fr_int*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((Pi*t)/2.));
        }
        else if(pathT == EExternalArcPath)
        {                   
            xt[0] = (fOrigin[0] + fr_ext*sin((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
            xt[1] = fr_ext*cos((Pi*t)/2.);
            xt[2] = (fOrigin[2] - fr_ext*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((Pi*t)/2.));
        }
    }
    else if(__defaultDirection == ECounterclockwise)
    {
        if(pathT == EExternalArcPath)
        {
            xt[0] = (fOrigin[0] - fr_ext*sin((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
            xt[1] = fr_ext*cos((Pi*t)/2.);
            xt[2] = (fOrigin[2] + fr_ext*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((Pi*t)/2.));
        }
        else if(pathT == EInternalArcPath)
        {                   
            xt[0] = (fOrigin[0] + fr_int*sin((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
            xt[1] = fr_int*cos((Pi*t)/2.);
            xt[2] = (fOrigin[2] - fr_int*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((Pi*t)/2.));
        }
    }
}


void Path::dXdt_arc(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt, pathType pathT)
{
    dxdt.Resize(dim3D,0.);

    if(__defaultDirection == EClockwise)
    {
        if(pathT == EInternalArcPath)
        {
            dxdt[0] = -(Pi*fr_int*cos((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            dxdt[1] = -(Pi*fr_int*sin((Pi*t)/2.))/2.;
            dxdt[2] = +(Pi*fr_int*cos((Pi*t)/2.)*cos(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            DETdxdt = Pi*fr_int/2.;
        }
        else if(pathT == EExternalArcPath)
        {                   
            dxdt[0] = +(Pi*fr_ext*cos((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            dxdt[1] = -(Pi*fr_ext*sin((Pi*t)/2.))/2.;
            dxdt[2] = -(Pi*fr_ext*cos((Pi*t)/2.)*cos(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            DETdxdt = Pi*fr_ext/2.;
        }
    }
    if(__defaultDirection == ECounterclockwise)
    {
        if(pathT == EExternalArcPath)
        {
            dxdt[0] = -(Pi*fr_ext*cos((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            dxdt[1] = -(Pi*fr_ext*sin((Pi*t)/2.))/2.;
            dxdt[2] = +(Pi*fr_ext*cos((Pi*t)/2.)*cos(atan2(fNormalDirection[0],fNormalDirection[2])))/2.;
            DETdxdt = Pi*fr_ext/2.;
        }
        else if(pathT == EInternalArcPath)
        {                   
            dxdt[0] = +(Pi*fr_int*cos((Pi*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            dxdt[1] = -(Pi*fr_int*sin((Pi*t)/2.))/2.;
            dxdt[2] = -(Pi*fr_int*cos((Pi*t)/2.)*cos(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
            DETdxdt = Pi*fr_int/2.;
        }
    }
}


//--------------------------------------------------------class linearPath


void linearPath::X(double t, TPZVec<REAL> & xt)
{
    X_line(t, xt);
}


void linearPath::dXdt(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt)
{
    dXdt_line(t, dxdt, DETdxdt);
}


void linearPath::normalVec(double t, TPZVec<REAL> & n)
{
    n[0] = 0.;
    n[1] = -1.;
    n[2] = 0.;
}


//--------------------------------------------------------class externalArcPath


void externalArcPath::X(double t, TPZVec<REAL> & xt)
{
    pathType pathT = EExternalArcPath;
    X_arc(t, xt, pathT);
}


void externalArcPath::dXdt(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt)
{
    pathType pathT = EExternalArcPath;
    dXdt_arc(t,dxdt, DETdxdt, pathT);
}


void externalArcPath::normalVec(double t, TPZVec<REAL> & n)
{
    TPZVec<REAL> x(dim3D);
    X(t, x);
    
    double normaN = 0.;
    for(int i = 0; i < dim3D; i++)
    {
        normaN += (x[i] - fOrigin[i]) * (x[i] - fOrigin[i]);
    }
    normaN = sqrt(normaN);
    
    for(int i = 0; i < dim3D; i++)
    {
        n[i] = +1. * fabs(x[i] - fOrigin[i])/normaN;
    }
}


//--------------------------------------------------------class internalArcPath


void internalArcPath::X(double t, TPZVec<REAL> & xt)
{
    pathType pathT = EInternalArcPath;
    X_arc(t, xt, pathT);
}


void internalArcPath::dXdt(double t, TPZVec<REAL> & dxdt, REAL & DETdxdt)
{
    pathType pathT = EInternalArcPath;
    dXdt_arc(t,dxdt, DETdxdt, pathT);
}


void internalArcPath::normalVec(double t, TPZVec<REAL> & n)
{
    TPZVec<REAL> x(dim3D);
    X(t, x);
    
    double normaN = 0.;
    for(int i = 0; i < dim3D; i++)
    {
        normaN += (x[i] - fOrigin[i]) * (x[i] - fOrigin[i]);
    }
    normaN = sqrt(normaN);
    
    for(int i = 0; i < dim3D; i++)
    {
        n[i] = -1. * fabs(x[i] - fOrigin[i])/normaN;
    }
}


//--------------------------------------------------------class JIntegral


JIntegral::JIntegral()
{
    fPathVec.Resize(0);
}

JIntegral::~JIntegral()
{
    fPathVec.Resize(0);
}

void JIntegral::PushBackPath(Path * pathElem)
{
    int oldSize = fPathVec.NElements();
    fPathVec.Resize(oldSize+1);
    fPathVec[oldSize] = pathElem;
}

TPZVec<REAL> JIntegral::IntegratePath(int p)
{
    Path * jpathElem = fPathVec[p];

    double precisionIntegralRule = 1.E-30;
    Adapt intRule(precisionIntegralRule);
    
    linearPath * _LinearPath = new linearPath;
    _LinearPath->fOrigin = jpathElem->fOrigin;
    _LinearPath->fNormalDirection = jpathElem->fNormalDirection;
    _LinearPath->fr_int = jpathElem->fr_int;
    _LinearPath->fr_ext = jpathElem->fr_ext;
    _LinearPath->fInitial2DElementId = jpathElem->fInitial2DElementId;
    _LinearPath->fcmesh = jpathElem->fcmesh;
    
    externalArcPath * _ExtArcPath = new externalArcPath;
    _ExtArcPath->fOrigin = jpathElem->fOrigin;
    _ExtArcPath->fNormalDirection = jpathElem->fNormalDirection;
    _ExtArcPath->fr_int = jpathElem->fr_int;
    _ExtArcPath->fr_ext = jpathElem->fr_ext;
    _ExtArcPath->fInitial2DElementId = jpathElem->fInitial2DElementId;
    _ExtArcPath->fcmesh = jpathElem->fcmesh;
    
    internalArcPath * _IntArcPath = new internalArcPath;
    _IntArcPath->fOrigin = jpathElem->fOrigin;
    _IntArcPath->fNormalDirection = jpathElem->fNormalDirection;
    _IntArcPath->fr_int = jpathElem->fr_int;
    _IntArcPath->fr_ext = jpathElem->fr_ext;
    _IntArcPath->fInitial2DElementId = jpathElem->fInitial2DElementId;
    _IntArcPath->fcmesh = jpathElem->fcmesh;
    
    
    TPZVec<REAL> integrLinPath = intRule.Vintegrate(*_LinearPath,dim3D,Path::leftLimit(),Path::rightLimit());
    TPZVec<REAL> integrExtArc  = intRule.Vintegrate(*_ExtArcPath,dim3D,Path::leftLimit(),Path::rightLimit());
    TPZVec<REAL> integrIntArc  = intRule.Vintegrate(*_IntArcPath,dim3D,Path::leftLimit(),Path::rightLimit());

    //Pela simetria do problema em relacao ao plano xz, deve-se somar a este vetor seu espelho em relacao ao plano xz.
    TPZVec<REAL> answ(dim3D);
    answ[0] = 2.*(integrLinPath[0] + integrExtArc[0] + integrIntArc[0]);
    answ[1] = 0.;
    answ[2] = 2.*(integrLinPath[2] + integrExtArc[2] + integrIntArc[2]);
    
    return answ;
}




