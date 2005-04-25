/***************************************************************************
                          pzgeoelrefless.h  -  description
                             -------------------
    begin                : Fri Dec 12 2003
    copyright            : (C) 2003 by phil
    email                : phil@localhost
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PZGEOELREFLESS_H
#define PZGEOELREFLESS_H

#include <pzgeoel.h>

class TPZGeoElSide;
class TPZCompMesh;
class TPZCompEl;
template<class T,int N>
class TPZStack;

/**Implements a class which subelement is the clone of the element (i.e. the same nodes, material, but a self pointer)
  *@author phil
  */

/// Implements a generic geometric element class without h-refinement
/**
This class implements the mapping between the master element and deformed element
Its data structure is the vector of node indices and element neighbours
@ingroup geometry
*/
template <class TShape, class TGeo>
class TPZGeoElRefLess : public TPZGeoEl  {
//  int fSubElement;
protected:
  int fNodeIndexes[TGeo::NNodes];
  TPZGeoElSide fNeighbours[TShape::NSides];
public:

  ~TPZGeoElRefLess();
  TPZGeoElRefLess();
  TPZGeoElRefLess(TPZGeoElRefLess &gel);
  TPZGeoElRefLess(int id,TPZVec<int> &nodeindexes,int matind,TPZGeoMesh &mesh);
  TPZGeoElRefLess(TPZVec<int> &nodeindices,int matind,TPZGeoMesh &mesh);
  TPZGeoElRefLess(TPZVec<int> &nodeindices,int matind,TPZGeoMesh &mesh,int &index);

  virtual void Read(TPZStream &str, void *context);
  
  virtual void Write(TPZStream &str, int withclassid);
  
  virtual void Initialize(TPZVec<int> &nodeindices,int matind,TPZGeoMesh &mesh,int &index);
  
  static TPZCompEl *(*fp)(TPZGeoEl *el,TPZCompMesh &mesh,int &index);

  static int main_refless();
  
  /** divides the element and puts the resulting elements in the vector */
  virtual void Divide(TPZVec < TPZGeoEl * > & pv) = 0;

  /** return 1 if the element has subelements along side */
  //virtual int HasSubElement();
  virtual  TPZCompEl *CreateCompEl(TPZCompMesh &mesh,int &index);

  static void SetCreateFunction(TPZCompEl *(*f)(TPZGeoEl *el,TPZCompMesh &mesh,int &index));

  /** return 1 if the element has subelements along side*/
  virtual  int HasSubElement() {return 0;}//fSubEl[0]!=0;}

  /**
   * returns a pointer to the neighbour and the neighbourside
   * along side of the current element
   */
   virtual  TPZGeoElSide Neighbour(int side) { return fNeighbours[side]; }

   virtual  int NodeIndex(int node);

  /**fill in the data structure for the neighbouring information*/
  virtual  void SetNeighbour(int side,const TPZGeoElSide &neighbour){
    fNeighbours[side]=neighbour;
  }

  virtual  int SideNodeIndex(int side,int node);

  virtual  int SideNodeLocIndex(int side,int node);

  /**flags the side as defined, this means no neighbouring element
   * was found*/
   virtual  void SetSideDefined(int side) { fNeighbours[side] = TPZGeoElSide(this,side); }

   virtual  void SetSubElement(int id, TPZGeoEl *el);

  /**
   * Creates an integration rule for the topology of the corresponding side
   * and able to integrate a polynom of order exactly
   */
   virtual  TPZIntPoints * CreateSideIntegrationRule(int side, int order);

  /**
   * returns the type of the element acording to the definition in pzeltype.h
   */
   virtual  MElementType Type() {
     return TGeo::Type();
   }
   
  /**
   * returns the type of the element acording to the definition in pzeltype.h
   */
   virtual  MElementType Type(int side) {
     return TGeo::Type(side);
   }
   
   /**
    * return the type of the element as a string
    */
    virtual std::string TypeName()
    {
      return TGeo::TypeName();
    }
   
  /**return the number of nodes of the element*/
  virtual  int NNodes();

  /**return the number of corner nodes of the element*/
  virtual  int NCornerNodes();

  /**return the number of connectivities of the element*/
  virtual  int NSides();

 /**
  * returns the local node number of the node "node" along side "side"
  */
  virtual  int SideNodeLocId(int side, int node);

  /**volume of the master element*/
  virtual  REAL RefElVolume();

  /**return the number of nodes for a particular side*/
  virtual  int NSideNodes(int side);

  /**returns the midside node index along a side of the element*/
  virtual  void MidSideNodeIndex(int side,int &index);

  /**returns 1 if the side has not been defined by buildconnectivity
     After construction the side is undefined. The buildconnectivity method
     loops over all elements and tries to identify neighbours along their
     uninitialized sides*/
  virtual  int SideIsUndefined(int side);

  /**
   * return the number of subelements of the element independent of the
   * fact hether the element has already been refined or not
   */
  virtual  int NSubElements();

  /**return the number of subelements of the same dimension of the element at the side*/
  virtual  int NSideSubElements(int side);

  /**
  * return the number of subelements as returned by GetSubElements2(side)
  */
  virtual  int NSideSubElements2(int side);

  /**
   * method which creates a computational boundary condition element based
   * on the current geometric element, a side and a boundary condition number
   */
   virtual  TPZGeoEl *CreateBCGeoEl(int side, int bc);

  /**initializes the node i of the element*/
  virtual  void SetNodeIndex(int i,int nodeindex);

  /**
   * compute the transformation between the master element space of one side
   * of an element to the master element space of a higher dimension side
   */
  virtual  TPZTransform SideToSideTransform(int sidefrom,int sideto);

  /**returns a pointer to the subelement is*/
  virtual  TPZGeoEl *SubElement(int is);

  /**return a pointer and a side of the subelement of the element at the side
     and the indicated position. position = 0 indicate first subelement, ...*/
//  virtual  TPZGeoElSide SideSubElement(int side,int position);

  /**return the dimension of side*/
  virtual  int SideDimension(int side);

  /**Returns the dimension of the element*/
  virtual int Dimension();

  virtual  TPZGeoElSide HigherDimensionSides(int side,int targetdimension);

  virtual  void AllHigherDimensionSides(int side,int targetdimension,TPZStack<TPZGeoElSide> &elsides);

  virtual  void LowerDimensionSides(int side,TPZStack<int> &smallsides);

  /**accumulates the transformation of the jacobian which maps the current
     master element space into the space of the master element of the father*/
  virtual  void BuildTransform(int side, TPZGeoEl *father,TPZTransform &t);

  virtual  TPZTransform BuildTransform2(int side, TPZGeoEl *father,TPZTransform &t);

  /**return the Jacobian matrix at the point*/
  virtual  void Jacobian(TPZVec<REAL> &coordinate,TPZFMatrix &jac,TPZFMatrix &axes,REAL &detjac,TPZFMatrix &jacinv);

  /**return the coordinate in real space of the point coordinate in the master element space*/
  virtual  void X(TPZVec<REAL> &coordinate,TPZVec<REAL> &result);

  virtual  TPZTransform GetTransform(int side,int son);

  /**
   * It returns the coordinates from the center of the side of the element
   */
  virtual void CenterPoint(int side, TPZVec<REAL> &masscent);

  virtual TPZGeoElSide Father2(int side);

  virtual int FatherSide(int side, int son) {
    return side;//TRef::FatherSide(side,son);
  }

  virtual void GetSubElements2(int side, TPZStack<TPZGeoElSide> &subel);

};

template<class TShape, class TGeo>
inline
TPZCompEl *TPZGeoElRefLess<TShape,TGeo>::CreateCompEl(TPZCompMesh &mesh,int &index){
  return fp(this,mesh,index);
}

template<class TShape, class TGeo>
inline
void TPZGeoElRefLess<TShape,TGeo>::SetCreateFunction(TPZCompEl *(*f)(TPZGeoEl *el,TPZCompMesh &mesh,int &index)){
  fp = f;
}
#endif
