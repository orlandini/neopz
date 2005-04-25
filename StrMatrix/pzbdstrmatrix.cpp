/* Generated by Together */

#include "pzbdstrmatrix.h"
#include "pzblockdiag.h"
#include "pzvec.h"
#include "pzadmchunk.h"
#include "pzconnect.h"
#include "pzcmesh.h"
#include "pzskylmat.h"
#include "pzsubcmesh.h"
#include "pzgmesh.h"
#include "pzsolve.h"
#include "pzstepsolver.h"

#include "pzelcq2d.h"

using namespace std;
void TPZBlockDiagonalStructMatrix::AssembleBlockDiagonal(TPZBlockDiagonal & block){

  TPZVec<int> blocksizes;
  BlockSizes(blocksizes);
  block.Initialize(blocksizes);
  int nblock = blocksizes.NElements();
  TPZAdmChunkVector<TPZCompEl *> &elementvec = fMesh->ElementVec();
  TPZAdmChunkVector<TPZConnect> &connectvec = fMesh->ConnectVec();
  TPZStack<int> connectlist;
  TPZBlockDiagonal elblock;
  int numel = elementvec.NElements();
  int el;
  for(el=0; el<numel; el++) {
    TPZCompEl *cel = elementvec[el];
    if(!cel) continue;
    TPZBlockDiagonal eldiag;
    cel->CalcBlockDiagonal(connectlist,elblock);
    //    elblock.Print("Element block diagonal");
    int ncon = connectlist.NElements();
    int c,seqnum;
    for(c=0; c<ncon; c++) {
      TPZConnect &con = connectvec[connectlist[c]];
      seqnum = con.SequenceNumber();
      if(seqnum <0 || seqnum >= nblock) continue;
      int bsize = blocksizes[seqnum];
      if(con.NDof(*fMesh) != bsize ) {
	cout << "TPZBlockDiagonalStructMatrix::AssembleBlockDiagonal wrong data structure\n";
	continue;
      }
      TPZFMatrix temp(bsize,bsize);
      elblock.GetBlock(c,temp);
      block.AddBlock(seqnum,temp);
    }
  }
}

void TPZBlockDiagonalStructMatrix::BlockSizes(TPZVec < int > & blocksizes){

    if(fMesh->FatherMesh() != 0) {
      TPZSubCompMesh *mesh = (TPZSubCompMesh *) fMesh;
      mesh->PermuteExternalConnects();
    }
    int nblocks = 0;
    TPZAdmChunkVector<TPZConnect> &connectvec = fMesh->ConnectVec();
    int nc = connectvec.NElements();
    int c;
    for(c=0; c<nc; c++) {
        TPZConnect &con = connectvec[c];
        if(con.HasDependency() || con.SequenceNumber() < 0) continue;
        nblocks++;
    }
    blocksizes.Resize(nblocks);
    int bl,blsize;
    for(c=0; c<nc; c++) {
        TPZConnect &con = connectvec[c];
        if(con.HasDependency() || con.SequenceNumber() < 0) continue;
        bl = con.SequenceNumber();
        blsize = con.NDof(*fMesh);
        blocksizes[bl] = blsize;
    }
    if(fMesh->FatherMesh() != 0) {
      TPZSubCompMesh *mesh = (TPZSubCompMesh *) fMesh;
      int nc = mesh->NConnects();
      blocksizes.Resize(nblocks-nc);
    }
}

TPZStructMatrix * TPZBlockDiagonalStructMatrix::Clone(){
    return new TPZBlockDiagonalStructMatrix(fMesh);
}
TPZMatrix * TPZBlockDiagonalStructMatrix::CreateAssemble(TPZFMatrix &rhs){
  int neq = fMesh->NEquations();
  //cout << "TPZBlockDiagonalStructMatrix::CreateAssemble will not assemble the right hand side\n";
  TPZBlockDiagonal *block = new TPZBlockDiagonal();
  rhs.Redim(neq,1);
  fMesh->Assemble(rhs);
  AssembleBlockDiagonal(*block);
//  block->Print("Block Diagonal matrix");
  return block;
}
TPZMatrix * TPZBlockDiagonalStructMatrix::Create(){
  TPZVec<int> blocksize;
  BlockSizes(blocksize);
  return new TPZBlockDiagonal(blocksize);
}
TPZBlockDiagonalStructMatrix::TPZBlockDiagonalStructMatrix(TPZCompMesh *mesh) : TPZStructMatrix(mesh)
{}

int TPZBlockDiagonalStructMatrix::main() {
  TPZCompMesh *cmesh = TPZCompElQ2d::CreateMesh();
  TPZGeoMesh *gmesh = cmesh->Reference();
  TPZBlockDiagonalStructMatrix blstr(cmesh);
  TPZBlockDiagonal *bldiag = (TPZBlockDiagonal *) blstr.Create();
  blstr.AssembleBlockDiagonal(*bldiag);
  bldiag->Print("BlockDiagonal from assembly");
  int neq = cmesh->NEquations();
  TPZFMatrix *glob = new TPZFMatrix(neq,neq,0.);
  TPZFMatrix rhs(neq,1,0.);
  cmesh->Assemble(*glob,rhs);
  //glob.Print("Global Matrix");
  TPZBlockDiagonal *bldiag2 = (TPZBlockDiagonal *) blstr.Create();
  bldiag2->BuildFromMatrix(*glob);
  bldiag2->Print("block diagonal built from global matrix\n");
  TPZStepSolver sol1(glob);
  sol1.SetJacobi(1,0.,0);
  //  TPZFMatrix *glob2 = new TPZFMatrix(*glob);
  TPZStepSolver sol2(sol1);
  sol2.SetCG(100,sol1,1.e-6,1);
  TPZFMatrix solution;
  sol2.Solve(rhs,solution);
  solution.Print("solution of the iterative method");
  delete cmesh;
  delete gmesh;
  delete bldiag;
  delete bldiag2;
  return 0;
}
