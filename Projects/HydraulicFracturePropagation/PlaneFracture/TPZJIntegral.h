//
//  TPZJIntegral.h
//  PZ
//
//  Created by Labmec on 25/04/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef PZ_TPZJIntegral_h
#define PZ_TPZJIntegral_h

#include <iostream>
#include "TPZGeoElement.h"
#include "pzcmesh.h"
#include "pzelasmat.h"
#include "pzelast3d.h"
#include "pzcompel.h"
#include "pzinterpolationspace.h"
#include "TPZPlaneFracture.h"


const REAL Pi = 3.1415926535897932384626433832795;


class LinearPath
{
public:
    
    LinearPath();//It is not to be used
    LinearPath(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure, int meshDim);
    ~LinearPath();
    
    void X(REAL t, TPZVec<REAL> & xt);
    //void dXdt(REAL t, TPZVec<REAL> & dxdt);
    void normalVec(REAL t, TPZVec<REAL> & n);
    
    REAL DETdxdt();
    
    TPZVec<REAL> operator()(REAL t)
    {
        TPZVec<REAL> Vval = Func(t);
        return Vval;
    }
    
    TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
    
    REAL Origin(int c);
    
protected:
    
    /** Origin of coupled arc */
    TPZVec<REAL> fOrigin;
    
    /** This direction defines the coupled arc plane.
     *  (this direction is orthogonal to coupled arc plane and defines
     *   the right hand convention for the coupled arc direction)
     */
    TPZVec<REAL> fNormalDirection;
    
    /** Radius of coupled arc */
    REAL fradius;
    
    /** Determinant of dXdt(3x1) */
    REAL fDETdxdt;
    
    /** CMesh that constains data */
    TPZAutoPointer<TPZCompMesh> fcmesh;
    
    /** For 2D problems (plane strain or plane stress), fMeshDim=2 */
    /** For 3D problems, fMeshDim=3 */
    int fMeshDim;
    
    /** pressure applied inside fracture */
    REAL fcrackPressure;
    
    /** map that holds t and respective elId and qsi for ComputeXInverse optimization */
    std::map< REAL , std::pair< int , TPZVec<REAL> > > f_t_elIdqsi;
};


class ArcPath
{
public:
    
    ArcPath();//It is not to be used
    ArcPath(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, int meshDim);
    ArcPath(ArcPath * cp);
    ~ArcPath();
    
    void X(REAL t, TPZVec<REAL> & xt);
    //void dXdt(REAL t, TPZVec<REAL> & dxdt);
    void normalVec(REAL t, TPZVec<REAL> & n);
    
    REAL DETdxdt();
    
    TPZVec<REAL> operator()(REAL t)
    {
        TPZVec<REAL> Vval = Func(t);
        return Vval;
    }
    
    TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
    
    void SetRadius(REAL radius);
    REAL Radius();
    
protected:
    
    /** Origin of arc */
    TPZVec<REAL> fOrigin;
    
    /** This direction defines the arc plane.
     *  (this direction is orthogonal to arc plane and defines
     *   the right hand convention for the arc direction)
     */
    TPZVec<REAL> fNormalDirection;
    
    /** Radius of arc */
    REAL fradius;
    
    /** Determinant of dXdt(3x1) */
    REAL fDETdxdt;
    
    /** CMesh that constains data */
    TPZAutoPointer<TPZCompMesh> fcmesh;
    
    /** For 2D problems (plane strain or plane stress), fMeshDim=2 */
    /** For 3D problems, fMeshDim=3 */
    int fMeshDim;
    
    /** map that holds t and respective elId and qsi for ComputeXInverse optimization */
    std::map< REAL , std::pair< int , TPZVec<REAL> > > f_t_elIdqsi;
};



class AreaPath
{
public:
    
    AreaPath();//It is not to be used
    AreaPath(LinearPath * givenLinearPath, ArcPath * givenArcPath);
    ~AreaPath();
    
    virtual TPZVec<REAL> Integrate(int linNDiv);
    
protected:
    
    struct ArcPath2 : public ArcPath
    {
        public:
        ArcPath2();
        ArcPath2(ArcPath * cp);
        ~ArcPath2();
        
        virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
        
        TPZVec<REAL> ComputeFiniteDifference(REAL t, TPZVec<REAL> xt, REAL halfDelta = 0.1);
        
        TPZVec<REAL> FunctionAux(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & direction);
    };
    
    LinearPath * fLinearPath;
    ArcPath2 * fArcPath;
};



/**
 *  ITS ALWAYS GOOD TO REMEMBER:
 *          THE CLASS PATH CONSIDERS THAT THE NORMAL DIRECTION IS IN X,Z PLANE (JUST LIKE FRACTURE PLANE) AND
 *          THE ORIENTATION OF ARC and LINEAR stretch is:
 *              ARC : RIGHT HAND DIRECTION WITH RESPECT TO GIVEN NORMAL AXE (axe that defines the (orthogonal) arc plane).
 *              LINEAR: FROM THE END OF ARC (supposed to be inside crack plane) TO ORIGIN.
 * SO, ITS ALWAYS GOOD DEFINE NORMAL AXE TANGENT TO THE CRACK BOUNDARY, FOLLOWING RIGHT HAND FROM OUTSIDE CRACK TO INSIDE CRACK
 */
class Path
{
public:

    Path();

    /**
     * Given unidimensional element reffers to the cracktip element that will be used
     * to compute J-integral around it.
     * Obs.: normal direction must be in xz plane and the arcs (internal and external) will be in (y>0).
     */
    Path(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure, int meshDim);
    ~Path();
    
    LinearPath * GetLinearPath()
    {
        return fLinearPath;
    }
    
    ArcPath * GetArcPath()
    {
        return fArcPath;
    }
    
    AreaPath * GetAreaPath()
    {
        return fAreaPath;
    }
    
    int MeshDim()
    {
        return fMeshDim;
    }
    
    
protected:
    
    LinearPath * fLinearPath;
    ArcPath * fArcPath;
    AreaPath * fAreaPath;
    
    int fMeshDim;
};



class JIntegral
{
public:
    
    JIntegral();
    ~JIntegral();
    
    void PushBackPath(Path *pathElem);
    
    TPZVec<REAL> IntegratePath(int p);
    
private:
    
    TPZVec<Path*> fPathVec;
};



#endif
