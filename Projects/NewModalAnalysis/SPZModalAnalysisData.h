//
// Created by Francisco Teixeira Orlandini on 2/8/18.
//

#ifndef PZ_SPZMODALANALYSISDATA_H
#define PZ_SPZMODALANALYSISDATA_H

#include <slepceps.h>
#include <pzreal.h>
#include <pzvec.h>
#include "parameter_handler.h"

struct SPZModalAnalysisData{
private:
  ParameterHandler &prm;
  SPZModalAnalysisData();
  void DeclareParameters();
public:
  SPZModalAnalysisData(ParameterHandler &prm);
  void ReadParameters();
  struct SPZPhysicalOpts{
    std::string meshFile;
    bool isCutOff;
    REAL fOp;
    int nMaterials;
    TPZVec<STATE> urVec;
    TPZVec<STATE> erVec;
  };
  struct SPZPZOpts{
    // polynomial order of basis functions
    int pOrder;
// generate vtk for fields visualisation
    bool genVTK;
//vtk file name
    std::string vtkName;
//whether to calculate error analysis
    bool l2error;
//l2 file name
    std::string l2Name;
// export eigen values
    bool exportEigen;
//number of NeoPZ threads
    int nThreads;
    int nSim;
  };

  struct SPZSolverOpts{
    EPSConv eps_conv_test;
    EPSWhich eps_which_eig;
    PetscScalar target;
    PetscReal eps_tol;
    PetscInt eps_max_its;
    PetscInt eps_nev;
    PetscInt eps_ncv;
    PetscInt eps_mpd;
    PetscInt eps_verbose;

    PCType st_precond;
    KSPType st_solver;
    PetscReal ksp_rtol;
    PetscReal ksp_atol;
    PetscReal ksp_dtol;
    PetscReal ksp_max_its;
    STType st_type;

    EPSProblemType eps_prob_type;
    EPSType eps_type;
    bool eps_krylov_locking;
    PetscReal eps_krylov_restart;
    bool eps_true_res;
  };
};

#endif //PZ_SPZMODALANALYSISDATA_H
