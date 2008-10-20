//
// C++ Interface: tpzgraphelpyramidmapped
//
// Description: 
//
//
// Author: Philippe R. B. Devloo <phil@fec.unicamp.br>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TPZGRAPHELPYRAMIDMAPPED_H
#define TPZGRAPHELPYRAMIDMAPPED_H

#include "pzgraphelq3dd.h"

/**
Implements the graphical element for a pyramid using a map to the cube element

	@author Philippe R. B. Devloo <phil@fec.unicamp.br>
*/
class TPZGraphElPyramidMapped : public TPZGraphElQ3dd
{
public:
    TPZGraphElPyramidMapped(TPZCompEl* cel, TPZGraphMesh* gmesh);

    ~TPZGraphElPyramidMapped();
 /**
  * This method maps the index of a point to parameter space as a function
  * of the number of divisions
  */
 virtual void QsiEta(TPZVec<int> &i, int imax, TPZVec<REAL> &qsieta);

};

#endif
