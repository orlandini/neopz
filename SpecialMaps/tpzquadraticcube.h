#ifndef TPZQUADRATICCUBE_H
#define TPZQUADRATICCUBE_H


#include "TPZGeoCube.h"
#include "pzgeoel.h"
#include "pznoderep.h"

#include <iostream>

/**
 / Class made by Paulo Cesar de Alvarenga Lucci (Caju)
 / LabMeC - FEC - UNICAMP
 / 2011
 */

class TPZQuadraticCube : public pzgeom::TPZNodeRep<20,pztopology::TPZCube> {
  
public:
  
  enum {NNodes = 20};
  
  bool IsLinearMapping() const {
    return false;
  }
  
  TPZQuadraticCube(TPZVec<int> &nodeindexes) : pzgeom::TPZNodeRep<NNodes,pztopology::TPZCube>(nodeindexes)
  {
  }
  
  TPZQuadraticCube() : pzgeom::TPZNodeRep<NNodes,pztopology::TPZCube>()
  {
  }
  
  TPZQuadraticCube(const TPZQuadraticCube &cp,std::map<int,int> & gl2lcNdMap) : pzgeom::TPZNodeRep<NNodes,pztopology::TPZCube>(cp,gl2lcNdMap)
  {
  }
  
  TPZQuadraticCube(const TPZQuadraticCube &cp) : pzgeom::TPZNodeRep<NNodes,pztopology::TPZCube>(cp)
  {
  }
  
  TPZQuadraticCube(const TPZQuadraticCube &cp, TPZGeoMesh &) : pzgeom::TPZNodeRep<NNodes,pztopology::TPZCube>(cp)
  {
  }
  
  /**
   * returns the type name of the element
   */
  static std::string TypeName() { return "Hexa";} 
  
  static void Shape(TPZVec<REAL> &x,TPZFMatrix &phi,TPZFMatrix &dphi);
  
  static void X(TPZFMatrix &coord, TPZVec<REAL> &par, TPZVec<REAL> &result);
  
  static void Jacobian(TPZFMatrix &coord, TPZVec<REAL> &par, TPZFMatrix &jacobian, TPZFMatrix &axes, REAL &detjac, TPZFMatrix &jacinv);
  
  
  /**
   * Creates a geometric element according to the type of the father element
   */
  static TPZGeoEl *CreateGeoElement(TPZGeoMesh &mesh, MElementType type,
                                    TPZVec<int>& nodeindexes,
                                    int matid,
                                    int& index);
  TPZGeoEl *CreateBCGeoEl(TPZGeoEl *orig,int side,int bc);	
};

#endif
