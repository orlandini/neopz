/**
 * @file
 * @brief Contains the implementation of the TPZHCurlNedFLinEl::Shape method.
 */

#ifdef HCURL_HIERARCHICAL_SCALED
#include "pzshapelinear.h"

using namespace pzshape;


void TPZHCurlNedFLinEl::Shape(TPZVec<REAL> &qsi, TPZFMatrix<REAL> &phi,
                              TPZFMatrix<REAL> &curlPhiHat) {
    const int nCon = NConnects();
    const int dim = Dimension();
    const int firstSide = TPZShapeLinear::NSides - TPZShapeLinear::NFaces - 1;

#ifdef PZDEBUG
    if (!(nCon == 1 && dim == 1 && firstSide == 0)) {
        DebugStop();
    }
#endif

    const int lastFuncPos =
        NConnectShapeF(nCon - 1, ConnectOrder(nCon - 1)) - 1;

    phi.Resize(lastFuncPos + 1, dim);
    curlPhiHat.Resize(1, 1); // The curl wont be calculated in boundary
                             // elements for now.
    const int pOrder = ConnectOrder(0);
    int currentFuncPos = lastFuncPos;

    switch (pOrder) {
    case 5:
        phi(currentFuncPos, 0) = 1 - 20 * qsi[0] + 90 * pow(qsi[0], 2) -
                                 140 * pow(qsi[0], 3) + 70 * pow(qsi[0], 4);
        currentFuncPos--;
    case 4:
        phi(currentFuncPos, 0) =
            -1 + 12 * qsi[0] - 30 * pow(qsi[0], 2) + 20 * pow(qsi[0], 3);
        currentFuncPos--;
    case 3:
        phi(currentFuncPos, 0) = 1 - 6 * qsi[0] + 6 * pow(qsi[0], 2);
        currentFuncPos--;
    case 2:
        phi(currentFuncPos, 0) = -1 + 2 * qsi[0];
        currentFuncPos--;
    case 1:
        phi(currentFuncPos, 0) = 1;
        break;
    default:
        DebugStop(); // polynomial order not implemented!
    }
}
#endif
