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

static LoggerPtr logger(Logger::getLogger("Hdiv3D.main"));

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
		sout<< "Construindo Malhas Hdiv em 3D"<<endl;
		LOGPZ_DEBUG(logger, sout.str().c_str());
	}
#endif
	for (int porder=2; porder<3; porder++) {
		
			for(int h=1;h<2;h++){
			//1. Criacao da malha geom. e computacional
					bool hrefine=false;
					bool prefine=true;
			TPZGeoMesh *gmesh = MalhaGeoTetraedro(h,hrefine);
					//gmesh->Print();
    std::ofstream file("MalhaPAdaptativa.vtk");
		PrintGMeshVTK( gmesh, file);

			
			TPZCompMesh *cmesh = CompMeshPAdap(*gmesh,porder,prefine);

			
		}
	
	}
	
	
	return 0;
}