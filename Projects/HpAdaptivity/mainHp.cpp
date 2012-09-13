/*
 * @file
 * @author Denise de Siqueira
 * @since 6/9/11.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include "pzgengrid.h"
#include "pzgmesh.h"
#include "pzgeoelbc.h"
#include "pzcmesh.h"
#include "tpzcompmeshreferred.h"
#include "pzcompel.h"
#include "pzpoisson3d.h"
#include "pzbndcond.h"
#include "pzanalysiserror.h"
#include "pzanalysis.h"
#include "pzcmesh.h"
#include "pzstepsolver.h"
#include "TPZParFrontStructMatrix.h"
#include "pzmatrix.h"
#include "TPZCompElDisc.h"
#include "pzfstrmatrix.h"
#include "pzinterpolationspace.h"
#include "pzsubcmesh.h"
#include "pzlog.h"
#include "pzelctemp.h"
#include "pzelchdiv.h"
#include "pzshapequad.h"
#include "pzshapetriang.h"
#include "pzgeoquad.h"
#include "pzgeotriangle.h"
#include "pzfstrmatrix.h"
#include "pzgengrid.h"
#include "pzbndcond.h"
#include "pzmaterial.h"
#include "tpzquadrilateral.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "pzlog.h"
#include <cmath>
#include "Tools.h"

#ifdef LOG4CXX

static LoggerPtr logger(Logger::getLogger("HpAdaptivity.main"));

#endif

using namespace std;

/** Resolver o problema do tipo 
 * -Laplac(u) = 0
 * du/dn = lambda u em todo contorno
 */


using namespace std;
int main()
{
	
#ifdef LOG4CXX
	{
		InitializePZLOG();
		std::stringstream sout;
		sout<< "Testando p adaptatividade"<<endl;
		LOGPZ_DEBUG(logger, sout.str().c_str());
	}
#endif
	std::ofstream erro("Caulotaxa.txt");
	//std::ofstream GraficoSol("SolGraf.txt");
	//	std::ofstream CalcSolExata("CalSolExata.txt");
	TPZVec<REAL> calcErro;
	for (int porder=1; porder<2; porder++) {
		
		erro<<"ordem "<<porder <<std::endl;
			for(int h=1;h<2;h++){
			erro<<std::endl;
			erro<< "\n NRefinamento "<<h<<std::endl;
			//1. Criacao da malha geom. e computacional
			TPZGeoMesh *gmesh = MalhaGeo(h);
    std::ofstream file("MalhaPAdaptativa.vtk");
		PrintGMeshVTK( gmesh, file);

			
			TPZCompMesh *cmesh = CompMeshPAdap(*gmesh,porder);

            
			//2. Resolve o problema
		
		 TPZAnalysis analysis(cmesh);
					cmesh->SaddlePermute();
		 SolveLU ( analysis );
		//	ofstream file("Solutout");
    //        analysis.Solution().Print("solution", file);
			
			 
			//Pos processamento
//			std::ofstream SolPoissonHdiv("Solucao.txt");
//			analysis.Solution().Print("Solucao",SolPoissonHdiv);
//			std::ofstream SolP("teste.txt");
//			analysis.Print( "SolTeste" ,  SolP);
					
			
			
			
			//3. Calcula o erro 
					
		//	TPZVec<REAL> calcErro;
		//	analysis.SetExact(*SolExata);
		//	analysis.PostProcess(calcErro,erro);
			
			//4. visualizacao grafica usando vtk
//			 TPZVec<std::string> scalnames(1), vecnames(1);
//			 
//			// scalnames[0] = "Divergence";
//			 scalnames[0] = "Pressure";
//			// scalnames[1] = "ExactPressure";
//			 //	scalnames[2] = "ExactDiv";
//			 
//			 
//			// vecnames[0] = "ExactFlux";
//			 vecnames[0] = "Flux";
//			 //scalnames[2] = "Divergence";
//			 
//			 
//			 //vecnames[0] = "Derivate";
//			 
//			 
//			 std::string plotfile("GraficoSolution.vtk");
//			 const int dim = 2;
//			 int div = 2;
//			 analysis.DefineGraphMesh(dim,scalnames,vecnames,plotfile);
//			 analysis.PostProcess(div);
			 
			
		}}
	
	
	return 0;
}