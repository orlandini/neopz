//
//  main.cpp
//  PZ
//
//  Created by Agnaldo Farias on 7/20/12.
//  Copyright (c) 2012 LabMec-Unicamp. All rights reserved.
//

#include <iostream>
using namespace std;

#include "pzgmesh.h"
#include "pzcmesh.h"
#include "pzcompel.h"
#include "pzbndcond.h"
#include "TPZInterfaceEl.h"
#include "pzbuildmultiphysicsmesh.h"
#include "pzinterpolationspace.h"

#include "pzgeoelbc.h"
#include "TPZCompElDisc.h"
#include "pzpoisson3d.h"
#include "pzelasmat.h"
#include "pzporoelasticmf2d.h"

#include "tpzgeoelrefpattern.h"
#include "TPZGeoLinear.h"
#include "tpztriangle.h"
#include "pzgeoquad.h"

#include "pzanalysis.h"
#include "pzskylstrmatrix.h"
#include "pzstrmatrix.h"
#include "pzstepsolver.h"
#include "TPZSpStructMatrix.h"
#include "pzfstrmatrix.h"

#include "mymeshes.h"
#include "pzfunction.h"

#include "pzlog.h"

#include "TPZVTKGeoMesh.h"

#include <iostream>
#include <math.h>
using namespace std;


REAL const Pi = 4.*atan(1.);
REAL ftimeatual = 0.;
REAL fdeltaT = 0.;
REAL felarea = 0.;
bool fdimensionless;


void SolucaoExata1D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);
void SolucaoPQMurad(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);//para calcular o erro
void SolucaoUMurad(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &deriv);//para calcular o erro

void TerzaghiProblem1D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);
void SolucaoPQTerzaghi(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);

void BarryMercerProblem2D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);
void SolucaoPQBarryMercer(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux);
void SolucaoUBarryMercer(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &deriv);

void RefinamentoPadrao3x3(TPZGeoMesh *gmesh, int nref);
void RefinamentoPadrao3x3(TPZGeoMesh *gmesh, int nref,TPZVec<REAL> pt, bool changeMatId, int newmatId, REAL &Area);

void ForcingSource(const TPZVec<REAL> &pt, TPZVec<STATE> &disp);

#ifdef LOG4CXX
static LoggerPtr logdata(Logger::getLogger("pz.porolasticmf2d.data"));
#endif

//problema murad e Loula
int main_Loula(int argc, char *argv[])
{
    
#ifdef LOG4CXX
	std::string logs("../logporoelastc2d.cfg");
	InitializePZLOG("../logporoelastc2d.cfg");
#endif
    
    bool triang=true;
    bool dimensionless = true;
    
    REAL Eyoung = 3.e4;
    REAL poisson = 0.2;
    
    REAL rockrho = 0.;
    REAL gravity = 0.;
    REAL fx=0.0;
    REAL fy = gravity*rockrho;
    
    REAL alpha = 1.0;
    REAL Se = 0.0;
    REAL perm = 1.e-10;
    REAL visc = 1.e-3;
    REAL sig0 = -1000.;
    REAL pini = 1000.;
    REAL Ly = 1.;
    REAL Lx = 1.;
    REAL timeT = 10.;
    
    
    //Incompressible fluid (Se=0)
    if(dimensionless==true)
    {
        REAL lambda = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));//firstlame
        REAL mu = 0.5*Eyoung/(1+poisson);//secondlame
        REAL pref = pini;
        REAL Lref = Ly;
        REAL Se_aux = alpha*alpha/(lambda+2.*mu);
        REAL kovervisc = perm/visc;
        REAL Cf = 0.;
        Cf = kovervisc/Se_aux;//fluid diffusivity coeffcient
        REAL lambdaD = lambda*Se_aux;
        REAL muD = mu*Se_aux;

        //adimemsionalizando
        Eyoung = muD*(3.*lambdaD+2.*muD)/(lambdaD+muD);
        poisson = 0.5*lambdaD/(lambdaD+muD);
        sig0 = sig0/pref;
        pini =pini/pref;
        timeT = 1.;//timeT*Cf/(Ly*Ly);
        Lx = Lx/Lref;
        Ly = Ly/Lref;
        perm = 1.;
        visc = 1.;
        fx = fx*Lref/pref;
        fy = fy*Lref/pref;
    }
    
    DadosMalhas * mydata = new DadosMalhas();
    mydata->SetParameters(Eyoung, poisson, alpha, Se, perm, visc, fx, fy, sig0);
    
    ofstream saidaerro("Erro.txt");
    
    
    for(int p = 3; p < 4; p++)
    {
        int pu = p;
        int pq = pu;
        int pp;
        if(triang==true){
            pp = pq-1;
        }else{
            pq=pu-1;
            pp = pq;
        }
        
        int h;
        saidaerro<<"\n CALCULO DO ERRO, ELEM. TRIANG., COM ORDEM POLINOMIAL pu = "<< pu << ", pq = "<< pq << " e pp = "<< pp<<endl;
        for (h = 4; h< 5; h++)
        {
        
        saidaerro<<"\n========= PARA h = "<< h<<"  ============= "<<endl;
    
        //primeira malha
        // geometric mesh (initial)
        TPZGeoMesh * gmesh = mydata->GMesh(triang,Lx,Ly);
        //mydata->RefiningNearLine(2, gmesh, 4);
        //TPZGeoMesh * gmesh = mydata->GMesh2(LxD,LyD);
        //TPZGeoMesh * gmesh = mydata->GMesh3(LxD,LyD);
//        ofstream arg1("gmesh_inicial.txt");
//        gmesh->Print(arg1);

        // mydata->UniformRefine(gmesh, 2);


        // First computational mesh
        TPZCompMesh * cmesh1 = mydata->MalhaCompElast(gmesh,pu,false);
//        ofstream arg2("cmesh1_inicial.txt");
//        cmesh1->Print(arg2);

        // second computational mesh
        TPZCompMesh * cmesh2= mydata->CMeshFlux(gmesh, pq,false);
//        ofstream arg3("cmesh2_inicial.txt");
//        cmesh2->Print(arg3);


        // Third computational mesh
        TPZCompMesh * cmesh3 = mydata->CMeshPressure(gmesh, pp,triang,false);
//        ofstream arg4("cmesh3_inicial.txt");
//        cmesh3->Print(arg4);


        // Cleaning reference of the geometric mesh to cmesh1
        gmesh->ResetReference();
        cmesh1->LoadReferences();
        TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh1,h,false);
        cmesh1->AdjustBoundaryElements();
        cmesh1->CleanUpUnconnectedNodes();
//        ofstream arg5("cmesh1_final.txt");
//        cmesh1->Print(arg5);


        // Cleaning reference to cmesh2
        gmesh->ResetReference();
        cmesh2->LoadReferences();
        TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh2,h,false);
        cmesh2->AdjustBoundaryElements();
        cmesh2->CleanUpUnconnectedNodes();
//        ofstream arg6("cmesh2_final.txt");
//        cmesh2->Print(arg6);

        // Cleaning reference to cmesh3
        gmesh->ResetReference();
        cmesh3->LoadReferences();
        TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh3,h,true);

        cmesh3->AdjustBoundaryElements();
        cmesh3->CleanUpUnconnectedNodes();
//        ofstream arg7("cmesh3_final.txt");
//        cmesh3->Print(arg7);


        //	Set initial conditions for pressure

        TPZAnalysis an3(cmesh3);
        mydata->SolveSist(an3, cmesh3);
        int nrs = an3.Solution().Rows();
        TPZVec<STATE> solini(nrs,pini);
        //	cmesh3->Solution() = solucao1;
        //    cmesh3->LoadSolution(solucao1);

        TPZCompMesh  * cmeshL2 = mydata->CMeshPressureL2(gmesh, pp, solini,triang);
        TPZAnalysis anL2(cmeshL2);
        mydata->SolveSist(anL2, cmeshL2);
        //anL2.Solution().Print("sol");
        
        an3.LoadSolution(anL2.Solution());
        //    an3.Solution().Print();

        //malha multifisica
        TPZVec<TPZCompMesh *> meshvec(3);
        meshvec[0] = cmesh1;
        meshvec[1] = cmesh2;
        meshvec[2] = cmesh3;
        TPZPoroElasticMF2d * mymaterial;
        TPZAutoPointer<TPZFunction<STATE> > solExata = new TPZDummyFunction<STATE>(SolucaoExata1D);
        TPZCompMesh * mphysics = mydata->MalhaCompMultphysics(gmesh,meshvec,mymaterial,solExata);
//        ofstream arg8("mphysic.txt");
//        mphysics->Print(arg8);

        int NDeltaT = 10000;
        int intervsaidas = NDeltaT/20;
        REAL deltaT=timeT/NDeltaT; //second
        mymaterial->SetTimeStep(deltaT);
        REAL maxTime = timeT;

       // mydata->SolveSistTransient(deltaT, maxTime, mymaterial, meshvec, mphysics,1,ftimeatual);


        //Saida dos erros
//        TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
//        TPZVec<REAL> erros;
//
//        saidaerro<<" Erro da simulacao multifisica do deslocamento (u)" <<endl;
//        TPZAnalysis an12(cmesh1);
//        an12.SetExact(*SolucaoUMurad);
//        an12.PostProcessError(erros, saidaerro);
//
//
//        saidaerro<<" \nErro da simulacao multifisica do fluxo (q)" <<endl;
//        TPZAnalysis an22(cmesh2);
//        an22.SetExact(*SolucaoPQMurad);
//        an22.PostProcessError(erros, saidaerro);
//
//        saidaerro<<" Erro da simulacao multifisica da pressao (p)" <<endl;
//        TPZAnalysis an32(cmesh3);
//        an32.SetExact(*SolucaoUMurad);
//        an32.PostProcessError(erros, saidaerro);

        
        TPZAnalysis an(mphysics);
        TPZFMatrix<STATE> Initialsolution = an.Solution();
        //Initialsolution.Print("solini");
        
        std::string outputfile;
        outputfile = "TransientSolution";
        
//        std::stringstream outputfiletemp;
//        outputfiletemp << outputfile << ".vtk";
//        std::string plotfile = outputfiletemp.str();
//        mydata->PosProcessMultphysics(meshvec,mphysics,an,plotfile);
        
        //Criando matriz de massa (matM)
        TPZAutoPointer <TPZMatrix<STATE> > matM = mydata->MassMatrix(mymaterial, mphysics);
        
        //Criando matriz de rigidez (matK) e vetor de carga
        TPZFMatrix<STATE> matK;
        TPZFMatrix<STATE> fvec;
        mydata->StiffMatrixLoadVec(mymaterial, mphysics, an, matK, fvec);
        
        int nrows;
        nrows = matM->Rows();
        TPZFMatrix<STATE> TotalRhs(nrows,1,0.0);
        TPZFMatrix<STATE> TotalRhstemp(nrows,1,0.0);
        TPZFMatrix<STATE> Lastsolution = Initialsolution;
        
        REAL TimeValue = 0.0;
        int cent = 1;
        TimeValue = cent*deltaT;
        while (TimeValue <= maxTime)
        {
            ftimeatual  = TimeValue;
            // This time solution i for Transient Analytic Solution
            mymaterial->SetTimeValue(TimeValue);
            matM->Multiply(Lastsolution,TotalRhstemp);
            
            TotalRhs = fvec + TotalRhstemp;
            an.Rhs() = TotalRhs;
            an.Solve();
            Lastsolution = an.Solution();
            
            if(cent%intervsaidas==0){
                saidaerro<<"\n========= PARA O PASSO n = "<< cent <<"  E TEMPO tn = "<< TimeValue <<" =========\n"<<endl;
                std::stringstream outputfiletemp;
                outputfiletemp << outputfile << ".vtk";
                std::string plotfile = outputfiletemp.str();
                mydata->PosProcessMultphysics(meshvec,mphysics,an,plotfile);
            
            
                TPZVec<REAL> erros;
                
                saidaerro<<" Erro da simulacao multifisica do deslocamento (u)" <<endl;
                TPZAnalysis an12(cmesh1);
                an12.SetExact(*SolucaoUMurad);
                an12.PostProcessError(erros, saidaerro);
                
                
                saidaerro<<" \nErro da simulacao multifisica do fluxo (q)" <<endl;
                TPZAnalysis an22(cmesh2);
                an22.SetExact(*SolucaoPQMurad);
                an22.PostProcessError(erros, saidaerro);
                
                saidaerro<<" Erro da simulacao multifisica da pressao (p)" <<endl;
                TPZAnalysis an32(cmesh3);
                an32.SetExact(*SolucaoPQMurad);
                an32.PostProcessError(erros, saidaerro);
            }
            
            
            cent++;
            TimeValue = cent*deltaT;
        }

        cmesh1->CleanUp();
        cmesh2->CleanUp();
        cmesh3->CleanUp();
        //mphysics->CleanUp();
        delete cmesh1;
        delete cmesh2;
        delete cmesh3;
        //delete mphysics;
        delete gmesh;
    }
    }
    return 0;
}

void SolucaoExata1D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux){
    
    bool sol_dimensionless=true;
    
	//REAL x = ptx[0];
	REAL x = ptx[1];
	
	REAL pini = 1000.;
	REAL lamb = 8333.33;
	REAL mi = 12500.0;
	REAL visc =0.001;
	REAL perm =  1.e-10;
	REAL H=1.;
	REAL tp = ftimeatual;
	int in;
	REAL pD = 0.0, uD = 0.0, sigD = 0.0, VDy=0.0;
    REAL sumpD = 0.0, sumuD = 0.0, sumsigD=0.0, sumVDy=0.0;
    
    REAL M =0.;
	REAL PI = atan(1.)*4.;
	
    sol.Resize(5, 0.);//p, ux, uy, sigx, sigy
    sol[0]=sol[1]=sol[2]=sol[3]=sol[4]=0.;
    flux.Resize(2, 1);
    flux(0,0)=0.;
    flux(1,0)=0.;
	
	REAL tD = tp;//(lamb+2.*mi)*perm*tp/(visc*H*H);
	REAL xD = fabs(1.-x)/H;
	for (in =0; in<1000; in++) {
		
		M = PI*(2.*in+1.)/2.;
		sumpD += (2./M)*sin(M*xD)*exp(-1.*M*M*tD);
		sumuD += (2./(M*M))*cos(M*xD)*exp(-1.*M*M*tD);
		sumsigD += (2./M)*sin(M*xD)*exp(-1.*M*M*tD);
        
        sumVDy+= 2.*cos(M*xD)*exp(-1.*M*M*tD);
	}
	
    pD = sumpD;
    uD = (H/H - xD) - sumuD;
    sigD = -1. + sumsigD;
    VDy = sumVDy;
    
    if(sol_dimensionless==true){
        sol[0] = pD;
        sol[2] = (-1.)*uD;
        sol[4] = sigD;
        
        flux(1,0) = (1.)*VDy;
    }else{
    
        sol[0] = pD*pini;
        sol[2] = (-1.)*uD*(pini*H)/(lamb+2.*mi);
        sol[4] = (sigD)*pini;
        flux(1,0) = (1.)*VDy*pini*(perm/visc);
    }
}


void SolucaoUMurad(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &deriv){
    
    bool sol_dimensionless=true;
    
	//REAL x = ptx[0];
	REAL x = ptx[1];
	
	REAL pini = 1000.;
	REAL lamb = 8333.33;
	REAL mi = 12500.0;
	REAL H=1.;
	REAL tp = ftimeatual;
	int in;
	REAL uD = 0.0, sigD=0.;
    REAL sumuD = 0.0, sumsigD = 0.;
    
    REAL M =0.;
	REAL PI = atan(1.)*4.;
	
    sol.Resize(2, 0.);// ux, uy;
    deriv.Resize(2,2);//sigx, sigxy, sigyx, sigy
    deriv(0,0) = deriv(0,1) = deriv(1,0) = deriv(1,1) = 0.;

	
	REAL tD = tp;//(lamb+2.*mi)*perm*tp/(visc*H*H);
	REAL xD = fabs(1.-x)/H;
	for (in =0; in<1000; in++) {
		
		M = PI*(2.*in+1.)/2.;
		sumuD += (2./(M*M))*cos(M*xD)*exp(-1.*M*M*tD);
		sumsigD += (2./M)*sin(M*xD)*exp(-1.*M*M*tD);
	}
	
    uD = (H/H - xD) - sumuD;
    sigD = -1. + sumsigD;
    
    if(sol_dimensionless==true){
        sol[1] = (-1.)*uD;
        deriv(1,1) = sigD;
    }else{
        sol[1] = (-1.)*uD*(pini*H)/(lamb+2.*mi);
        deriv(1,1) = (sigD)*pini;
    }
}


void SolucaoPQMurad(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux){
    
    bool sol_dimensionless=true;
    
	//REAL x = ptx[0];
	REAL x = ptx[1];
	
	REAL pini = 1000.;
	REAL visc =0.001;
	REAL perm =  1.e-10;
	REAL H=1.;
	REAL tp = ftimeatual;
	int in;
	REAL pD = 0.0, VDy=0.0;
    REAL sumpD = 0.0, sumVDy=0.0;
    
    REAL M =0.;
	REAL PI = atan(1.)*4.;
	
    sol.Resize(1, 0.);//p, ux, uy, sigx, sigy
    flux.Redim(3, 1);
    flux(0,0)=flux(1,0)=flux(2,0)=0.;
	
	REAL tD = tp;//(lamb+2.*mi)*perm*tp/(visc*H*H);
	REAL xD = fabs(1.-x)/H;
	for (in =0; in<1000; in++) {
		
		M = PI*(2.*in+1.)/2.;
		sumpD += (2./M)*sin(M*xD)*exp(-1.*M*M*tD);
        sumVDy+= 2.*cos(M*xD)*exp(-1.*M*M*tD);
	}
	
    pD = sumpD;
    VDy = sumVDy;
    
    if(sol_dimensionless==true){
        sol[0] = pD;
        
        flux(1,0) = (1.)*VDy;
    }else{
        
        sol[0] = pD*pini;        
        flux(1,0) = (1.)*VDy*pini*(perm/visc);
    }
}



//problema de Terzaghi com Se!=0
int main_Terzaghi(int argc, char *argv[]){
#ifdef LOG4CXX
	std::string logs("../logporoelastc2d.cfg");
	InitializePZLOG("../logporoelastc2d.cfg");
#endif
    
    bool triang = false;
   fdimensionless = true;
    
    REAL Eyoung = 1.e5;
    REAL poisson = 0.2;
    REAL firstlame = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
    REAL secondlame = 0.5*Eyoung/(1.+poisson);
    REAL alpha = 1.0;
    REAL Se = 1.e-1;
    
    REAL K = firstlame + (2./3.)*secondlame;
    REAL Ku = 0.;
    Ku = K + alpha*alpha/Se;
    REAL sig0 = -1000.;
    REAL F  = abs(sig0);
    REAL pini = 0.;//(alpha*F)/(Se*(Ku+(4./3.)*secondlame));
    
    REAL rockrho = 0.;
    REAL gravity = 0.;
    REAL fx=0.0;
    REAL fy = gravity*rockrho;
    REAL perm = 1.e-10;
    REAL visc = 1.e-4;
   
    REAL Ly = 1.;
    REAL Lx = 1.;
    REAL timeT = 1000;
    
    
    //Incompressible fluid (Se=0)
    if(fdimensionless==true)
    {
        REAL lambda = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));//firstlame
        REAL mu = 0.5*Eyoung/(1+poisson);//secondlame
        REAL lambdau = alpha*alpha/Se +lambda; //undrained;
        REAL Se_aux = alpha*alpha*(lambdau+2.*mu)/((lambdau - lambda)*(lambda+2.*mu));
        REAL lambdaD = lambda*Se_aux;
        REAL muD = mu*Se_aux;
        
        REAL pref = (alpha*F)/(Se*(lambdau + 2.*mu));
        REAL Lref = Ly;
        REAL kovervisc = perm/visc;
        REAL Cf = 0.;
        Cf = kovervisc/Se_aux;//fluid diffusivity coeffcient
        
        //adimemsionalizando
        Eyoung = muD*(3.*lambdaD+2.*muD)/(lambdaD+muD);
        poisson = 0.5*lambdaD/(lambdaD+muD);
        sig0 = sig0/pref;
        pini =pini/pref;
        timeT = 0.1;// timeT*Cf/(Ly*Ly);
        Lx = Lx/Lref;
        Ly = Ly/Lref;
        perm = 1.;
        visc = 1.;
        fx = fx*Lref/pref;
        fy = fy*Lref/pref;
        Se = Se/Se_aux;
    }
    
    DadosMalhas * mydata = new DadosMalhas();
    mydata->SetParameters(Eyoung, poisson, alpha, Se, perm, visc, fx, fy, sig0);
    
    
    ofstream saidaerro("Erro.txt");
    
    
    for(int p =2; p<3; p++)
    {
        int pu = p;
        int pq = pu;
        int pp;
        if(triang==true){
            pp = pq-1;
        }else{
            pq=pu-1;
            pp = pq;
        }
        
        int h;
        saidaerro<<"\n CALCULO DO ERRO, ELEM. QUAD., COM ORDEM POLINOMIAL pu = "<< pu << ", pq = "<< pq << " e pp = "<< pp<<endl;
        for (h = 0; h< 7; h++)
        {
            
            saidaerro<<"\n========= PARA h = "<< h<<"  ============= "<<endl;
            // geometric mesh (initial)
            TPZGeoMesh * gmesh = mydata->GMesh(triang,Lx,Ly);
            //mydata->RefiningNearLine(2, gmesh, 4);


            // First computational mesh
            TPZCompMesh * cmesh1 = mydata->MalhaCompElast(gmesh,pu,false);

            // second computational mesh
            TPZCompMesh * cmesh2= mydata->CMeshFlux(gmesh, pq, false);

            // Third computational mesh
            TPZCompMesh * cmesh3=mydata->CMeshPressure(gmesh, pp,triang, false);

            // Cleaning reference of the geometric mesh to cmesh1
            gmesh->ResetReference();
            cmesh1->LoadReferences();
            TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh1,h,false);
            cmesh1->AdjustBoundaryElements();
            cmesh1->CleanUpUnconnectedNodes();


            // Cleaning reference to cmesh2
            gmesh->ResetReference();
            cmesh2->LoadReferences();
            TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh2,h,false);
            cmesh2->AdjustBoundaryElements();
            cmesh2->CleanUpUnconnectedNodes();

            // Cleaning reference to cmesh3
            gmesh->ResetReference();
            cmesh3->LoadReferences();
            TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh3,h,true);

            cmesh3->AdjustBoundaryElements();
            cmesh3->CleanUpUnconnectedNodes();


            //	Set initial conditions for pressure
            TPZAnalysis an3(cmesh3);
            int nrs = an3.Solution().Rows();
            TPZVec<STATE> solini(nrs,pini);

            TPZCompMesh  * cmeshL2 = mydata->CMeshPressureL2(gmesh, pp, solini,triang);
            TPZAnalysis anL2(cmeshL2);
            mydata->SolveSist(anL2, cmeshL2);
            //anL2.Solution().Print("sol");

            an3.LoadSolution(anL2.Solution());

            //malha multifisica
            TPZVec<TPZCompMesh *> meshvec(3);
            meshvec[0] = cmesh1;
            meshvec[1] = cmesh2;
            meshvec[2] = cmesh3;
            TPZPoroElasticMF2d * mymaterial;
            TPZAutoPointer<TPZFunction<STATE> > solExata = new TPZDummyFunction<STATE>(TerzaghiProblem1D);
            TPZCompMesh * mphysics = mydata->MalhaCompTerzaghi(gmesh,meshvec,mymaterial,solExata);

            
            int NDeltaT = 10000;
            int intervsaidas = NDeltaT/20;
            REAL deltaT=timeT/NDeltaT; //second
            mymaterial->SetTimeStep(deltaT);
            REAL maxTime = timeT;

            //mydata->SolveSistTransient(deltaT, maxTime, mymaterial, meshvec, mphysics,100,ftimeatual);

            
            //==== Imprimir erros ======
            TPZAnalysis an(mphysics);
            TPZFMatrix<STATE> Initialsolution = an.Solution();

            std::string outputfile;
            outputfile = "TransientSolution";

            //        std::stringstream outputfiletemp;
            //        outputfiletemp << outputfile << ".vtk";
            //        std::string plotfile = outputfiletemp.str();
            //        mydata->PosProcessMultphysics(meshvec,mphysics,an,plotfile);

            //Criando matriz de massa (matM)
            TPZAutoPointer <TPZMatrix<STATE> > matM = mydata->MassMatrix(mymaterial, mphysics);

            //Criando matriz de rigidez (matK) e vetor de carga
            TPZFMatrix<STATE> matK;
            TPZFMatrix<STATE> fvec;
            mydata->StiffMatrixLoadVec(mymaterial, mphysics, an, matK, fvec);

            int nrows;
            nrows = matM->Rows();
            TPZFMatrix<STATE> TotalRhs(nrows,1,0.0);
            TPZFMatrix<STATE> TotalRhstemp(nrows,1,0.0);
            TPZFMatrix<STATE> Lastsolution = Initialsolution;

            REAL TimeValue = 0.0;
            int cent = 1;
            TimeValue = cent*deltaT;
            while (TimeValue <= maxTime)
            {
                ftimeatual  = TimeValue;
                // This time solution i for Transient Analytic Solution
                mymaterial->SetTimeValue(TimeValue);
                matM->Multiply(Lastsolution,TotalRhstemp);
                
                TotalRhs = fvec + TotalRhstemp;
                an.Rhs() = TotalRhs;
                an.Solve();
                Lastsolution = an.Solution();
                
                if(cent%intervsaidas==0){
                    saidaerro<<"\n========= PARA O PASSO n = "<< cent <<"  E TEMPO tn = "<< TimeValue <<" =========\n"<<endl;
                    
                    std::stringstream outputfiletemp;
                    outputfiletemp << outputfile << ".vtk";
                    std::string plotfile = outputfiletemp.str();
                    mydata->PosProcessMultphysics(meshvec,mphysics,an,plotfile);
                    
                    
                    TPZVec<REAL> erros;
                    
                    saidaerro<<" \nErro da simulacao multifisica do fluxo (q)" <<endl;
                    TPZAnalysis an22(cmesh2);
                    an22.SetExact(*SolucaoPQTerzaghi);
                    an22.PostProcessError(erros, saidaerro);
                    
                    saidaerro<<" Erro da simulacao multifisica da pressao (p)" <<endl;
                    TPZAnalysis an32(cmesh3);
                    an32.SetExact(*SolucaoPQTerzaghi);
                    an32.PostProcessError(erros, saidaerro);
                }
                
                
                cent++;
                TimeValue = cent*deltaT;
            }

            cmesh1->CleanUp();
            cmesh2->CleanUp();
            cmesh3->CleanUp();
            //mphysics->CleanUp();
            delete cmesh1;
            delete cmesh2;
            delete cmesh3;
            //delete mphysics;
            delete gmesh;

        }
    }
    
    return 0;
}

void TerzaghiProblem1D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux){
    
    //REAL x = ptx[0];
	REAL y = 1.- ptx[1];
	
    REAL alpha = 1.;
    REAL Se = 1.e-1;
    
    REAL Eyoung = 1.e5;
    REAL poisson = 0.2;
    REAL lamb = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
    REAL mu = 0.5*Eyoung/(1.+poisson);
    
	REAL visc =1.e-4;
	REAL perm =  1.e-10;
    
    REAL H=1.;
    REAL F = 1000.;
	REAL tp = ftimeatual;
	
    REAL kappa = perm/visc;
    REAL K = lamb + (2./3.)*mu;
    REAL Ku = K + alpha*alpha/Se;
    REAL Cf = (1./Se)*kappa*(K + (4./3.)*mu)/(Ku + (4./3.)*mu);
    REAL S0 = (F*H)/(Ku + (4./3.)*mu);
    REAL SInfty = (F*H)/(K + (4./3.)*mu);
    
    
	int in;
	REAL p = 0.0, uyH = 0.0, VDy = 0.0;
    REAL sump = 0.0, sumuyH = 0.0, sumVDy=0.0;
    
	REAL PI = atan(1.)*4.;
	
	sol.Resize(5, 0.);
    flux(0,0)=0.;
    flux(1,0)=0.;
	
    REAL aux1= (alpha*F)/(Se*(Ku + (4./3.)*mu));
    REAL aux2=0.;
    REAL aux3=0.;
    
    if(fdimensionless==true)
    {
        for (in =0; in<1000; in++)
        {
            aux2 = 4./(PI*(2.*in + 1.));
            aux3 = PI*(2.*in + 1.)/2.;
            
            sump += aux2*sin(aux3*y)*exp(-aux3*aux3*tp);
            
            sumuyH += 0.5*aux2*aux2*exp(-aux3*aux3*tp);
            
            sumVDy += cos(aux3*y)*exp(-aux3*aux3*tp);
        }
        
        p = sump;
        uyH = SInfty + (S0 - SInfty)*sumuyH/aux1;
        VDy = (-2.)*sumVDy;
        
        sol[0] = p;
        sol[2] = -uyH;
        flux(1,0) = -VDy;
        
    }else{
        for (in =0; in<1000; in++)
        {
            aux2 = 4./(PI*(2.*in + 1.));
            aux3 = Pi*(2.*in + 1.)/(2.*H);
            
            sump += aux2*sin(aux3*y)*exp(-aux3*aux3*Cf*tp);
            
            sumuyH += 0.5*aux2*aux2*exp(-aux3*aux3*Cf*tp);
            
            sumVDy += cos(aux3*y)*exp(-aux3*aux3*Cf*tp);
        }
        
        p = aux1*sump;
        uyH = SInfty + (S0 - SInfty)*sumuyH;
        VDy = -(2./H)*kappa*aux1*sumVDy;
        
        
        sol[0] = p;
        sol[2] = -uyH;
        flux(1,0) = -VDy;
    }
}

void SolucaoPQTerzaghi(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux)
{
 
    bool dimensionless = true;
    
    //REAL x = ptx[0];
	REAL y = 1.- ptx[1];
	
    REAL alpha = 1.;
    REAL Se = 1.e-1;
    
    REAL Eyoung = 1.e5;
    REAL poisson = 0.2;
    REAL lamb = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
    REAL mu = 0.5*Eyoung/(1.+poisson);
    
	REAL visc =1.e-4;
	REAL perm =  1.e-10;
    
    REAL H=1.;
    REAL F = 1000.;
	REAL tp = ftimeatual;
	
    REAL kappa = perm/visc;
    REAL K = lamb + (2./3.)*mu;
    REAL Ku = K + alpha*alpha/Se;
    REAL Cf = (1./Se)*kappa*(K + (4./3.)*mu)/(Ku + (4./3.)*mu);
    
	int in;
	REAL p = 0.0, VDy = 0.0;// DivVD = 0.0;
    REAL sump = 0.0, sumVDy=0.0;//, sumDiv = 0.0;
    
	REAL PI = atan(1.)*4.;
	
	sol.Resize(1, 0.);
    flux.Resize(2,1);
    flux(0,0)=0.;
    flux(1,0)=0.;
    //flux(2,0)=0.;
	
    REAL aux1= (alpha*F)/(Se*(Ku + (4./3.)*mu));
    REAL aux2=0.;
    REAL aux3=0.;
    
    if(dimensionless==true)
    {
        for (in =0; in<1000; in++)
        {
            aux2 = 4./(PI*(2.*in + 1.));
            aux3 = PI*(2.*in + 1.)/2.;
            
            sump += aux2*sin(aux3*y)*exp(-aux3*aux3*tp);
            
            sumVDy += cos(aux3*y)*exp(-aux3*aux3*tp);
            
            //sumDiv += (-aux3)*sin(aux3*y)*exp(-aux3*aux3*tp);
        }
        
        p = sump;
        VDy = -2.*sumVDy;
        //DivVD = 2.*sumDiv;
        
        sol[0] = p;
        flux(1,0) = -VDy;
        //flux(2,0) = -DivVD;
        
    }else{
        for (in =0; in<1000; in++)
        {
            aux2 = 4./(PI*(2.*in + 1.));
            aux3 = Pi*(2.*in + 1.)/(2.*H);
            
            sump += aux2*sin(aux3*y)*exp(-aux3*aux3*Cf*tp);
            
            sumVDy += cos(aux3*y)*exp(-aux3*aux3*Cf*tp);
            
            //sumDiv += (-aux3)*sin(aux3*y)*exp(-aux3*aux3*tp);
        }
        
        p = aux1*sump;
        VDy = (2./H)*kappa*aux1*sumVDy;
       // DivVD = (2./H)*kappa*aux1*sumDiv;
        
        sol[0] = p;
        flux(1,0) = -VDy;
        //flux(2,0) = -DivVD;
    }
}

//Problema Barry and Mercer
int main(int argc, char *argv[]){
    
    #ifdef LOG4CXX
        InitializePZLOG();
    #endif
    
    gRefDBase.InitializeUniformRefPattern(EOned);
    gRefDBase.InitializeUniformRefPattern(EQuadrilateral);
    
    bool triang=false;
    bool dimensionless=true;
    
    REAL Eyoung = 1.e5;
    REAL poisson = 0.1;
    
    REAL alpha = 1.0;
    REAL Se = 0.;
    
    REAL sig0 =0.;
    REAL pini = 0.;
    
    REAL Lx = 1.;
    REAL Ly = 1.;
    
    REAL rockrho = 0.;
    REAL gravity = 0.;
    REAL fx=0.0;
    REAL fy = gravity*rockrho;
    REAL perm = 1.e-6;
    REAL visc = 1.e-4;
    
    REAL timeT = 0.00153589;
    
    
    //Incompressible fluid (Se=0)
    if(dimensionless==true)
    {
        REAL lambda = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));//firstlame
        REAL mu = 0.5*Eyoung/(1+poisson);//secondlame
        REAL Lref = sqrt(Lx*Ly);
        REAL Se_aux = alpha*alpha/(lambda+2.*mu);
        REAL kovervisc = perm/visc;
        REAL Cf = kovervisc/Se_aux;//fluid diffusivity coeffcient
        REAL lambdaD = lambda*Se_aux;
        REAL muD = mu*Se_aux;
        REAL beta = Cf/(Ly*Ly);
        
        
        //adimemsionalizando
        Eyoung = muD*(3.*lambdaD+2.*muD)/(lambdaD+muD);
        poisson = 0.5*lambdaD/(lambdaD+muD);
        sig0 = 0.;
        pini = 0.;
        timeT = timeT*beta;
        Lx = Lx/Lref;
        Ly = Ly/Lref;
        perm = 1.;
        visc = 1.;
        fx = 0.;
        fy = 0.;
        Se = 0.;
    }
    
    {//NAO APAGAR!!!
        char buf[] =
        "16 10 "
        "-50 Qua000022224 "
        "-1 -1 0 "
        "1 -1 0 "
        "1 1 0  "
        "-1 1 0 "
        "-0.33333 -1 0  "
        "+0.33333 -1 0  "
        "+1 -0.33333 0  "
        "+1 +0.33333 0  "
        "+0.33333 +1 0  "
        "-0.33333 +1 0  "
        "-1 +0.33333 0  "
        "-1 -0.33333 0  "
        "-0.33333 -0.33333 0    "
        "+0.33333 -0.33333 0    "
        "+0.33333 +0.33333 0    "
        "-0.33333 +0.33333 0    "
        "3 4 0  1  2  3 "
        "3 4 0 4 12 11  "
        "3 4 4 5 13 12  "
        "3 4 5 1 6 13   "
        "3 4 11 12 15 10    "
        "3 4 12 13 14 15    "
        "3 4 13 6 7 14  "
        "3 4 10 15 9 3  "
        "3 4 15 14 8 9  "
        "3 4 14 7 2 8   ";
        std::istringstream str(buf);
        TPZAutoPointer<TPZRefPattern> refp = new TPZRefPattern(str);
        refp->GenerateSideRefPatterns();
        gRefDBase.InsertRefPattern(refp);
        if(!refp)
        {
            DebugStop();
        }
    }
    
    //    {//NAO APAGAR!!!
    //        char buf[] =
    //        "4 4 "
    //        "-50 Lin000022224 "
    //        "-1 0 0 "
    //        "1 0 0 "
    //        "-0.33333 0 0  "
    //        "+0.33333 0 0  "
    //        "1 2 0 1  "
    //        "1 2 0 2  "
    //        "1 2 2 3  "
    //        "1 2 3 1  ";
    //        std::istringstream str(buf);
    //        TPZAutoPointer<TPZRefPattern> refp = new TPZRefPattern(str);
    //        gRefDBase.InsertRefPattern(refp);
    //        if(!refp)
    //        {
    //            DebugStop();
    //        }
    //    }
    
    
    DadosMalhas * mydata = new DadosMalhas();
    
    TPZVec<REAL> pt(3);
    pt[0] = Lx/4.;
    pt[1] = Ly/4.;
    pt[2] = 0.;
    REAL Area;
    
    ofstream saidaerro("Erro.txt");
    for(int p = 1; p<2; p++)
    {
        int pu = p+1;
        int pq = p;
        int pp = p;
        int h;
        
        saidaerro<<"\n CALCULO DO ERRO, ELEM. TRIANG., COM ORDEM POLINOMIAL pu = "<< pu << ", pq = "<< pq << " e pp = "<< pp<<endl;
        for (h = 1; h< 2; h++)
        {
            
            saidaerro<<"\n========= PARA h = "<< h<<"  ============= "<<endl;
            
            TPZGeoMesh * gmesh = mydata->GMesh4(Lx,Ly);
            mydata->UniformRefine(gmesh, h);
            RefinamentoPadrao3x3(gmesh,h);
            
            int newmatId = mydata->GetIdSourceTerm();//mat id of the source term
            RefinamentoPadrao3x3(gmesh,h,pt, true, newmatId, Area);
            felarea = Area;
            
            //            std::ofstream malhaGeo("gmesh2D.txt");
            //            gmesh->Print(malhaGeo);
            
            
            mydata->SetParameters(Eyoung, poisson, alpha, Se, perm, visc, fx, fy, sig0);
            
            
            // First computational mesh
            TPZCompMesh * cmesh1 = mydata->MalhaCompElast(gmesh,pu,true);
            ofstream arg1("cmesh1.txt");
            cmesh1->Print(arg1);
            
            // second computational mesh
            TPZCompMesh * cmesh2= mydata->CMeshFlux(gmesh, pq,true);
            ofstream arg2("cmesh2.txt");
            cmesh2->Print(arg2);
            
            // Third computational mesh
            TPZCompMesh * cmesh3=mydata->CMeshPressure(gmesh, pp,triang,true);
            ofstream arg3("cmesh3.txt");
            cmesh3->Print(arg3);
            
            //malha multifisica
            TPZVec<TPZCompMesh *> meshvec(3);
            meshvec[0] = cmesh1;
            meshvec[1] = cmesh2;
            meshvec[2] = cmesh3;
            
            TPZAutoPointer<TPZFunction<STATE> > sourceterm = new TPZDummyFunction<STATE>(ForcingSource);
            TPZAutoPointer<TPZFunction<STATE> > solExata = new TPZDummyFunction<STATE>(BarryMercerProblem2D);
            
            TPZCompMesh * mphysics = mydata->MalhaCompBarryMercer(gmesh,meshvec,sourceterm, solExata);
            ofstream arg8("mphysic.txt");
            mphysics->Print(arg8);
            
            //            ofstream arg9("gmesh_final.txt");
            //            gmesh->Print(arg9);
            
            
            int NDeltaT = 1;
            int intervsaidas = NDeltaT/1;
            REAL deltaT=timeT/NDeltaT; //second
            REAL maxTime = timeT;
            fdeltaT = deltaT;
            
            //mydata->SolveSistBarryMercert(deltaT, maxTime, meshvec, mphysics,1,ftimeatual);
            
            
            ///======== Impimir Erros ========
            TPZMaterial *mat1 = mphysics->FindMaterial(mydata->GetMatId());
            TPZMaterial *mat2 = mphysics->FindMaterial(mydata->GetMatId()+1);
            TPZPoroElasticMF2d * mat12 = dynamic_cast<TPZPoroElasticMF2d *>(mat1);
            TPZPoroElasticMF2d * mat22 = dynamic_cast<TPZPoroElasticMF2d *>(mat2);
            
            mat12->SetTimeStep(deltaT);
            mat22->SetTimeStep(deltaT);
            
            TPZAnalysis an(mphysics);
            TPZFMatrix<STATE> Initialsolution = an.Solution();
            
            std::string outputfile;
            outputfile = "TransientSolution";
            
            //Criando matriz de massa (matM)
            TPZAutoPointer <TPZMatrix<STATE> > matM = mydata->MassMatrix(mphysics);
            
            //Criando matriz de rigidez (matK) e vetor de carga
            TPZFMatrix<STATE> matK;
            TPZFMatrix<STATE> fvec;
            mydata->StiffMatrixLoadVec(mphysics, an, matK, fvec);
            
            int nrows;
            nrows = matM->Rows();
            TPZFMatrix<STATE> TotalRhs(nrows,1,0.0);
            TPZFMatrix<STATE> TotalRhstemp(nrows,1,0.0);
            TPZFMatrix<STATE> Lastsolution = Initialsolution;
            
            REAL TimeValue = 0.0;
            int cent = 1;
            TimeValue = cent*deltaT;
            while (TimeValue <= maxTime)
            {
                ftimeatual  = TimeValue;
                // This time solution i for Transient Analytic Solution
                matM->Multiply(Lastsolution,TotalRhstemp);
                mydata->StiffMatrixLoadVec(mphysics, an, matK, fvec);
                
                TotalRhs = fvec + TotalRhstemp;
                an.Rhs() = TotalRhs;
                an.Solve();
                Lastsolution = an.Solution();
                
                if(cent%intervsaidas==0)
                {
                    saidaerro<<"\n========= PARA O PASSO n = "<< cent <<"  E TEMPO tn = "<< TimeValue <<" =========\n"<<endl;
                    
                    std::stringstream outputfiletemp;
                    outputfiletemp << outputfile << ".vtk";
                    std::string plotfile = outputfiletemp.str();
                    mydata->PosProcessMultphysics(meshvec,mphysics,an,plotfile);
                    
                    TPZVec<REAL> erros;
                    
                    saidaerro<<" Erro da simulacao multifisica do deslocamento (u)" <<endl;
                    TPZAnalysis an12(cmesh1);
                    an12.SetExact(*SolucaoUBarryMercer);
                    an12.PostProcessError(erros, saidaerro);
                    
                    
                    saidaerro<<" \nErro da simulacao multifisica do fluxo (q)" <<endl;
                    TPZAnalysis an22(cmesh2);
                    an22.SetExact(*SolucaoPQBarryMercer);
                    an22.PostProcessError(erros, saidaerro);
                    
                    saidaerro<<" Erro da simulacao multifisica da pressao (p)" <<endl;
                    TPZAnalysis an32(cmesh3);
                    an32.SetExact(*SolucaoPQBarryMercer);
                    an32.PostProcessError(erros, saidaerro);
                }
                
                cent++;
                TimeValue = cent*deltaT;
                
            }
            
            cmesh1->CleanUp();
            cmesh2->CleanUp();
            cmesh3->CleanUp();
            //mphysics->CleanUp();
            delete cmesh1;
            delete cmesh2;
            delete cmesh3;
            //delete mphysics;
            delete gmesh;
        }
    }
    
    return 0;
}

void ForcingSource(const TPZVec<REAL> &pt, TPZVec<STATE> &disp){
    
    REAL tp = ftimeatual;
    REAL deltaT = fdeltaT;
    REAL elarea = felarea;
    
    REAL temp = (cos(tp) - cos(tp+deltaT))/elarea;
    disp[0]= 2.*temp/deltaT;
}

void BarryMercerProblem2D(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux){
    
    bool dimensionless = true;
    
    REAL x = ptx[0];
	REAL y = ptx[1];
	
    //dados adimensional
    REAL Lx = 1.;
    REAL Ly = 1.;
    REAL x0 = 0.25;
    REAL y0= 0.25;
    REAL perm = 1.;
    REAL visc = 1.;
    REAL kovervisc =perm/visc;
    REAL tp = ftimeatual;
    
    //dados com dimensao
    REAL beta = 0.;
    REAL Lref = 0.;
    REAL lamb=0., mu=0.;
    if(dimensionless == false){
        
        REAL Eyoung = 1.e5;
        REAL poisson = 0.1;
        lamb = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
        mu = 0.5*Eyoung/(1.+poisson);
        perm =  1.e-6;
        visc =1.e-4;
        kovervisc =perm/visc;
        Lx =1.;
        Ly = 1.;
        Lref = sqrt(Lx*Ly);
        beta = (lamb+2.*mu)*kovervisc/Lref;
        tp = beta*tp;
    }
	
    int in, jq;
    REAL PI = atan(1.)*4.;
    
    sol.Resize(5, 0.);
	
    flux(0,0)=0.;
    flux(1,0)=0.;
    
    REAL gamman = 0., gammaq = 0., gammanq = 0.;
	REAL ptil = 0., util = 0., wtil =0., temp=0.;
    REAL sump=0., sumux = 0., sumuy = 0., sumVDx = 0., sumVDy=0.;
    
    for(in=1; in<500; in++)
    {
        gamman = in*PI;
        for(jq=1; jq<500; jq++)
        {
            gammaq = jq*PI;
            gammanq = gamman*gamman + gammaq*gammaq;
            temp = -2./(gammanq*gammanq+1);
            
            ptil = temp*sin(gamman*x0)*sin(gammaq*y0)*(gammanq*sin(tp) - cos(tp) + exp(-gammanq*tp));
            util = gamman*ptil/gammanq;
            wtil = gammaq*ptil/gammanq;
            temp = 4./(Lx*Ly);
            
            sump += temp*ptil*sin(gamman*x)*sin(gammaq*y); //(multiplicado por -1, multipliquei depois da soma)
            sumux += temp*util*cos(gamman*x)*sin(gammaq*y);
            sumuy += temp*wtil*sin(gamman*x)*cos(gammaq*y);
            sumVDx += temp*ptil*gamman*cos(gamman*x)*sin(gammaq*y);//(multiplicado por -1, multipliquei depois da soma)
            sumVDy += temp*ptil*gammaq*sin(gamman*x)*cos(gammaq*y);//(multiplicado por -1, multipliquei depois da soma)
        }
    }
    
    if(dimensionless == false){
        sol[0] = -(lamb+2.*mu)*sump;
        sol[1] = Lref*sumux;
        sol[2] = Lref*sumuy;
        flux(0,0) = kovervisc*(lamb+2.*mu)*sumVDx; //(multiplicado por -kovervisc, cancela com o outro sinal de menos)
        flux(1,0) = kovervisc*(lamb+2.*mu)*sumVDy; //(multiplicado por -kovervisc, cancela com o outro sinal de menos)
    }
    else{
        sol[0] = -sump;
        sol[1] = sumux;
        sol[2] = sumuy;
        flux(0,0) = sumVDx;//(multiplicado por -kovervisc, cancela com o outro sinal de menos)
        flux(1,0) = sumVDy;//(multiplicado por -kovervisc, cancela com o outro sinal de menos)
    }
}

void SolucaoUBarryMercer(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &deriv){
    
    
    bool dimensionless = true;
    
    REAL x = ptx[0];
	REAL y = ptx[1];
	
    //dados adimensional
    REAL Lx = 1.;
    REAL Ly = 1.;
    REAL x0 = 0.25;
    REAL y0= 0.25;
    REAL perm = 1.;
    REAL visc = 1.;
    REAL kovervisc =perm/visc;
    REAL tp = ftimeatual;
    
    //dados com dimensao
    REAL beta = 0.;
    REAL Lref = 0.;
    REAL lamb=0., mu=0.;
    if(dimensionless == false){
        
        REAL Eyoung = 1.e5;
        REAL poisson = 0.1;
        lamb = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
        mu = 0.5*Eyoung/(1.+poisson);
        perm =  1.e-6;
        visc =1.e-4;
        kovervisc =perm/visc;
        Lx =1.;
        Ly = 1.;
        Lref = sqrt(Lx*Ly);
        beta = (lamb+2.*mu)*kovervisc/Lref;
        tp = beta*tp;
    }
	
    int in, jq;
    REAL PI = atan(1.)*4.;
    
    sol.Resize(2, 0.);// ux, uy;
    deriv.Resize(2,2);//sigx, sigxy, sigyx, sigy
    deriv(0,0) = deriv(0,1) = deriv(1,0) = deriv(1,1) = 0.;
    
    REAL gamman = 0., gammaq = 0., gammanq = 0.;
	REAL ptil = 0., util = 0., wtil =0., temp=0.;
    REAL sumux = 0., sumuy = 0., sumsigx=0., sumsigy=0., sumsigxy=0.;
    
    for(in=1; in<500; in++)
    {
        gamman = in*PI;
        for(jq=1; jq<500; jq++)
        {
            gammaq = jq*PI;
            gammanq = gamman*gamman + gammaq*gammaq;
            temp = -2./(gammanq*gammanq+1);
            
            ptil = temp*sin(gamman*x0)*sin(gammaq*y0)*(gammanq*sin(tp) - cos(tp) + exp(-gammanq*tp));
            util = gamman*ptil/gammanq;
            wtil = gammaq*ptil/gammanq;
            temp = 4./(Lx*Ly);
            
            sumux += temp*util*cos(gamman*x)*sin(gammaq*y);
            sumuy += temp*wtil*sin(gamman*x)*cos(gammaq*y);
            sumsigx += gamman*temp*util*sin(gamman*x)*sin(gammaq*y);//multiplicado por -1
            sumsigy += gammaq*temp*wtil*sin(gamman*x)*sin(gammaq*y);//multiplicado por -1
            sumsigxy += gammaq*temp*util*cos(gamman*x)*cos(gammaq*y);
        }
    }
    
    if(dimensionless == false){
        std::cout<<"nao foi calculdo\n";
        DebugStop();
    }
    else{
        sol[0] = sumux;
        sol[1] = sumuy;
        deriv(0,0) = -sumsigx;
        deriv(1,1) = -sumsigy;
        deriv(0,1) = sumsigxy;
        deriv(1,0) = sumsigxy;
    }
    
}

void SolucaoPQBarryMercer(const TPZVec<REAL> &ptx, TPZVec<STATE> &sol, TPZFMatrix<STATE> &flux){
    
    bool dimensionless = true;
    
    REAL x = ptx[0];
	REAL y = ptx[1];
	
    //dados adimensional
    REAL Lx = 1.;
    REAL Ly = 1.;
    REAL x0 = 0.25;
    REAL y0= 0.25;
    REAL perm = 1.;
    REAL visc = 1.;
    REAL kovervisc =perm/visc;
    REAL tp = ftimeatual;
    
    //dados com dimensao
    REAL beta = 0.;
    REAL Lref = 0.;
    REAL lamb=0., mu=0.;
    if(dimensionless == false){
        
        REAL Eyoung = 1.e5;
        REAL poisson = 0.1;
        lamb = (Eyoung*poisson)/((1.+poisson)*(1.-2.*poisson));
        mu = 0.5*Eyoung/(1.+poisson);
        perm =  1.e-6;
        visc =1.e-4;
        kovervisc =perm/visc;
        Lx =1.;
        Ly = 1.;
        Lref = sqrt(Lx*Ly);
        beta = (lamb+2.*mu)*kovervisc/Lref;
        tp = beta*tp;
    }
	
    int in, jq;
    REAL PI = atan(1.)*4.;
    
    sol.Resize(1, 0.);
	flux.Redim(3, 1);
    flux(0,0)=flux(1,0)=flux(2,0)=0.;
    
    REAL gamman = 0., gammaq = 0., gammanq = 0.;
	REAL ptil = 0., util = 0., wtil =0., temp=0.;
    REAL sump=0., sumVDx = 0., sumVDy=0.;
    
    for(in=1; in<500; in++)
    {
        gamman = in*PI;
        for(jq=1; jq<500; jq++)
        {
            gammaq = jq*PI;
            gammanq = gamman*gamman + gammaq*gammaq;
            temp = -2./(gammanq*gammanq+1);
            
            ptil = temp*sin(gamman*x0)*sin(gammaq*y0)*(gammanq*sin(tp) - cos(tp) + exp(-gammanq*tp));
            util = gamman*ptil/gammanq;
            wtil = gammaq*ptil/gammanq;
            temp = 4./(Lx*Ly);
            
            sump += temp*ptil*sin(gamman*x)*sin(gammaq*y);//(multiplicado por -1, multipliquei depois da soma)
            sumVDx += temp*ptil*gamman*cos(gamman*x)*sin(gammaq*y);//(multiplicado por -1, multipliquei depois da soma)
            sumVDy += temp*ptil*gammaq*sin(gamman*x)*cos(gammaq*y);
        }
    }
    
    if(dimensionless == false){
        sol[0] = -(lamb+2.*mu)*sump;
        flux(0,0) = kovervisc*(lamb+2.*mu)*sumVDx;//(multiplicado por -kovervisc, cancela com o outro sinal de menos)
        flux(1,0) = kovervisc*(lamb+2.*mu)*sumVDy;
    }
    else{
        sol[0] = -sump;
        flux(0,0) = sumVDx;//(multiplicado por -kovervisc, cancela com o outro sinal de menos)
        flux(1,0) = sumVDy;
    }
    
}


void RefinamentoPadrao3x3(TPZGeoMesh *gmesh, int nref){
    
    TPZAutoPointer<TPZRefPattern> refp2D = gRefDBase.FindRefPattern("Qua000022224");
    //TPZAutoPointer<TPZRefPattern> refp1D = gRefDBase.FindRefPattern("Lin000022224");
    
    if(!refp2D) DebugStop();
    
    TPZGeoEl * gel = NULL;
    for(int r = 0; r < nref; r++)
    {
        int nels = gmesh->NElements();
        for(int iel = 0; iel < nels; iel++)
        {
            gel = gmesh->ElementVec()[iel];
            if(!gel) DebugStop();
            if(gel->Dimension()==2)
            {
                gel->SetRefPattern(refp2D);
                TPZVec<TPZGeoEl*> sons;
                gel->Divide(sons);
            }
            if(gel->Dimension()==1)
            {
                TPZAutoPointer<TPZRefPattern> refp1D = TPZRefPatternTools::PerfectMatchRefPattern(gel);
                if(refp1D)
                {
                    gel->SetRefPattern(refp1D);
                    TPZVec<TPZGeoEl*> sons;
                    gel->Divide(sons);
                }
                else{
                    DebugStop();//nao conseguiu refinar elementos 1D baseados no refp do pai
                }
            }
            
        }
    }
    
    std::ofstream malhaOut("malhaOut.vtk");
    TPZVTKGeoMesh::PrintGMeshVTK(gmesh, malhaOut, true);
}

void RefinamentoPadrao3x3(TPZGeoMesh *gmesh, int nref,TPZVec<REAL> pt, bool changeMatId, int newmatId, REAL &Area){
    
    TPZAutoPointer<TPZRefPattern> refpOutroLugar = gRefDBase.FindRefPattern("Qua000022224");
    if(!refpOutroLugar) DebugStop();
    
    int iniEl = 0;
    TPZVec<REAL> qsi(2,0.);
    
    TPZGeoEl * gel = NULL;
    for(int r = 0; r < nref; r++)
    {
        gel = gmesh->FindElement(pt, qsi, iniEl, 2);
        if(!gel) DebugStop();
        if(gel->Dimension()==2)
        {
            gel->SetRefPattern(refpOutroLugar);
            TPZVec<TPZGeoEl*> sons;
            gel->Divide(sons);
            for(int edg = gel->NNodes(); edg < gel->NSides(); edg++)
            {
                TPZGeoElSide gelS(gel,edg);
                if(gelS.Dimension() > 1)
                {
                    break;
                }
                TPZGeoElSide neighS(gelS.Neighbour());
                while(neighS != gelS)
                {
                    if(neighS.Element()->Dimension() == 1)
                    {
                        TPZAutoPointer<TPZRefPattern> refp = gel->GetRefPattern();
                        TPZAutoPointer<TPZRefPattern> refp2 = refp->SideRefPattern(gelS.Side());
                        if(refp2)
                        {
                            TPZGeoEl * gel1D = neighS.Element();
                            gel1D->SetRefPattern(refp2);
                            TPZVec<TPZGeoEl*> sons2;
                            gel1D->Divide(sons2);
                        }
                    }
                    neighS = neighS.Neighbour();
                }
            }
        }
    }
    
    if(changeMatId==true)
    {
        gel = gmesh->FindElement(pt, qsi, iniEl, 2);
        if(!gel) DebugStop();
        gel->SetMaterialId(newmatId);
        Area = gel->Volume();
    }
    
    std::ofstream malhaOut("malhaOut2.vtk");
    TPZVTKGeoMesh::PrintGMeshVTK(gmesh, malhaOut, true);
}
