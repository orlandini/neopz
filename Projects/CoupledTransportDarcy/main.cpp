//$Id: main.cpp,v 1.5 2006-03-16 01:53:09 tiago Exp $

/**
 * Percolation of water from the fracture into the porous media.
 * September 16, 2005
 */

#include <sstream>
#include "pzvec.h"
#include "pzcmesh.h"
#include "pzdebug.h"
#include "pzcheckgeom.h"
//#include "pzerror.h"

#include "pzgeoel.h"
#include "pzgnode.h"
#include "pzgeoelside.h"

//#include "pzintel.h"
#include "pzcompel.h"
#include "TPZCompElDisc.h"

#include "pzmatrix.h"

#include "pzanalysis.h"
#include "pzfstrmatrix.h"
#include "pzskylstrmatrix.h"
#include "TPZParFrontStructMatrix.h"
#include "TPZParFrontMatrix.h"
#include "TPZFrontNonSym.h"
#include "pzbdstrmatrix.h"
#include "pzblockdiag.h"
#include "TPZSpStructMatrix.h"
#include "TPZCopySolve.h"
#include "TPZStackEqnStorage.h"


#include "pzbstrmatrix.h"
#include "pzstepsolver.h"
#include "pzonedref.h"


#include "pzbndcond.h"
#include "pzpoisson3d.h"

#include "pzvisualmatrix.h"

#include "TPZGeoElement.h"
#include "pzgeoel.h"

#include "TPZGeoCube.h"
#include "pzshapecube.h"
#include "TPZRefCube.h"
#include "pzshapelinear.h"
#include "TPZGeoLinear.h"
#include "TPZRefLinear.h"
#include "pzrefquad.h"
#include "pzshapequad.h"
#include "pzgeoquad.h"
#include "pzshapetriang.h"
#include "pzreftriangle.h"
#include "pzgeotriangle.h"
#include "pzshapeprism.h"
#include "pzrefprism.h"
#include "pzgeoprism.h"
#include "pzshapetetra.h"
#include "pzreftetrahedra.h"
#include "pzgeotetrahedra.h"
#include "pzshapepiram.h"
#include "pzrefpyram.h"
#include "pzgeopyramid.h"
#include "pzrefpoint.h"
#include "pzgeopoint.h"
#include "pzshapepoint.h"
#include "pzskylstrmatrix.h"

#include <time.h>
#include <stdio.h>
#include "meshes.h"
#include "pzcoupledtransportdarcy.h"
#include "TPZTimer.h"

using namespace pzgeom;
using namespace pzshape;
using namespace pzrefine;
using namespace std;
 
int main22(){
  TPZFMatrix A(4,4), B(4,4), C(4,4);
  A(0,0) = 1.;
  A(0,1) = 2.;
  A(1,0) = 3.;
  A(1,1) = 4.;
  A(2,2) = 3.;
  A(3,3) = 4.;
  A.Print("",cout,EMathematicaInput); 
  int p;
  for(p = 0; p < 3; p++){
    B = A;  B.SetIsDecomposed(ENoDecompose);
    cout << "p = " << p << " - " << B.ConditionNumber(p, 200000, 1.e-14) << endl;
    B = A;  B.SetIsDecomposed(ENoDecompose);
    B.Inverse(C); C.SetIsDecomposed(ENoDecompose); B = A; B.SetIsDecomposed(ENoDecompose);
    cout << B.MatrixNorm(p) << "  " << C.MatrixNorm(p, 200000, 1.e-14) << endl;
  }
}//main

int main(){   

  int h, p;
  cout << "h p\n";
  cin >> h;
  cin >> p;

  TPZShapeLinear::fOrthogonal = TPZShapeLinear::Jacobi;
  TPZTimer WholeTime;
  WholeTime.start();

  TPZMaterial::gBigNumber= 1.e12;
  cout << "h = " << h << " - p = " << p << endl;
  TPZCompMesh * cmesh = /*CheckBetaNonConstant*/CreateSimpleMeshWithExactSolution2(h,p);
  std::cout << "Numero de elementos = " << cmesh->ElementVec().NElements() << std::endl;
  std::cout << "Numero de equacoes  = " << cmesh->NEquations() << std::endl;
  TPZGeoMesh *gmesh = cmesh->Reference();
 
  TPZAnalysis an(cmesh);

//#define ITER_SOLVER
#ifdef ITER_SOLVER
  cout << "ITER_SOLVER" << endl;  
  /*TPZFStructMatrix*/ /*TPZSpStructMatrix*/ TPZBandStructMatrix full(cmesh);
  an.SetStructuralMatrix(full);  
  TPZStepSolver step( full.Create() );
  an.SetSolver(step);  
  REAL tol = 1.e-11;
  /*TPZStepSolver*/ TPZCopySolve precond( full.Create() );
  REAL precondtol = 1.E-20;
//  precond.SetSSOR( 1, 1., precondtol, 0);     
//  precond.SetJacobi(1, precondtol, 0);
  step.ShareMatrix( precond );  
  step.SetGMRES( 20000, 30, precond, 1.e-16, 0 ); 
  cout << "SSOR/JACOBI PRECOND" << endl;
  an.SetSolver(step);
#endif  

#define DIRECT
#ifdef DIRECT  
  /*TPZParFrontStructMatrix*/TPZFrontStructMatrix <TPZFrontNonSym>/*TPZFStructMatrix*//*TPZBandStructMatrix*/ full(cmesh);
  full.Create();
  an.SetStructuralMatrix(full);
  TPZStepSolver step;
   step.SetDirect(ELU);
  an.SetSolver(step);
#endif

  std::cout << "Numero de equacoes = " << an.Mesh()->NEquations() << std::endl;

  TPZCoupledTransportDarcy::SetCurrentMaterial(0);
  std::cout << "\nCalling an.Run() for FirstEq\n";
  an.Assemble();
  for(int ii = 0; ii < 0*3; ii++)
  {
    TPZMatrix * fmat = step.Matrix();
    ofstream matrizfile("matriz.txt");
    fmat->Print("", matrizfile,EMathematicaInput);
    matrizfile.flush();
    int n = fmat->Rows();
    TPZFMatrix cp(n,n);
    for(int i = 0; i < n; i++) for(int j = 0; j < n; j++) cp(i,j) = fmat->Get(i,j);
    cout << " CN[" << ii << "] = " << cp.ConditionNumber(ii, 20000, 1e-10) << endl;
  }  
  an.Solve();
  an.SetExact(ExactSol_p); //  an.SetExact(SolExata);
  TPZVec<REAL> pos;
  an.PostProcess(pos,std::cout);
  std::cout << "Problem solved\n";
  
  {
  TPZVec<char *> scalnames(1);
  TPZVec<char *> vecnames(0);
  scalnames[0] = "Solution";
//  vecnames[0] = "Derivative";
  std::stringstream filedx;
  filedx << "1stEq_";
  filedx << "Solution.dx";
  an.DefineGraphMesh(2,scalnames,vecnames,&(filedx.str()[0]));
  an.PostProcess(4);  
  }      

  TPZCoupledTransportDarcy::SetCurrentMaterial(1);
  std::cout << "\nCalling an.Run() for SecondEq\n";
  an.Solution().Zero();
  an.Assemble();
  an.Solution().Zero();
  an.Solve();
  an.SetExact(ExactSol_u2);
  an.PostProcess(pos,std::cout);  
  std::cout << "Problem solved\n";  
   
  {
  TPZVec<char *> scalnames(1);
  TPZVec<char *> vecnames(0);
  scalnames[0] = "Solution";
//  vecnames[0] = "Derivative";
  std::stringstream filedx;
  filedx << "2ndEq_";
  filedx << "Solution.dx";
  an.DefineGraphMesh(2,scalnames,vecnames,&(filedx.str()[0]));
  an.PostProcess(4);  
  }    
    
  WholeTime.stop();
  cout.flush();
  std::cout << "Numero de equacoes = " << an.Mesh()->NEquations() << std::endl;
  cout << WholeTime << endl;
  cout << "Banda = " << cmesh->BandWidth() << endl;
  
  delete cmesh;
  delete gmesh;
  return 0; 
}

