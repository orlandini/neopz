
#include "convhyper.h"
#include "pzfmatrix.h"
#include "pzvec.h"
#include "checkconv.h"
#include <stdlib.h>

int main() {

  int numnod = 3;
  int nn3 = 3*numnod;
  TPZConvHyper mat(numnod,1,1000., 0.3,500.,1000.);
  TPZFMatrix state(nn3,1,0.),range(nn3,1,0.);
  TPZVec<REAL> coefs(1,1.);
//  randomize();
  int i;
  REAL coef;
  cout << "\nCoeficiente ";
  cin >>coef;
  for(i=0; i<nn3; i++) {
    state(i,0) = coef*(rand()%100);
    range(i,0) = coef;
  }
  CheckConvergence<TPZConvHyper>(mat, state, range, coefs);
  int fim;
  cout << "\n>Fim\n";
  cin >> fim;
  return 0;
}
