/**
 * @file
 * @brief Contains the implementation of the TPZHCurlNedFLinEl::Shape method.
 */

#ifdef HCURL_HIERARCHICAL_SCALED
#include "pzshapelinear.h"

using namespace pzshape;

void TPZHCurlNedFLinEl::CalcShape(TPZVec<REAL> &qsi, TPZFMatrix<REAL> &phi,
                              TPZFMatrix<REAL> &curlPhiHat, TPZVec<int> &order, TPZVec<int> nShapeF){

    

    const int nCon = order.size();
    const int dim = TPZShapeLinear::Dimension;
    const int firstSide = TPZShapeLinear::NSides - TPZShapeLinear::NFaces - 1;

#ifdef PZDEBUG
    if (!(nCon == 1 && dim == 1 && firstSide == 0)) {
        DebugStop();
    }
#endif

    const int lastFuncPos = nShapeF[nCon - 1] - 1;

    phi.Resize(lastFuncPos + 1, dim);
    curlPhiHat.Resize(1, 1); // The curl wont be calculated in boundary
                             // elements for now.
    const int pOrder = order[0];
    int currentFuncPos = lastFuncPos;
    const REAL x = (1. + qsi[0])/2;
    switch (pOrder) {
    case 5:
        phi(currentFuncPos, 0) = 1 - 20 * x + 90 * pow(x, 2) -
                                 140 * pow(x, 3) + 70 * pow(x, 4);
        currentFuncPos--;
    case 4:
        phi(currentFuncPos, 0) =
            -1 + 12 * x - 30 * pow(x, 2) + 20 * pow(x, 3);
        currentFuncPos--;
    case 3:
        phi(currentFuncPos, 0) = 1 - 6 * x + 6 * pow(x, 2);
        currentFuncPos--;
    case 2:
        phi(currentFuncPos, 0) = -1 + 2 * x;
        currentFuncPos--;
    case 1:
        phi(currentFuncPos, 0) = 1;
        break;
    default:
        std::cout<<"Polynomial order "<<pOrder<<" is not implemented!"<<std::endl;
        DebugStop(); // polynomial order not implemented!
    }
}
#endif
