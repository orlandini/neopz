#include "TPBrDataControl.h"
#include <iostream>

int main(int argc, char *argv[])
{
  
      string FileName, dirname = PZSOURCEDIR;
      FileName = dirname + "/Projects/Plasticity/";
      FileName += "ensaio_all_columns.txt";

      REAL poisson, E, A, B, C, R, D, W;
      E = 29269;
      poisson = 0.203;
      A = 616.67;
      B = 0.0036895;
      C = 111.48;
      D = 0.018768;
      R = 0.91969;
      W = 0.006605;
      
      TPZSandlerDimaggio<SANDLERDIMAGGIOSTEP2> sandlerObj;
      sandlerObj.SetUp(poisson, E, A, B, C, R, D, W);
      DADOS.SetSandlerDimaggio(sandlerObj);
      
      TPBrLaboratoryData newLabFile (FileName);
      newLabFile.Set_start_idx(100);
      int idx_sim = newLabFile.RunSimulation(sandlerObj);
      int med_idx = DADOS.InsertLaboratoryData(newLabFile);
      
      std::cout << "END" << std::endl;

}