
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
#include "pzplaca.h"
#include "pzintel.h"

#include "TPZGeoElement.h"
#include "pzgmesh.h"
#include "pzcmesh.h"
#include "pzfmatrix.h"
#include "pzelgc3d.h"
#include "pzbndcond.h"
#include "pztempmat.h"
#include "pzcompel.h"
#include "pzanalysis.h"
#include "pzfstrmatrix.h"
#include "pzskylstrmatrix.h"
#include "pzskylmat.h"
#include "pzstepsolver.h"
#include "pzgeoel.h"
#include "pzgnode.h"
#include "pzstack.h"
#include "pzvec.h"
#include "pzsolve.h"
#include "pzelgpoint.h"
#include "pzelg1d.h"
#include "pzelgq2d.h"
#include "pzelgt2d.h"
#include "pzelct2d.h"
#include "pzelcc3d.h"
#include "pzelgt3d.h"
#include "pzelct3d.h"
#include "pzelgpi3d.h"
#include "pzelcpi3d.h"
#include "pzelgpr3d.h"
#include "pzelcpr3d.h"
#include "pzelmat.h"
#include "pzelasmat.h"
#include "pzmattest.h"
#include "pzmat1dlin.h"
#include "pzmat2dlin.h"
#include "pzpoisson3d.h"
#include "pzmaterial.h"
#include "TPZConservationLaw.h"
#include "TPZConsLawTest.h"
#include "TPZEulerConsLaw.h"
#include "TPZDiffusionConsLaw.h"
#include "TPZCompElDisc.h"
#include "TPZShapeDisc.h"
#include "TPZInterfaceEl.h"
#include "pzreal.h"
#include "pzdxmesh.h"
#include "pzmatorthotropic.h"
#include "TPZMulticamadaOrtho.h"
#include "TPZPlacaOrthotropic.h"
//#include "TPZSurfaceInteg.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <ostream>
#include <string.h>
#include <math.h>
using namespace std;

void ProcessamentoLocal(TPZGeoMesh &gmesh,ostream &out);
void CriaNos(int num, TPZGeoMesh &geomesh, double list [20][3]);
void CriaElementos(int numelem,int ncon,TPZGeoMesh &geomesh, int list[20][8] );
void CriaCondCont(TPZGeoMesh &gmesh);
void ExecutaAnalysis(TPZCompMesh &cmesh,TPZMaterial *mat);
void PosProcessamento(TPZAnalysis &an);
void Print(TPZGeoMesh *geomesh);
void CriaCondCont2(TPZGeoMesh &gmesh);
//static double NosPlaca[8][3] = {{




/**
 * -> PROGRAMA PRINCIPAL <- -> PROGRAMA PRINCIPAL <- -> PROGRAMA PRINCIPAL <- -> PROGRAMA PRINCIPAL <-
 */

int main(){
  int ord, quantplacas;

//criando objetos das classes computacional e geométrica
  TPZMulticamadaOrthotropic *multcam = new TPZMulticamadaOrthotropic(0., 2.0, 1.5,1,1);


  TPZFMatrix naxes(3,3);
  //  REAL E = 200000.0, G = 173913.0, v = 0.15;
  //REAL E = 10000.0, G = 4347.83, v = 0.15;//teste
  //REAL E = 15.133e6, G = 2.59398e7, v = 0.471;//Pinus Pinaster ait.
 //  REAL E = 69.0e6, G = 5.14378e6, v = 0.330;//Aluminio
//   REAL eppx=E, eppy=E, eppz=E, vxy=v, vyz=v,vzx=v, gxy=G, gyz=G, gzx=G;
//  int matindex = 1;

  naxes(0,0) =  1.0;    naxes(0,1) =  0.0;    naxes(0,2) =  0.0;
  naxes(1,0) =  0.0;    naxes(1,1) =  1.0;    naxes(1,2) =  0.0;
  naxes(2,0) =  0.0;    naxes(2,1) =  0.0;    naxes(2,2) =  1.0;

  //criando objeto para a classe material
  TPZMatOrthotropic *orto;
  orto = new TPZMatOrthotropic(1, naxes, 69.e06, 69.e06, 69.e06, 0.33, 0.33, 0.33, 5.15378e06, 5.15378e06, 5.15378e06) ;
  multcam->AddPlacaOrtho(orto,0.2);

  multcam->GenerateMesh();

  multcam->SetMX(1.);

  multcam->ComputeSolution();

  multcam->ComputeCenterForces();

  return 0;
}

