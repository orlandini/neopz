/**
 * @file
 */

#include "pzgengrid.h"
#include "TPZExtendGridDimension.h"
#include "TPZVTKGeoMesh.h"

#include "MultiResMesh.h"
#include "pzbstrmatrix.h"

#include "pzmaterial.h"
#include "pzbndcond.h"
#include "pzelasmat.h"

//#include "pzadaptmesh.h"

#include <stdio.h>

#include "pzexplfinvolanal.h"
#include "adapt.h"

#include "pzlog.h"

#ifdef LOG4CXX
static LoggerPtr loggerconv(Logger::getLogger("pz.adaptivity.conv"));
static LoggerPtr loggerpoint(Logger::getLogger("pz.adaptivity.points"));
#endif

using namespace std;

/**
 * @addtogroup Tutorials
 * @{
 */

// Global variable
int gLMax;
int NUniformRefs = 2;
REAL alfa = M_PI/6.;
bool anothertests = false;
int nstate = 2;

/** Printing level */
int gPrintLevel = 0;
bool gDebug = false;

//TPZFNMatrix<16,REAL> Rot(4,4,0.),RotInv(4,4,0.);

void Exact(const TPZVec<REAL> &x, TPZVec<REAL> &sol, TPZFMatrix<REAL> &dsol);

void InitializeSolver(TPZAnalysis &an);
void InitialSolutionLinearConvection(TPZFMatrix<REAL> &InitialSol, TPZCompMesh *cmesh);
void PrintGeoMeshVTKWithDimensionAsData(TPZGeoMesh *gmesh,char *filename);

/**
 * @brief This project shows the creation of a rectangular mesh (two-dimensional) and the creation of a three-dimensional cube mesh using extrude method (ExtendMesh).
 */
int main(int argc, char *argv[]) {

#ifdef LOG4CXX
	if (argc > 1) {
		std::string logpath ( argv[1] );
		cout << "initializing LOG usign the following configuration file " << logpath << endl;
		InitializePZLOG ( logpath );	
	} else {
		cout << "initializing LOG\n";
		InitializePZLOG();
	}
#endif
	const int L = 4;
	gLMax = L-1;
#ifdef DEBUG
	char saida[260];
#endif
	
	//-----------  INITIALIZING CONSTRUCTION OF THE MESHES
	
	int r, dim = 2;
	
	// Initializing a ref patterns
	//gRefDBase.InitializeAllUniformRefPatterns();
	//gRefDBase.InitializeRefPatterns();
	gRefDBase.InitializeUniformRefPattern(EOned);
	gRefDBase.InitializeUniformRefPattern(EQuadrilateral);
	gRefDBase.InitializeUniformRefPattern(ETriangle);
	
	// To visualization of the geometric mesh
	std::ofstream fgeom("GeoMesh.vtk");
	// output files
    std::ofstream convergence("conv3d.txt");
    std::ofstream out("output.txt");

	cout << "Generating geometric mesh bi-dimensional ...\n";
    // First rectangular mesh:
	// The rectangular mesh has four corners: (0,0,0), (1,0,0), (1,1,0) and (0,1,0)
	// and was divides in two segments on X and two on Y, then hx = 0.5 and hy = 0.5
	// Has 4 elements, 9 connects and 8 bc elements
    TPZGeoMesh* gmesh = new TPZGeoMesh;
	TPZManVector<REAL> x0(3,0.), x1(3,1.);  // Corners of the rectangular mesh. Coordinates of the first extreme are zeros.
	TPZManVector<int> nx(dim,2);   // subdivisions in X and in Y. 
	TPZGenGrid gen(nx,x0,x1);    // mesh generator. On X we has three segments and on Y two segments. Then: hx = 0.2 and hy = 0.1  
	gen.SetElementType(0);       // type = 0 means rectangular elements
	gen.Read(gmesh);             // generating grid in gmesh
	
	/// Applying hp adaptive techniques 2012/10/01
	
	// Creating boundary condition on top and bottom of the quadrilateral domain
	gen.SetBC(gmesh,4,-1);
	gen.SetBC(gmesh,6,-2);
	
	// Refinement of the some element
	TPZVec<TPZGeoEl *> sub;
	for(int nele=0;nele<1;nele++) {
		TPZGeoEl *gel = gmesh->ElementVec()[nele];
		gel->Divide(sub);
		gel = sub[2];
		gel->Divide(sub);
	}
	gmesh->ResetConnectivities();
	gmesh->BuildConnectivity();
	
    // Creating computational mesh
    TPZCompMesh *comp = new TPZCompMesh(gmesh);
	/** Set polynomial order */
	int p = 1;
    TPZCompEl::SetgOrder(p);
  
	// Creating and inserting materials into computational mesh
    TPZMaterial * mat = new TPZElasticityMaterial(1,1.e5,0.2,0,0);
    comp->InsertMaterialObject(mat);
    // Boundary conditions
    // Dirichlet
    TPZFMatrix<REAL> val1(3,3,0.),val2(3,1,0.);
    TPZMaterial *bnd = mat->CreateBC(mat,-1,0,val1,val2);
    comp->InsertMaterialObject(bnd);
    bnd = mat->CreateBC (mat,-2,0,val1,val2);
	// Neumann
    TPZFMatrix<REAL> val3(3,3,1);
    val2(0,0)=1.;
    bnd = mat->CreateBC(mat,-2,1,val1,val2);
    comp->InsertMaterialObject(bnd);
    
    // Constructing and adjusting computational mesh
    comp->AutoBuild();
    comp->AdjustBoundaryElements();
    comp->CleanUpUnconnectedNodes();
    
    comp->SetName("Malha Computacional Com Refinamento");
	
	//--- END construction of the meshes
	
	// Printing geo mesh to check
	TPZVTKGeoMesh::PrintGMeshVTK(gmesh,fgeom);
	
	/** Variable names for post processing */
    TPZStack<std::string> scalnames, vecnames;
    scalnames.Push("POrder");
    scalnames.Push("Error");
    
    if(nstate == 1) {
        vecnames.Push("state");
        scalnames.Push("TrueError");
        scalnames.Push("EffectivityIndex");
    }else if(nstate == 2) {
        scalnames.Push("sig_x");
        scalnames.Push("sig_y");
        scalnames.Push("tau_xy");
        vecnames.Push("state");
    }
    if(dim < 3 && nstate == 2){
        vecnames.Push("displacement");
    }
	
	// END Determining the name of the variables
	
	if(anothertests) {
		// Second rectangular domain - subdivisions and corners of the second rectangular mesh
		TPZAutoPointer<TPZGeoMesh> gmesh2 = new TPZGeoMesh;
		x0[1] = 0.2;                 // left and right extremes of the new geo mesh. Coordinates: (0.,0.2,0.0) (3.,1.,0.) 
		x1[0] = 3.; x1[1] = 1.;
		nx[0] = 15; nx[1] = 8;       // subdivision in X and Y. hx = 0.2 and hy = 0.1
		TPZGenGrid gen2(nx,x0,x1);   // second mesh generator
		gen2.SetElementType(0);      // type = 0 means rectangular elements, type = 1 means triangular elements
		
		// Generating gmesh2 with last data and after this the gmesh is merged into the gmesh2. But gmesh is unmodified
		// if exist boundary elements into the mesh merged it will be deleted
		gen2.ReadAndMergeGeoMesh(gmesh2,gmesh);
		
		// setting bc condition -1 [no flux - is wall] from (0.0, 0.0) until (3.0, 0.2)
		x0[1] = 0.0; x1[1] = 0.2;
		gen2.SetBC(gmesh2,x0,x1,-1);
		// setting bc condition -1 from (3.0, 1.0) until (0.0, 1.0)
		x0[0] = 3.; x0[1] = 1.;
		x1[0] = 0.;	x1[1] = 1.;
		gen2.SetBC(gmesh2,x0,x1,-1);
		// setting bc condition -2 [free flux] from (3.0, 1.0) until (3.0, 0.2)
		x1[0] = 3.;	x1[1] = .2;
		gen2.SetBC(gmesh2,x1,x0,-2);
		// setting bc condition -2 [free flux] from (0.0, 1.0) until (0.0, 0.0)
		x0[0] = 0.;
		x1[0] = x1[1] = 0.;
		gen2.SetBC(gmesh2, x0, x1, -2);
		
#ifdef DEBUG
		sprintf(saida,"original.vtk");
		PrintGeoMeshVTKWithDimensionAsData(gmesh,saida);
		sprintf(saida,"meshes.vtk");
		PrintGeoMeshVTKWithDimensionAsData(gmesh2.operator->(),saida);
#endif
		
		// Extending geometric mesh (two-dimensional) to three-dimensional geometric mesh
		// The elements are hexaedras(cubes) over the quadrilateral two-dimensional elements
		TPZExtendGridDimension gmeshextend(gmesh2,0.3);
		
		TPZGeoMesh *gmesh3D = gmeshextend.ExtendedMesh(2,-5,-6);
#ifdef DEBUG
		sprintf(saida,"meshextruded.vtk");
		PrintGeoMeshVTKWithDimensionAsData(gmesh3D,saida);
#endif
		dim = 3;
	}
	
	// INITIAL POINT FOR SOLVING AND APPLYING REFINEMENT
	
	for(r=0;r<NUniformRefs;r++) {
		
		// Introduzing exact solution depending on the case
		TPZAnalysis an (comp);
		an.SetExact(Exact);
		
		{
			std::stringstream sout;
			int angle = (int) (alfa*180./M_PI + 0.5);
			sout << "hptestAngo" << angle << "." << r << ".vtk";
			an.DefineGraphMesh(dim,scalnames,vecnames,sout.str());
		}
		std::string MeshFileName;
		{
			std::stringstream sout;
			int angle = (int) (alfa*180./M_PI + 0.5);
			sout << "meshAngle" << angle << "." << r << ".vtk";
			MeshFileName = sout.str();
		}

		comp->SetName("Malha computacional adaptada");
		// Printing geometric and computational mesh
		// comp->Reference()->Print(std::cout);
		comp->Print(std::cout);
		
		// Solve using symmetric matrix then using Cholesky (direct method)
		TPZSkylineStructMatrix strskyl(comp);
		an.SetStructuralMatrix(strskyl);
		
		TPZStepSolver<REAL> *direct = new TPZStepSolver<REAL>;
		direct->SetDirect(ECholesky);
		an.SetSolver(*direct);
		delete direct;
		direct = 0;
		
		an.Run();
		
		// Post processing
		an.PostProcess(0,dim);
		{
			std::ofstream out(MeshFileName.c_str());
			comp->LoadReferences();
			TPZVTKGeoMesh::PrintCMeshVTK(comp->Reference(), out, false);
		}
		
		return 0;
		/*
		REAL valerror =0.;
		REAL valtruerror=0.;
		TPZVec<REAL> ervec,truervec,effect;
		
		TPZAdaptMesh adapt;
		adapt.SetCompMesh (comp);
		
		std::cout << "\n\n\n\nEntering Auto Adaptive Methods... step " << r << "\n\n\n\n";

		time_t sttime;
		time (& sttime);
		TPZCompMesh *adptmesh;
		
		adptmesh = adapt.GetAdaptedMesh(valerror,valtruerror,ervec,Exact,truervec,effect,0);
		
		time_t endtime;
		time (& endtime);
		
		int time_elapsed = endtime - sttime;
		std::cout << "\n\n\n\nExiting Auto Adaptive Methods....step " << r
		<< "time elapsed " << time_elapsed << "\n\n\n\n";
		
		int prt;
		std::cout << "neq = " << comp->NEquations() << " error estimate = " << valerror
		<< " true error " << valtruerror <<  " effect " << valerror/valtruerror << std::endl;
		
#ifdef LOG4CXX
		if (loggerconv->isDebugEnabled())
		{
			std::stringstream sout;
			sout << "neq = " << comp->NEquations() << " error estimate = " << valerror
			<< " true error " << valtruerror <<  " effect " << valerror/valtruerror << std::endl;
			LOGPZ_DEBUG(loggerconv, sout.str())
		}
#endif
		
		convergence  << comp->NEquations() << "\t"
		<< valerror << "\t"
		<< valtruerror << "\t"
		<< ( valtruerror / valerror ) <<  "\t"
		<< sttime <<std::endl;
		for (prt=0;prt<ervec.NElements();prt++){
			std::cout <<"error " << ervec[prt] << "  truerror = " << truervec[prt] << "  Effect " << effect[prt] << std::endl;
			// convergence << '\t' << ervec[prt] << '\t' << truervec[prt] << "  Effect " << effect[prt] <<  std::endl;
			//  adptmesh->Print(cout);
		}
		
		std::cout.flush();
		comp->Reference()->ResetReference();
		comp->LoadReferences();
		adapt.DeleteElements(comp);
		delete comp;
		comp = adptmesh;
		
		comp->CleanUpUnconnectedNodes();
		*/
	}
	/* Uniform refinement. Two times
	 UniformRefinement(2,gmesh3D,3);
	 
	 #ifdef DEBUG
	 sprintf(saida,"meshrefined.vtk");
	 PrintGeoMeshVTKWithDimensionAsData(gmesh3D,saida);
	 #endif
	 
	 // Creating a computational mesh (interpolation space)
	 TPZCompMesh *cmesh = CreateMeshMultires(gmesh3D);
	 #ifdef DEBUG
	 sprintf(saida,"aftercmesh.vtk");
	 PrintGeoMeshVTKWithDimensionAsData(gmesh3D,saida);
	 #endif
	 
	 // To work with the temporal variable.
	 REAL timeStep;
	 timeStep = ComputeTimeStep(1.,L,cmesh->Reference());
	 
	 #ifdef DEBUG
	 {
	 ofstream malhas("malhas.vtk");
	 cmesh->Print(malhas);
	 }
	 #endif
	 
	 TPZExplFinVolAnal an(cmesh, cout);
	 
	 InitializeSolver(an);
	 const double PhysicalTime = 0.1;
	 int niter = PhysicalTime/timeStep+1;
	 cout << "L = " << L << endl;
	 cout << "\nnequations = " << cmesh->NEquations();
	 cout << "\nNiter = " << niter << "\n";
	 
	 TPZFMatrix<REAL> InitialSol;
	 InitialSolutionLinearConvection(InitialSol,cmesh);
	 
	 an.SetInitialSolution(InitialSol);
	 
	 an.Set(timeStep,niter,1e-10);
	 an.SetSaveFrequency(niter/6,0);
	 
	 double Epsl = 1.e12;
	 an.MultiResolution( Epsl );
	 #ifdef DEBUG
	 sprintf(saida,"meshInitialSol.vtk");
	 TPZVTKGeoMesh::PrintGMeshVTK(gmesh3D,saida,0);
	 #endif
	 
	 return EXIT_SUCCESS;
	 */
}

#include "TPZGenSpecialGrid.h"

int main2() {
	// To visualization of the geometric mesh
	std::ofstream fgeom("GeoMeshByTolerance.vtk");
	std::ofstream fgeom2("GeoMeshByNRefinements.vtk");

	/** --- To test a polygonalized sphere using a tolerance defined */
	 TPZVec<REAL> center(3,-3.);
	REAL radius = 0.5;
	REAL tol = 0.002;
	 TPZGeoMesh *ggrid = TPZGenSpecialGrid::GeneratePolygonalSphereFromOctahedron(center,radius,tol);
	 TPZCompMesh *cgrid = new TPZCompMesh(ggrid);
	 TPZMaterial * mat = new TPZElasticityMaterial(1,1.e5,0.2,0,0);
	 cgrid->InsertMaterialObject(mat);
	 cgrid->AutoBuild();
	 std::cout << "N Elements = " << cgrid->NElements() << std::endl << "N G Elements = " << ggrid->NElements() << std::endl;
	 TPZVTKGeoMesh::PrintGMeshVTK(ggrid,fgeom);

	/** --- To test a polygonalized sphere using number of refinements */
	int nrefs = 5;
	TPZGeoMesh *ggrid2 = TPZGenSpecialGrid::GeneratePolygonalSphereFromOctahedron(center,radius,nrefs);
	TPZCompMesh *cgrid2 = new TPZCompMesh(ggrid2);
	cgrid2->InsertMaterialObject(mat);
	cgrid2->AutoBuild();
	std::cout << "N Elements = " << cgrid2->NElements() << std::endl << "N G Elements = " << ggrid2->NElements() << std::endl;
	TPZVTKGeoMesh::PrintGMeshVTK(ggrid2,fgeom2);
	
	 ////  ----  END SPHERE  -----  */
	return 0;
}
	
/** Exact solutions to calculate the rate of convergence */

static REAL onethird = 0.33333333333333333;
static REAL PI = 3.141592654;

void Exact(const TPZVec<REAL> &x, TPZVec<REAL> &sol, TPZFMatrix<REAL> &dsol) {
    TPZManVector<REAL,3> x2(x);
//    TransformInvX(x2,RotInv);
  	REAL r = sqrt(x2[0]*x2[0]+x2[1]*x2[1]);
  	REAL theta = atan2(x2[1],x2[0]);
#ifdef LOG4CXX
    if (loggerpoint->isDebugEnabled())
    {
        std::stringstream sout;
        sout << "Point " << x2;
        LOGPZ_DEBUG(loggerpoint, sout.str())
    }
#endif
  	REAL rexp = pow(r,onethird);
  	sol[0] = rexp*sin(onethird*(theta+PI/2));
    TPZFNMatrix<3,REAL> grad(4,1,0.),grad2(4,1,0.);
  	grad(0,0) = onethird*sin(onethird*(PI/2.-2.*theta))/(rexp*rexp);
  	grad(1,0) = onethird*cos(onethird*(PI/2.-2.*theta))/(rexp*rexp);
//    Rot.Multiply(grad, grad2);
    dsol(0,0) = grad2(0,0);
    dsol(1,0) = grad2(1,0);
}

void PrintGeoMeshVTKWithDimensionAsData(TPZGeoMesh *gmesh,char *filename) {
	int i, size = gmesh->NElements();
	TPZChunkVector<int> DataElement;
	DataElement.Resize(size);
	// Making dimension of the elements as data element
	for(i=0;i<size;i++) {
		if(gmesh->ElementVec()[i])
			DataElement[i] = (gmesh->ElementVec()[i])->Dimension();
		else
			DataElement[i] = -999;
	}
	// Printing geometric mesh to visualization in Paraview
	TPZVTKGeoMesh::PrintGMeshVTK(gmesh, filename, DataElement);
}
/*
 void PrintGeoMeshVTKWithData(TPZGeoMesh *gmesh,char *filename,int var) {
 int nrows = data.Rows();
 int ncols = data.Cols();
 int i, j, nvars;
 std::map<int, TPZAutoPointer <TPZMaterial> >::iterator m;
 for(m=gmesh->Reference()->MaterialVec().begin();m != gmesh->Reference()->MaterialVec().end();m++) {
 if(m->second->Id() > 0) {
 nvars = m->second->NStateVariables();
 break;
 }
 }
 if(m == gmesh->Reference()->MaterialVec().end() || var > nvars) {
 DebugStop();
 }
 TPZChunkVector<double> DataElement;
 int k = 0;
 char newname[1024];
 memset(newname,0,sizeof(newname));
 sprintf(newname,"%s",filename);
 char *p;
 DataElement.Resize(nrows/nvars);
 for(i=0;i<ncols;i++) {
 for(j=0;j<nrows;j+=nvars)
 DataElement[k++] = data.GetVal(j+var,i);
 
 p = strrchr(newname,'_');
 if(p) {
 sprintf(p,"%i.vtk",i);
 }
 else if(i) {
 p = strrchr(newname,'.');
 sprintf(newname,"_%i.vtk",i);
 }
 // Printing geometric mesh to visualization in Paraview
 TPZVTKGeoMesh::PrintGMeshVTK(gmesh, newname, DataElement);
 }
 }	*/

void InitializeSolver(TPZAnalysis &an) {
	TPZStepSolver<REAL> step;
	TPZBandStructMatrix matrix(an.Mesh());
	an.SetStructuralMatrix(matrix);
	step.SetDirect(ELU);
	an.SetSolver(step);
}

void InitialSolutionLinearConvection(TPZFMatrix<REAL> &InitialSol, TPZCompMesh * cmesh){
	InitialSol.Redim(cmesh->NEquations(),1);
	InitialSol.Zero();
	for(int iel = 0; iel < cmesh->NElements(); iel++){
		TPZCompEl * cel = cmesh->ElementVec()[iel];
		if(!cel) continue;
		TPZCompElDisc * disc = dynamic_cast<TPZCompElDisc*>(cel);
		if(!disc) continue;
		if(disc->NConnects() == 0) continue;
		int bl = disc->Connect(0).SequenceNumber();
		int blpos = cmesh->Block().Position(bl);
		int blocksize = cmesh->Block().Size(bl);
		
		TPZGeoEl * gel = cel->Reference();
		TPZVec<REAL> xi(3), xVec(3);
		gel->CenterPoint(gel->NSides()-1,xi);
		gel->X(xi,xVec);
		double x = xVec[0];
		double y = xVec[1];
		double u = 0.125;
		
		double xCircle = 0.25;
		double yCircle = 0.5;
		double R = 0.1;
		if( (x-xCircle)*(x-xCircle)+(y-yCircle)*(y-yCircle) <= R*R ) u = 1.;
		
		InitialSol(blpos+blocksize-20+0,0) = u;
		InitialSol(blpos+blocksize-20+1,0) = 0.;
		InitialSol(blpos+blocksize-20+2,0) = 0.;
		InitialSol(blpos+blocksize-20+3,0) = 0.;
		InitialSol(blpos+blocksize-20+4,0) = 0.;
		
	}//for iel
	
	TPZVec<REAL> celerity(3,0.);
	celerity[0] = 1.;
#ifdef LinearConvection
	TPZEulerEquation::SetLinearConvection(cmesh, celerity);
#endif
	
}//method

void UniformRefinement(const int nDiv, TPZGeoMesh *gmesh, const int dim, bool allmaterial, const int matidtodivided) {
	TPZManVector<TPZGeoEl*> filhos;
    for(int D=0; D<nDiv; D++)
    {
        int nels = gmesh->NElements();
        for(int elem = 0; elem < nels; elem++)
        {    
            TPZGeoEl * gel = gmesh->ElementVec()[elem];
			if(!gel || gel->HasSubElement())
				continue;
			if(dim > 0 && gel->Dimension() != dim) continue;
			if(!allmaterial){
				if(gel->MaterialId() == matidtodivided){
					gel->Divide(filhos);
				}
			}
			else{
				gel->Divide(filhos);
			}
        }
    }
	gmesh->ResetConnectivities();
	gmesh->BuildConnectivity();
}

/** @} */
