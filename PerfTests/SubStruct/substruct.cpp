/**
 * @file
 * @brief Tests for sub structuration
 * @author Philippe Devloo
 * @since 2006
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>

#include "tpzdohrsubstruct.h"
#include "tpzdohrmatrix.h"
#include "tpzdohrprecond.h"
#include "pzdohrstructmatrix.h"
#include "pzstepsolver.h"
#include "pzcompel.h"
#include "pzgeoelbc.h"

#include "pzelast3d.h"
#include "pzbndcond.h"

#include "tpzdohrassembly.h"

#include "pzlog.h"
#include "tpzgensubstruct.h"
#include "tpzpairstructmatrix.h"
#include "pzviscoelastic.h"
#include "TPZVTKGeoMesh.h"
#include "pzgeotetrahedra.h"
#include "pzskylstrmatrix.h"

#include "tpzarc3d.h"
#include "tpzdohrmatrix.h"

#include "pzvtkmesh.h"

#include "pzlog.h"

#include "pzbfilestream.h" // TPZBFileStream

#include <fstream>
#include <string>

#ifdef LOG4CXX
static LoggerPtr loggerconverge(Logger::getLogger("pz.converge"));
static LoggerPtr logger(Logger::getLogger("main"));
#endif

#include "clock_timer.h"
#include "timing_analysis.h"
#include "arglib.h"

#ifdef HAS_GETRUSAGE
#include <sys/resource.h> // getrusage
#endif

void InsertElasticity(TPZAutoPointer<TPZCompMesh> mesh);
void InsertViscoElasticity(TPZAutoPointer<TPZCompMesh> mesh);
void InsertViscoElasticityCubo(TPZAutoPointer<TPZCompMesh> mesh);
TPZGeoMesh *MalhaPredio();
TPZGeoMesh *MalhaCubo();
void SetPointBC(TPZGeoMesh *gr, TPZVec<REAL> &x, int bc);
REAL Height(TPZGeoMesh *gmesh);
int SubStructure(TPZAutoPointer<TPZCompMesh> cmesh, REAL height);

using namespace std;

void help(const char* prg)
{
  cout << "Compute ...." << endl;
  cout << "The application is divided in three main steps: s1, s2 and s3" << endl;
  cout << endl;
  cout << "  Step 1 description: ... " << endl;
  cout << "  Step 2 description: ... " << endl;
  cout << "  Step 3 description: ... " << endl << endl;
  cout << "Usage: " << prg << "starting_point file [-of output_file]" << endl << endl;
  cout << " starting_point = {-cf1|-cf2|-cf3|-mc|-mp} input_file" << endl;

  clarg::arguments_descriptions(cout, "  ", "\n");
} 


clarg::argString cf1("-cf1", "starts execution from checkpoint 1 (read checkpoint file)", "ckpt1.ckpt");
clarg::argString cf2("-cf2", "starts execution from checkpoint 2 (read checkpoint file)", "ckpt2.ckpt");
clarg::argString cf3("-cf3", "starts execution from checkpoint 3 (read checkpoint file)", "ckpt3.ckpt");
clarg::argString dc1("-dc1", "dump checkpoint 1 to file", "ckpt1.ckpt");
clarg::argString dc2("-dc2", "dump checkpoint 2 to file", "ckpt2.ckpt");
clarg::argString dc3("-dc3", "dump checkpoint 3 to file", "ckpt3.ckpt");
clarg::argString mc("-mc", "starts execution from beginning - read a \"malha_cubo\" input file",
		    "../cube1.txt");
clarg::argString mp("-mp", "starts execution from beginning - read a \"malha_predio\" input file",
		    "../8andares02.txt");

clarg::argInt plevel     ("-p", "plevel", 1);
clarg::argInt nt_submeshs("-nt_sm", "number of threads to process the submeshes", 0);
clarg::argInt nt_assemble("-nt_a", "number of threads to assemble each submesh", 0);
clarg::argInt nt_decompose("-nt_d", "number of threads to decompose each submesh", 0);
clarg::argInt nt_multiply("-nt_m", "number of threads to multiply", 0);
clarg::argInt nsub("-nsub", "number of substructs", 4);

clarg::argInt dim_arg("-dim", "dim???", 3);
clarg::argInt maxlevel("-maxlevel", "maxlevel???", 5);
clarg::argInt sublevel("-sublevel", "sublevel???", 3);

clarg::argInt verb_level("-v", "verbosity level", 0);

clarg::argBool h("-h", "help message", false);

int main(int argc, char *argv[])
{
#ifdef LOG4CXX
  InitializePZLOG("log4cxx.cfg");
#endif
  
  /* Parse the arguments */
  if (clarg::parse_arguments(argc, argv)) {
    cerr << "Error when parsing the arguments!" << endl;
    return 1;
  }

  if (h.get_value() == true) {
    help(argv[0]);
    return 1;
  }

  /* Verbose macro. */
  unsigned verbose = verb_level.get_value();
#define VERBOSE(level,...) if (level <= verbose) cout << __VA_ARGS__

  if (verbose >= 1) {
    std::cout << "- Arguments -----------------------" << std::endl;
    clarg::values(std::cout, false);
    std::cout << "-----------------------------------" << std::endl;
  }

  if (!mp.was_set() && !mc.was_set() && !cf1.was_set() && 
      !cf2.was_set() && !cf3.was_set()) 
  {
    cerr << "A \"starting_point\" must be provided!" << endl;
    help(argv[0]);
    return 1;
  }

  /* Measure time. */
  ClockTimer timer;
  ClockTimer total_timer;
  //TimingAnalysis ta;	
  total_timer.start();

  TPZPairStructMatrix::gNumThreads = nt_assemble.get_value();

  TPZGeoMesh  *gmesh = 0;
  TPZAutoPointer<TPZCompMesh> cmeshauto = 0;
  TPZDohrStructMatrix* dohrstruct = 0;
  TPZFMatrix<REAL> *rhs = NULL;
  TPZMatrix<REAL> *matptr = 0;
  int dim = dim_arg.get_value();
  TPZCompEl::SetgOrder(plevel.get_value());    

  bool running = false;

  /* Start from malha_cubo or malha_predio? */
  if (mp.was_set() || mc.was_set())
    {
      if (mp.was_set()) // Predio Elastisco
	{
	  if (running) {
	    cerr << "ERROR: you must select only one of the start modes: "
		 << "mp, mc, cf1, cf2 or cf3" << endl;
	    exit(1);
	  }
	  else  
	    running = true;
	  
	  gmesh = MalhaPredio();
	  cmeshauto = new TPZCompMesh(gmesh);
	  cmeshauto->SetDimModel(dim);
	  InsertElasticity(cmeshauto);
	  cmeshauto->AutoBuild();
	}
      if (mc.was_set()) // Cubo Viscoso
	{
	  if (running) {
	    cerr << "ERROR: you must select only one of the start modes: "
		 << "mp, mc, cf1, cf2 or cf3" << endl;
	    exit(1);
	  }
	  else  running = true;
	  
	  VERBOSE(1, "Reading MalhaCubo from file: " << mc.get_value() << endl);
	  gmesh = MalhaCubo();
	  cmeshauto = new TPZCompMesh(gmesh);
	  cmeshauto->SetDimModel(dim);
	  cmeshauto->SetDefaultOrder(plevel.get_value());
	  //cmeshauto->SetAllCreateFunctionsContinuousWithMem();
	  //cmeshauto->SetAllCreateFunctionsContinuous();
	  InsertViscoElasticityCubo(cmeshauto);
	  cmeshauto->AutoBuild();
	}
      
      VERBOSE(1, "Number of equations " << cmeshauto->NEquations() << endl);
      
      dohrstruct = new TPZDohrStructMatrix(cmeshauto,nt_submeshs.get_value(),
					   nt_decompose.get_value());
      dohrstruct->IdentifyExternalConnectIndexes();
      
      VERBOSE(1, "Substructuring the mesh" << endl);
      
      dohrstruct->SubStructure(nsub.get_value());

#ifdef LOG4CXX
      {
	std::stringstream str;
	cmeshauto->Print(str);
	LOGPZ_DEBUG(logger,str.str());
      }
#endif

      /* Dump checkpoint 1? */
      if (dc1.was_set() && running)
      {
	VERBOSE(1, "Dumping checkpoint 1 into: " << dc1.get_value() << endl);
	TPZFileStream CheckPoint1;
	CheckPoint1.OpenWrite(dc1.get_value());
	cmeshauto->Reference()->Write(CheckPoint1, 0);
	cmeshauto->Write(CheckPoint1, 0);
	dohrstruct->Write(CheckPoint1);
      }

      matptr = dohrstruct->Create();
    }

    // Start from Checkpoint 1
    if (cf1.was_set())
    {
      if (running) {
	cerr << "ERROR: you must select only one of the start modes: mp, mc, cf1, cf2 or cf3" << endl;
	exit(1);
      }
      else  
	running = true;

      gmesh = new TPZGeoMesh;
      cmeshauto = new TPZCompMesh(gmesh);
      dohrstruct = new TPZDohrStructMatrix(cmeshauto,nt_submeshs.get_value(),nt_decompose.get_value());
      /* Read the checkpoint. */
      {
	TPZFileStream CheckPoint1;
	CheckPoint1.OpenRead(cf1.get_value());
	gmesh->Read(CheckPoint1, 0);
	cmeshauto->Read(CheckPoint1, gmesh);
	dohrstruct->Read(CheckPoint1);
      }

      dim = cmeshauto->Dimension();
      VERBOSE(1, "Reading dim from file. new dim = " << dim << ", old dim = " << dim_arg.get_value() << endl);

      matptr = dohrstruct->Create();
    }

    if (dc2.was_set() && running)
    {
      VERBOSE(1, "Dumping checkpoint 2 into: " << dc2.get_value() << endl);
      TPZFileStream CheckPoint2;
      CheckPoint2.OpenWrite(dc2.get_value());
      cmeshauto->Reference()->Write(CheckPoint2, 0);
      cmeshauto->Write(CheckPoint2, 0);
      matptr->Write(CheckPoint2, 1);
      dohrstruct->Write(CheckPoint2);
    }

    // Start from Checkpoint 2
    if (cf2.was_set())
    {
      if (running) {
	cerr << "ERROR: you must select only one of the start modes: mp, mc, cf1, cf2 or cf3" << endl;
	exit(1);
      }
      else  
	running = true;

      TPZFileStream CheckPoint2;
      CheckPoint2.OpenRead(cf2.get_value());
      gmesh = new TPZGeoMesh;
      gmesh->Read(CheckPoint2,0);
      cmeshauto = new TPZCompMesh(gmesh);
      cmeshauto->Read(CheckPoint2, &gmesh);
      matptr = dynamic_cast<TPZMatrix<REAL> *>(TPZSaveable::Restore(CheckPoint2, 0));
      dohrstruct = new TPZDohrStructMatrix(cmeshauto,nt_submeshs.get_value(),nt_decompose.get_value());
      dohrstruct->Read(CheckPoint2);
    }

    TPZAutoPointer<TPZMatrix<REAL> > precond = NULL;
    if (running) {

      TPZAutoPointer<TPZGuiInterface> gui;
      rhs = new TPZFMatrix<REAL>(cmeshauto->NEquations(),1,0.);
      VERBOSE(1,"dohrstruct->Assemble()" << endl);
      dohrstruct->Assemble(*matptr,*rhs, gui);
      precond = dohrstruct->Preconditioner();
    }
      
    if (dc3.was_set() && running)
    {
      VERBOSE(1, "Dumping checkpoint 3 into: " << dc2.get_value() << endl);
      TPZFileStream CheckPoint3;
      CheckPoint3.OpenWrite(dc3.get_value());
      cmeshauto->Reference()->Write(CheckPoint3, 0);
      cmeshauto->Write(CheckPoint3, 0);
      matptr->Write(CheckPoint3, 1);
      precond->Write(CheckPoint3, 1);
      rhs->Write(CheckPoint3, 0);
    }

    // Start from Checkpoint 3
    if (cf3.was_set())
    {
      if (running) {
	cerr << "ERROR: you must select only one of the start modes: mp, mc, cf1, cf2 or cf3" << endl;
	exit(1);
      }
      else  
	running = true;
      gmesh = new TPZGeoMesh;
      cmeshauto = new TPZCompMesh(gmesh);
      dohrstruct = new TPZDohrStructMatrix(cmeshauto,nt_submeshs.get_value(),nt_decompose.get_value());
        
      dim = cmeshauto->Dimension();
      VERBOSE(1, "Reading dim from file. new dim = " << dim << ", old dim = " << dim_arg.get_value() << endl);

      TPZFileStream CheckPoint3;
      CheckPoint3.OpenRead(cf3.get_value());
      gmesh->Read(CheckPoint3, 0);
      cmeshauto->Read(CheckPoint3, gmesh);
      matptr = dynamic_cast<TPZMatrix<REAL> *>(TPZSaveable::Restore(CheckPoint3, 0));
      precond = dynamic_cast<TPZMatrix<REAL> *>(TPZSaveable::Restore(CheckPoint3, matptr));
      rhs = new TPZFMatrix<REAL>(cmeshauto->NEquations(),1,0.);
      rhs->Read(CheckPoint3, 0);
    }

    TPZAutoPointer<TPZMatrix<STATE> > dohr = matptr;

    int neq = dohr->Rows();
    TPZFMatrix<REAL> diag(neq,1,0.), produto(neq,1);
        
    VERBOSE(1, "Number of equations " << neq << endl);
        
    TPZStepSolver<REAL> pre(precond);
    pre.SetMultiply();
    TPZStepSolver<REAL> cg(dohr);
        
    cg.SetCG(500,pre,1.e-8,0);
    cg.Solve(*rhs,diag);

    TPZDohrMatrix<STATE,TPZDohrSubstructCondense<STATE> > *dohrptr = 
      dynamic_cast<TPZDohrMatrix<STATE,TPZDohrSubstructCondense<STATE> > *> (dohr.operator->());

    if (!dohrptr) {
      DebugStop();
    }
         
    dohrptr->AddInternalSolution(diag);
		
    typedef std::list<TPZAutoPointer<TPZDohrSubstructCondense<STATE> > > subtype;
    const subtype &sublist = dohrptr->SubStructures(); 
    subtype::const_iterator it = sublist.begin();
    int subcount=0;
    while (it != sublist.end()) {

      TPZFMatrix<REAL> subext,subu;
      dohrptr->fAssembly->Extract(subcount,diag,subext);
      (*it)->UGlobal(subext,subu);
      TPZCompMesh *submesh = SubMesh(cmeshauto, subcount);
      submesh->LoadSolution(subu);
      
      //ViscoElastico
      //Atualizando memoria do material
      std::map<int ,TPZMaterial * > materialmap(submesh->MaterialVec());
      std::map<int ,TPZMaterial * >::iterator itmat;
      for (itmat = materialmap.begin(); itmat != materialmap.end() ; itmat++) 
	{
	  TPZMaterial * mat = itmat->second;
	  TPZViscoelastic *vmat = dynamic_cast< TPZViscoelastic *> (mat);
	  if(vmat)
	    {
	      vmat->SetUpdateMem();
	    }
	}	
      subcount++;
      it++;
    }
    
#ifdef LOG4CXX
    {
      std::stringstream sout;
      diag.Print("Resultado do processo iterativo",sout);
      LOGPZ_INFO(loggerconverge,sout.str())
    }
#endif	
		
    TPZMaterial * mat = cmeshauto->FindMaterial(1);
    int nstate = mat->NStateVariables();
    int nscal = 0, nvec = 0;
    if(nstate ==1) 
      {
	nscal = 1;
      }
    else
      {
	nvec = 1;
      }
    TPZManVector<std::string> scalnames(nscal),vecnames(nvec);
    if(nscal == 1)
      {
	scalnames[0]="state";            
      }
    else
      {
	vecnames[0] = "state";
      }
    std::string postprocessname("dohrmann_visco.vtk");
    TPZVTKGraphMesh vtkmesh(cmeshauto.operator->(),dim,mat,scalnames,vecnames);
    vtkmesh.SetFileName(postprocessname);
    vtkmesh.SetResolution(1);
    int numcases = 1;
    
    
    // Iteracoes de tempo
    int istep = 0;
    vtkmesh.DrawMesh(numcases);
    vtkmesh.DrawSolution(istep, 1.);

    delete gmesh;

    return EXIT_SUCCESS;
}

void InsertElasticity(TPZAutoPointer<TPZCompMesh> mesh)
{
	mesh->SetDimModel(3);
	int nummat = 1;
	REAL E = 1.e6;
	REAL poisson = 0.3;
	TPZManVector<REAL> force(3,0.);
	force[1] = 20.;
	TPZElasticity3D *elast = new TPZElasticity3D(nummat,E,poisson,force);
	TPZMaterial * elastauto(elast);
	TPZFMatrix<REAL> val1(3,3,0.),val2(3,1,0.);
	TPZBndCond *bc = elast->CreateBC(elastauto, -1, 0, val1, val2);
	TPZMaterial * bcauto(bc);
	mesh->InsertMaterialObject(elastauto);
	mesh->InsertMaterialObject(bcauto);
}

void InsertViscoElasticity(TPZAutoPointer<TPZCompMesh> mesh)
{
	mesh->SetDimModel(3);
	int nummat = 1;
	REAL Ela = 1.e6;
	REAL poisson = 0.2;
	TPZManVector<REAL> force(3,0.);
	force[1] = 20.;
	REAL lambdaV = 0, muV = 0, alphaT = 0;
	lambdaV = 111.3636;
	muV = 455.4545;
	alphaT = 0.1;	
	TPZViscoelastic *viscoelast = new TPZViscoelastic(nummat,Ela,poisson,lambdaV,muV,alphaT,force);
	TPZMaterial * viscoelastauto(viscoelast);
	TPZFMatrix<REAL> val1(3,3,0.),val2(3,1,0.);
	TPZBndCond *bc = viscoelast->CreateBC(viscoelastauto, -1, 0, val1, val2);
	TPZMaterial * bcauto(bc);
	mesh->InsertMaterialObject(viscoelastauto);
	mesh->InsertMaterialObject(bcauto);	
}

void InsertViscoElasticityCubo(TPZAutoPointer<TPZCompMesh> mesh)
{
	mesh->SetDimModel(3);
	int nummat = 1, neumann = 1, mixed = 2;
	//	int dirichlet = 0;
	int dir1 = -1, dir2 = -2, dir3 = -3, neumann1 = -4., neumann2 = -5, dirp2 = -6;
	TPZManVector<REAL> force(3,0.);
	//force[1] = 0.;
	REAL Ela = 1000, poisson = 0.; 
	REAL lambdaV = 0, muV = 0, alphaT = 0;
	lambdaV = 11.3636;
	muV = 45.4545;
	alphaT = 0.01;	
	
	
	//TPZViscoelastic *viscoelast = new TPZViscoelastic(nummat, Ela, poisson, lambdaV, muV, alphaT, force);
	TPZElasticity3D *viscoelast = new TPZElasticity3D(nummat, Ela, poisson, force);
	
	TPZFNMatrix<6> qsi(6,1,0.);
	//viscoelast->SetDefaultMem(qsi); //elast
	//int index = viscoelast->PushMemItem(); //elast
	TPZMaterial * viscoelastauto(viscoelast);
	mesh->InsertMaterialObject(viscoelastauto);
	
	// Neumann em x = 1;
	TPZFMatrix<> val1(3,3,0.),val2(3,1,0.);
	val2(0,0) = 1.;
	TPZBndCond *bc4 = viscoelast->CreateBC(viscoelastauto, neumann1, neumann, val1, val2);
	TPZMaterial * bcauto4(bc4);
	mesh->InsertMaterialObject(bcauto4);
	
	// Neumann em x = -1;
	val2(0,0) = -1.;
	TPZBndCond *bc5 = viscoelast->CreateBC(viscoelastauto, neumann2, neumann, val1, val2);
	TPZMaterial * bcauto5(bc5);
	mesh->InsertMaterialObject(bcauto5);
	
	val2.Zero();
	// Dirichlet em -1 -1 -1 xyz;
	val1(0,0) = 1e4;
	val1(1,1) = 1e4;
	val1(2,2) = 1e4;
	TPZBndCond *bc1 = viscoelast->CreateBC(viscoelastauto, dir1, mixed, val1, val2);
	TPZMaterial * bcauto1(bc1);
	mesh->InsertMaterialObject(bcauto1);
	
	// Dirichlet em 1 -1 -1 yz;
	val1(0,0) = 0.;
	val1(1,1) = 1e4;
	val1(2,2) = 1e4;
	TPZBndCond *bc2 = viscoelast->CreateBC(viscoelastauto, dir2, mixed, val1, val2);
	TPZMaterial * bcauto2(bc2);
	mesh->InsertMaterialObject(bcauto2);
	
	// Dirichlet em 1 1 -1 z;
	val1(0,0) = 0.;
	val1(1,1) = 0.;
	val1(2,2) = 1e4;
	TPZBndCond *bc3 = viscoelast->CreateBC(viscoelastauto, dir3, mixed, val1, val2);
	TPZMaterial * bcauto3(bc3);
	mesh->InsertMaterialObject(bcauto3);
	
}

TPZGeoMesh *MalhaPredio()
{
	//int nBCs = 1;
	int numnodes=-1;
	int numelements=-1;
	
	string FileName = mp.get_value();	
	{
		bool countnodes = false;
		bool countelements = false;
		
		ifstream read (FileName.c_str());
		if (!read.is_open()) {
		  cerr << "Could not open file: " << FileName << endl;
		  exit(1);
		}

		while(read)
		{
			char buf[1024];
			read.getline(buf, 1024);
			std::string str(buf);
			if(str == "Coordinates") countnodes = true;
			if(str == "end coordinates") countnodes = false;
			if(countnodes) numnodes++;
			
			if(str == "Elements") countelements = true;
			if(str == "end elements") countelements = false;
			if(countelements) numelements++;
		}
	}
	
	TPZGeoMesh * gMesh = new TPZGeoMesh;
	
	gMesh -> NodeVec().Resize(numnodes);
	
	TPZVec <int> TopolTetra(4);
	
	const int Qnodes = numnodes;
	TPZVec <TPZGeoNode> Node(Qnodes);
	
	//setting nodes coords
	int nodeId = 0, elementId = 0, matElId = 1;
	
	ifstream read;
	read.open(FileName.c_str());
	
	double nodecoordX , nodecoordY , nodecoordZ ;
	
	char buf[1024];
	read.getline(buf, 1024);
	read.getline(buf, 1024);
	std::string str(buf);
	int in;
	for(in=0; in<numnodes; in++)
	{ 
		read >> nodeId;
		read >> nodecoordX;
		read >> nodecoordY;
		read >> nodecoordZ;
		Node[nodeId-1].SetNodeId(nodeId);
		Node[nodeId-1].SetCoord(0,nodecoordX);
		Node[nodeId-1].SetCoord(1,nodecoordY);
		Node[nodeId-1].SetCoord(2,nodecoordZ);
		gMesh->NodeVec()[nodeId-1] = Node[nodeId-1];
	}
	
	{
		read.close();
		read.open(FileName.c_str());

		int l , m = numnodes+5;
		for(l=0; l<m; l++)
		{
			read.getline(buf, 1024);
		}
		
		int el;
		int matBCid = -1;
		//std::set<int> ncoordz; //jeitoCaju
		for(el=0; el<numelements; el++)
		{
			read >> elementId;
			read >> TopolTetra[0]; //node 1
			read >> TopolTetra[1]; //node 2
			read >> TopolTetra[2]; //node 3
			read >> TopolTetra[3]; //node 4
			
			// O GID comeca com 1 na contagem dos nodes, e nao zero como no PZ, assim o node 1 na verdade é o node 0
			TopolTetra[0]--;
			TopolTetra[1]--;
			TopolTetra[2]--;
			TopolTetra[3]--;
			
			int index = el;
			
			TPZGeoEl * tetra = new TPZGeoElRefPattern< pzgeom::TPZGeoTetrahedra> (index, TopolTetra, matElId, *gMesh);
		}
		
		gMesh->BuildConnectivity();
		// Colocando as condicoes de contorno
		
		for(el=0; el<numelements; el++)
		{
			TPZManVector <TPZGeoNode,4> Nodefinder(4);
			TPZManVector <REAL,3> nodecoord(3);
			TPZGeoEl *tetra = gMesh->ElementVec()[el];
			// na face z = 0
			TPZVec<int> ncoordzVec(0); int sizeOfVec = 0;
			for (int i = 0; i < 4; i++) 
			{
				int pos = tetra->NodeIndex(i);
				Nodefinder[i] = gMesh->NodeVec()[pos];
				Nodefinder[i].GetCoordinates(nodecoord);
				if (nodecoord[2] == 0.)
				{
					sizeOfVec++;
					ncoordzVec.Resize(sizeOfVec);
					ncoordzVec[sizeOfVec-1] = pos;
				}
			}
			if(ncoordzVec.NElements() == 3)
			{
				int lado = tetra->WhichSide(ncoordzVec);
				TPZGeoElSide tetraSide(tetra, lado);
				TPZGeoElBC(tetraSide,matBCid);	
			}
		}
	}
	
	ofstream arg("malhaPZ.txt");
	gMesh->Print(arg);
	ofstream predio("GeoPredio.vtk");
	TPZVTKGeoMesh::PrintGMeshVTK(gMesh,predio,true); 
	
	return gMesh;
}

TPZGeoMesh *MalhaCubo()
{
	int numnodes=-1;
	int numelements=-1;
	
	string FileName = mc.get_value();
	{
		bool countnodes = false;
		bool countelements = false;
		
		ifstream read (FileName.c_str());
		if (!read.is_open()) {
		  cerr << "Could not open file: " << FileName << endl;
		  exit(1);
		}
		
		while(read)
		{
			char buf[1024];
			read.getline(buf, 1024);
			std::string str(buf);
			if(str == "Coordinates") countnodes = true;
			if(str == "end coordinates") countnodes = false;
			if(countnodes) numnodes++;
			
			if(str == "Elements") countelements = true;
			if(str == "end elements") countelements = false;
			if(countelements) numelements++;
		}
	}
	
	TPZGeoMesh * gMesh = new TPZGeoMesh;
	
	gMesh -> NodeVec().Resize(numnodes);
	
	TPZManVector <int> TopolTetra(4);
	
	const int Qnodes = numnodes;
	TPZVec <TPZGeoNode> Node(Qnodes);
	
	//setting nodes coords
	int nodeId = 0, elementId = 0, matElId = 1;
	
	ifstream read;
	read.open(FileName.c_str());
	
	double nodecoordX , nodecoordY , nodecoordZ ;
	
	char buf[1024];
	read.getline(buf, 1024);
	read.getline(buf, 1024);
	std::string str(buf);
	int in;
	for(in=0; in<numnodes; in++)
	{ 
		read >> nodeId;
		read >> nodecoordX;
		read >> nodecoordY;
		read >> nodecoordZ;
		Node[nodeId-1].SetNodeId(nodeId);
		Node[nodeId-1].SetCoord(0,nodecoordX);
		Node[nodeId-1].SetCoord(1,nodecoordY);
		Node[nodeId-1].SetCoord(2,nodecoordZ);
		gMesh->NodeVec()[nodeId-1] = Node[nodeId-1];
	}
	
	{
		read.close();
		read.open(FileName.c_str());
		
		int l , m = numnodes+5;
		for(l=0; l<m; l++)
		{
			read.getline(buf, 1024);
		}
		
		
		int el;
		int neumann1 = -4, neumann2 = -5;
		//std::set<int> ncoordz; //jeitoCaju
		for(el=0; el<numelements; el++)
		{
			read >> elementId;
			read >> TopolTetra[0]; //node 1
			read >> TopolTetra[1]; //node 2
			read >> TopolTetra[2]; //node 3
			read >> TopolTetra[3]; //node 4
			
			// O GID comeca com 1 na contagem dos nodes, e nao zero como no PZ, assim o node 1 na verdade é o node 0
			TopolTetra[0]--;
			TopolTetra[1]--;
			TopolTetra[2]--;
			TopolTetra[3]--;
			
			int index = el;
			
			TPZGeoEl * tetra = new TPZGeoElRefPattern< pzgeom::TPZGeoTetrahedra> (index, TopolTetra, matElId, *gMesh);
		}
		
		gMesh->BuildConnectivity();
		
		// Colocando as condicoes de contorno
		for(el=0; el<numelements; el++)
		{
			TPZManVector <TPZGeoNode,4> Nodefinder(4);
			TPZManVector <REAL,3> nodecoord(3);
			TPZGeoEl *tetra = gMesh->ElementVec()[el];
			
			// na face x = 1
			TPZVec<int> ncoordzVec(0); int sizeOfVec = 0;
			for (int i = 0; i < 4; i++) 
			{
				int pos = tetra->NodeIndex(i);
				Nodefinder[i] = gMesh->NodeVec()[pos];
				Nodefinder[i].GetCoordinates(nodecoord);
				if (nodecoord[0] == 1.)
				{
					sizeOfVec++;
					ncoordzVec.Resize(sizeOfVec);
					ncoordzVec[sizeOfVec-1] = pos;
				}
			}
			if(ncoordzVec.NElements() == 3)
			{
				int lado = tetra->WhichSide(ncoordzVec);
				TPZGeoElSide tetraSide(tetra, lado);
				TPZGeoElBC(tetraSide,neumann1);	
			}
			
			// Na face x = -1
			ncoordzVec.Resize(0);
			sizeOfVec = 0;
			for (int i = 0; i < 4; i++) 
			{
				int pos = tetra->NodeIndex(i);
				Nodefinder[i] = gMesh->NodeVec()[pos];
				
				Nodefinder[i].GetCoordinates(nodecoord);
				if (nodecoord[0] == -1.)
				{
					sizeOfVec++;
					ncoordzVec.Resize(sizeOfVec);
					ncoordzVec[sizeOfVec-1] = pos;
				}
			}
			if(ncoordzVec.NElements() == 3)
			{
				int lado = tetra->WhichSide(ncoordzVec);
				TPZGeoElSide tetraSide(tetra, lado);
				TPZGeoElBC(tetraSide,neumann2);	
			}
			
		}
		
		TPZVec <REAL> xyz(3,-1.), yz(3,-1.), z(3,1.);
		yz[0] = 1.;
		z[2] = -1;
		int bcidxyz = -1, bcidyz = -2, bcidz = -3;
		SetPointBC(gMesh, xyz, bcidxyz);
		SetPointBC(gMesh, yz, bcidyz);
		SetPointBC(gMesh, z, bcidz);
		
	}
	
	ofstream arg("malhaPZ1BC.txt");
	gMesh->Print(arg);
	
	std::ofstream out("Cube.vtk");
	TPZVTKGeoMesh::PrintGMeshVTK(gMesh, out, true);
	
	return gMesh;
}

/// Generate a boundary geometric element at the indicated node
void SetPointBC(TPZGeoMesh *gr, TPZVec<REAL> &x, int bc)
{
	// look for an element/corner node whose distance is close to start
	TPZGeoNode *gn1 = gr->FindNode(x);
	int iel;
	int nelem = gr->ElementVec().NElements();
	TPZGeoEl *gel;
	for (iel = 0; iel<nelem; iel++) {
		gel = gr->ElementVec()[iel];
		if(!gel) continue;
		int nc = gel->NCornerNodes();
		int c;
		for (c=0; c<nc; c++) {
			TPZGeoNode *gn = gel->NodePtr(c);
			if (gn == gn1) {
				break;
			}
		}
		if (c<nc) {
			TPZGeoElBC(gel, c, bc);
			return;
		}
	}
}

REAL Height(TPZGeoMesh *gmesh)
{
	TPZAdmChunkVector<TPZGeoNode> &nodevec = gmesh->NodeVec();
	int nnodes = nodevec.NElements();
	int in;
	REAL maxz = 0.;
	for (in=0; in<nnodes; in++) {
		REAL z = nodevec[in].Coord(2);
		maxz = (maxz < z) ? z : maxz;
	}
	return maxz;
}

int SubStructure(TPZAutoPointer<TPZCompMesh> cmesh, REAL height)
{
	int nelem = cmesh->NElements();
	TPZManVector<int> subindex(nelem,-1);
	int iel;
	int nsub = 0;
	for (iel=0; iel<nelem; iel++) 
	{
		TPZCompEl *cel = cmesh->ElementVec()[iel];
		if (!cel) {
			continue;
		}
		TPZGeoEl *gel = cel->Reference();
		if (!gel) {
			continue;
		}
		int nsides = gel->NSides();
		TPZManVector<REAL> center(gel->Dimension(),0.), xco(3,0.);
		gel->CenterPoint(nsides-1,center);
		gel->X(center,xco);
		REAL z = xco[2];
		int floor = (int) z/height;
		nsub = (floor+1) > nsub ? (floor+1) : nsub;
		subindex[iel] = floor;
	}
	
#ifdef DEBUG 
	{
		TPZGeoMesh *gmesh = cmesh->Reference();
		int nelgeo = gmesh->NElements();
		TPZVec<int> domaincolor(nelgeo,-999);
		int cel;
		int nel = cmesh->NElements();
		for (cel=0; cel<nel; cel++) {
			TPZCompEl *compel = cmesh->ElementVec()[cel];
			if(!compel) continue;
			TPZGeoEl *gel = compel->Reference();
			if (!gel) {
				continue;
			}
			domaincolor[gel->Index()] = subindex[cel];
		}
		ofstream vtkfile("partition.vtk");
		TPZVTKGeoMesh::PrintGMeshVTK(gmesh, vtkfile, domaincolor);
	}
#endif
	
	int isub;
	TPZManVector<TPZSubCompMesh *> submeshes(nsub,0);
	for (isub=0; isub<nsub; isub++) 
	{
		int index;
		std::cout << '^'; std::cout.flush();
		submeshes[isub] = new TPZSubCompMesh(cmesh,index);
		
		if (index < subindex.NElements()) 
		{
			subindex[index] = -1;
		}
	}
	for (iel=0; iel<nelem; iel++) 
	{
		int domindex = subindex[iel];
		if (domindex >= 0) 
		{
			TPZCompEl *cel = cmesh->ElementVec()[iel];
			if (!cel) 
			{
				continue;
			}
			submeshes[domindex]->TransferElement(cmesh.operator->(),iel);
		}
	}
	cmesh->ComputeNodElCon();
	for (isub=0; isub<nsub; isub++) 
	{
		submeshes[isub]->MakeAllInternal();
		std::cout << '*'; std::cout.flush();
	}
	
	cmesh->ComputeNodElCon();
	cmesh->CleanUpUnconnectedNodes();
	return nsub;
}