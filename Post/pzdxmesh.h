#ifndef DXMESHH
#define DXMESHH

#include "pzgraphmesh.h"
#include "pzstack.h"

/// This class implements the interface of the graphmesh to the OpenDX graphics package
/**
This is the most actively supported graphics interface of the pz environment
*/
class TPZDXGraphMesh : public TPZGraphMesh {

  int fNextDataField;
  int fNumConnectObjects[8];
  int fElConnectivityObject[8];
  int fNodePosObject[8];
  int fNormalObject;
  TPZStack<REAL> fTimes;
  TPZStack<int> fFirstFieldValues[3];
  int fNumCases;
  std::string fElementType;

public:

  TPZDXGraphMesh(TPZCompMesh *mesh, int dimension, TPZAutoPointer<TPZMaterial> mat, const TPZVec<std::string> &scalarnames,const TPZVec<std::string> &vecnames);
  TPZDXGraphMesh(TPZCompMesh *cmesh,int dim,TPZDXGraphMesh *graph,TPZAutoPointer<TPZMaterial> mat);

  virtual ~TPZDXGraphMesh();


  virtual void DrawMesh(int numcases);
  virtual void DrawSolution(TPZBlock &Sol);
  virtual void DrawSolution(char * var = 0);
  virtual void DrawSolution(int step, REAL time);

  int  NNodes();
  std::string ElementName();
  void Close();
	
	/** Jorge 16/06/2001 */
	/** To redefinition of the graph mesh */
  void SetNumCases(int numcases) { fNumCases = numcases; }

private:
  void DrawNormals(int numnormals);
};

#endif
