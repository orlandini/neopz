#include "tpzquadratictrig.h"

#include "pzfmatrix.h"
#include "pzvec.h"
#include "pzgeotriangle.h"
#include "pzgmesh.h"
#include "pzgeoel.h"
#include "tpztriangle.h"
#include "pznoderep.h"
#include "pzshapetriang.h"

#include <iostream>
#include <iostream>
#include <cmath>

using namespace std;
using namespace pzshape;

  /**
  / Class made by Paulo Cesar de Alvarenga Lucci (Caju)
  / LabMeC - FEC - UNICAMP
  / 2007
 */

void TPZQuadraticTrig::Shape(TPZVec<REAL> &param,TPZFMatrix &phi,TPZFMatrix &dphi)
{
     REAL qsi = param[0], eta = param[1];
     phi(0,0) = (qsi+eta-1.)*(2.*qsi+2.*eta-1.);
     phi(1,0) = qsi*(2.*qsi-1.);
     phi(2,0) = eta*(2.*eta-1.);
     phi(3,0) = -4.*qsi*(qsi+eta-1.);
     phi(4,0) =  4.*qsi*eta;
     phi(5,0) = -4.*eta*(qsi+eta-1.);
     dphi(0,0) = 1.-4.*(1.-qsi-eta); dphi(0,1) = -1.+4.*qsi; dphi(0,2) = 0.; dphi(0,3) = 4.*(1.-qsi-eta)-4.*qsi; dphi(0,4) = 4.*eta; dphi(0,5) = -4.*eta;
     dphi(1,0) = 1.-4.*(1.-qsi-eta); dphi(1,1) = 0.; dphi(1,2) = -1.+4.*eta; dphi(1,3) = -4.*qsi; dphi(1,4) = 4.*qsi; dphi(1,5) = 4.*(1.-qsi-eta)-4.*eta;
}

void TPZQuadraticTrig::X(TPZFMatrix &coord, TPZVec<REAL>& par, TPZVec< REAL >& result)
{
     TPZVec<REAL> parMap(2);
     REAL spacephi[6],spacedphi[12];
     TPZFMatrix phi(6,1,spacephi,6);
     TPZFMatrix dphi(2,6,spacedphi,12);
     Shape(par,phi,dphi);
     for(int i = 0; i < 3; i++)
     {
          result[i] = 0.0;
          for(int j = 0; j < 6; j++) result[i] += phi(j,0)*coord(i,j);
     }
}

void TPZQuadraticTrig::Jacobian(TPZFMatrix & coord, TPZVec<REAL>& par, TPZFMatrix &jacobian, TPZFMatrix &axes, REAL &detjac, TPZFMatrix &jacinv)
{
     jacobian.Resize(2,2); axes.Resize(2,3); jacinv.Resize(2,2);
     REAL spacephi[6],spacedphi[12];
     TPZFMatrix phi(6,1,spacephi,6);
     TPZFMatrix dphi(2,6,spacedphi,12);
     Shape(par,phi,dphi);
     jacobian.Zero();

     TPZFMatrix VecMatrix(3,2,0.);
     for(int i = 0; i < 6; i++)
     {
          for(int j = 0; j < 3; j++)
          {
               VecMatrix(j,0) += coord(j,i)*dphi(0,i);
               VecMatrix(j,1) += coord(j,i)*dphi(1,i);
          }
     }
     TPZFMatrix axest;
     VecMatrix.GramSchmidt(axest,jacobian);
     axest.Transpose(&axes);

     detjac = jacobian(0,0)*jacobian(1,1)-jacobian(1,0)*jacobian(0,1);
     jacinv(0,0) =  jacobian(1,1)/detjac;
     jacinv(1,1) =  jacobian(0,0)/detjac;
     jacinv(0,1) = -jacobian(0,1)/detjac;
     jacinv(1,0) = -jacobian(1,0)/detjac;
}

TPZGeoEl *TPZQuadraticTrig::CreateBCGeoEl(TPZGeoEl *orig,int side,int bc)
{
     if(side==6)
     {
          TPZManVector<int> nodes(3); int i;
          for (i=0;i<3;i++) nodes[i] = orig->SideNodeIndex(side,i);
          int index;
          TPZGeoEl *gel = orig->Mesh()->CreateGeoElement(ETriangle,nodes,bc,index);
          int iside;
          for (iside = 0; iside <6; iside++)
          {
               TPZGeoElSide(gel,iside).SetConnectivity(TPZGeoElSide(orig,TPZShapeTriang::SideConnectLocId(side,iside)));
          }
          TPZGeoElSide(gel,6).SetConnectivity(TPZGeoElSide(orig,side));
          return gel;
     }

     else if(side>-1 && side<3)
     {
          TPZManVector<int> nodeindexes(1);
          nodeindexes[0] = orig->SideNodeIndex(side,0);
          int index;
          TPZGeoEl *gel = orig->Mesh()->CreateGeoElement(EPoint,nodeindexes,bc,index);
          TPZGeoElSide(gel,0).SetConnectivity(TPZGeoElSide(orig,side));
          return gel;
     }

     else if(side > 2 && side < 6)
     {
          TPZManVector<int> nodes(2);
          nodes[0] = orig->SideNodeIndex(side,0);
          nodes[1] = orig->SideNodeIndex(side,1);
          int index;
          TPZGeoEl *gel = orig->Mesh()->CreateGeoElement(EOned,nodes,bc,index);
          TPZGeoElSide(gel,0).SetConnectivity(TPZGeoElSide(orig,TPZShapeTriang::SideConnectLocId(side,0)));
          TPZGeoElSide(gel,1).SetConnectivity(TPZGeoElSide(orig,TPZShapeTriang::SideConnectLocId(side,1)));
          TPZGeoElSide(gel,2).SetConnectivity(TPZGeoElSide(orig,side));
          return gel;
     }

     else PZError << "TPZGeoTriangle::CreateBCGeoEl has no bc.\n";
     return 0;
}

#include "pzgeoelrefless.h.h"

template class TPZGeoElRefLess<TPZQuadraticTrig>;
