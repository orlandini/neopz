//
//  toolstransienttime.cpp
//  PZ
//
//  Created by Agnaldo Farias on 9/5/12.
//  Copyright (c) 2012 LabMec-Unicamp. All rights reserved.
//

#include <iostream>

#include "toolstransienttime.h"
#include "pzbuildmultiphysicsmesh.h"
#include "pzcompel.h"
#include "pzintel.h"
#include "pzskylstrmatrix.h"
#include "pzstepsolver.h"
#include "TPZSpStructMatrix.h"
#include "pzelastpressure.h"
#include "pznlfluidstructure2d.h"
#include "pzfstrmatrix.h"
#include "../HydraulicFracturePropagation/PlaneFracture/TPZJIntegral.h"
#include "pzreducedspace.h"
#include "tpzcompmeshreferred.h"

#include <fstream>
#include <sstream>

#include "pzlog.h"
#ifdef LOG4CXX
static LoggerPtr logdata(Logger::getLogger("pz.toolstransienttime"));
#endif

const double timeScale = 1.;

ToolsTransient::ToolsTransient(){
    
}

ToolsTransient::~ToolsTransient(){
    
}

TPZFMatrix<REAL> ToolsTransient::InitialSolution(TPZGeoMesh *gmesh, TPZCompMesh * cmesh, int matId, int porder, REAL valsol){
    
    TPZAnalysis an(cmesh);
	int nrs = an.Solution().Rows();
    TPZVec<REAL> initsol(nrs,valsol);
    int dim = cmesh->Dimension();
    
    TPZCompMesh  * cmesh_projL2 = CMeshProjectionL2(gmesh, dim, matId, porder, initsol);
    TPZAnalysis anL2(cmesh_projL2);
	TPZSkylineStructMatrix full(cmesh_projL2);
	anL2.SetStructuralMatrix(full);
	TPZStepSolver<REAL> step;
	step.SetDirect(ELDLt);
	anL2.SetSolver(step);
	anL2.Run();
    
    TPZFMatrix<REAL> InitialSolution=anL2.Solution();
    cmesh->LoadSolution(InitialSolution);
    
    return InitialSolution;
}

TPZCompMesh * ToolsTransient::CMeshProjectionL2(TPZGeoMesh *gmesh, int dim, int matId, int pOrder, TPZVec<STATE> &solini)
{
    /// criar materiais
	TPZL2Projection *material;
	material = new TPZL2Projection(matId, dim, 1, solini, pOrder);
    
    TPZCompMesh * cmesh = new TPZCompMesh(gmesh);
    cmesh->SetDimModel(dim);
    TPZMaterial * mat(material);
    cmesh->InsertMaterialObject(mat);
    
	cmesh->SetAllCreateFunctionsContinuous();
    
	cmesh->SetDefaultOrder(pOrder);
    cmesh->SetDimModel(dim);
	
	//Ajuste da estrutura de dados computacional
	cmesh->AutoBuild();
    
	return cmesh;
}

void ToolsTransient::StiffMatrixLoadVec(TPZNLFluidStructure2d *mymaterial, TPZCompMesh* mphysics, TPZAnalysis *an, TPZFMatrix<REAL> &matK1, TPZFMatrix<REAL> &fvec)
{
	mymaterial->SetCurrentState();
    TPZFStructMatrix matsk(mphysics);
    //TPZSkylineStructMatrix matsk(mphysics);
	an->SetStructuralMatrix(matsk);
	TPZStepSolver<REAL> step;
	//step.SetDirect(ELDLt);
	step.SetDirect(ELU);
	an->SetSolver(step);
    
    an->Assemble();
	
	matK1 = an->StructMatrix();
	fvec = an->Rhs();
}

TPZAutoPointer <TPZMatrix<REAL> > ToolsTransient::MassMatrix(TPZNLFluidStructure2d *mymaterial, TPZCompMesh *mphysics){
    
    mymaterial->SetLastState();
	TPZSpStructMatrix matsp(mphysics);
	TPZAutoPointer<TPZGuiInterface> guiInterface;
	TPZFMatrix<REAL> Un;
    TPZAutoPointer <TPZMatrix<REAL> > matK2 = matsp.CreateAssemble(Un,guiInterface);
    
    return matK2;
}


void ToolsTransient::SolveSistTransient(REAL deltaT,REAL maxTime, TPZFMatrix<REAL> InitialSolution , TPZAnalysis *an, TPZNLFluidStructure2d * &mymaterial, TPZVec<TPZCompMesh *> meshvec, TPZCompMesh* mphysics)
{
    std::ofstream outPW("OUTPUT.nb");
    
    std::string outputfile;
	outputfile = "TransientSolution";
    
    std::stringstream outP, outW, outJ;
    
    //Criando matriz de massa (matM)
    TPZAutoPointer <TPZMatrix<REAL> > matM = MassMatrix(mymaterial, mphysics);
    //mymaterial->UpdateLeakoff();
    
    outP << "Saida" << 0 << "={";
    SaidaMathPressao(meshvec, mphysics, outP);
    outP << "};\n";
    
	int nrows;
	nrows = an->Solution().Rows();
	TPZFMatrix<REAL> res_total(nrows,1,0.0);
	TPZFMatrix<REAL> Mass_X_SolTimeN(nrows,1,0.0);
    TPZFMatrix<REAL> chutenewton(meshvec[0]->Solution().Rows(),1,1.);
    meshvec[0]->LoadSolution(chutenewton);
    TPZBuildMultiphysicsMesh::TransferFromMeshes(meshvec, mphysics);
    TPZFMatrix<REAL> SolIterK = mphysics->Solution();
	TPZFMatrix<REAL> matK;
	TPZFMatrix<REAL> fres;
    
	REAL TimeValue = 0.0;
	int cent = 1;
	TimeValue = cent*deltaT;
    
    ///>>>>>>> Calculo da Integral-J
    ///////////////////J-integral/////////////////////////////////////////////////
    outJ << "J={";
    REAL XcrackTip = -1.;
    TPZGeoMesh * gm = meshvec[0]->Reference();
    int bcfluxOutMat = -20;//ver materialId no arquivo main_elast.cpp
    for(int ell = 0; ell < gm->NElements(); ell++)
    {
        if(gm->ElementVec()[ell] && gm->ElementVec()[ell]->MaterialId() == bcfluxOutMat)
        {
            int nodeIndex = gm->ElementVec()[ell]->NodeIndex(0);
            XcrackTip = gm->NodeVec()[nodeIndex].Coord(0);
            break;
        }
    }
    if(XcrackTip < 1.E-3)
    {
        DebugStop();
    }
    TPZVec<REAL> Origin(3,0.);
    Origin[0] = XcrackTip;
    TPZVec<REAL> normalDirection(3,0.);
    normalDirection[2] = 1.;
    REAL radius = 0.5;
    ////////////////////////////////////////////////////////////////////////////
    
	while (TimeValue <= maxTime) //passo de tempo
	{
        outP << "Saida" << (int)(TimeValue/timeScale) << "={";
        
        //Criando matriz de rigidez (tangente) matK e vetor de carga (residuo)
        StiffMatrixLoadVec(mymaterial, mphysics, an, matK, fres);
        matM->Multiply(InitialSolution,Mass_X_SolTimeN);
        
        REAL res = Norm(fres + Mass_X_SolTimeN);
        REAL tol = 1.e-8;
        int maxit = 15;
        int nit = 0;
        
        res_total = fres + Mass_X_SolTimeN;
        while(res > tol && nit < maxit) //itercao de Newton
        {
            an->Rhs() = res_total;
            an->Solve();
            
            an->LoadSolution(SolIterK + an->Solution());
            
            TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
            
            SolIterK = an->Solution();
            
            StiffMatrixLoadVec(mymaterial, mphysics, an, matK, fres);
            
            res_total = fres + Mass_X_SolTimeN;
            //res_total.Print();
            res = Norm(res_total);
            nit++;
        }
        
        REAL pfracMedio = SaidaMathPressao(meshvec, mphysics, outP);
        outP << "};\n";
        
        mymaterial->UpdateLeakoff(pfracMedio);
        
        std::stringstream outputfiletemp;
        outputfiletemp << outputfile << ".vtk";
        std::string plotfile = outputfiletemp.str();
        PosProcessMult(meshvec,mphysics,an,plotfile);
        
        meshvec[0]->LoadReferences();
        
        ///>>>>>>> Calculo da Integral-J
        ///////////////////J-integral/////////////////////////////////////////////////
        TPZVec<REAL> xx(3,0.), qsii(2,0.);
        xx[0] = XcrackTip - radius;
        int initialEl = 0;
        TPZGeoEl * geoEl = (meshvec[0])->Reference()->FindElement(xx, qsii, initialEl, 2);
        if(!geoEl)
        {
            DebugStop();
        }
        TPZCompEl * compEl = geoEl->Reference();

        TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
        TPZMaterialData data;
        intpEl->InitMaterialData(data);

        intpEl->ComputeShape(qsii, data);
        intpEl->ComputeSolution(qsii, data);

        TPZFMatrix<REAL> Sigma(2,2);
        Sigma.Zero();

        TPZElasticityMaterial * elast2D = dynamic_cast<TPZElasticityMaterial *>(compEl->Material());

        TPZVec<REAL> Solout(3);
        int var;

        var = 10;//Stress Tensor
        elast2D->Solution(data, var, Solout);
        Sigma(0,0) = Solout[0];
        Sigma(1,1) = Solout[1];
        Sigma(0,1) = Solout[2];
        Sigma(1,0) = Solout[2];

        REAL SigmaYY = Sigma(1,1);

        REAL pressure = -SigmaYY;
        Path2D * Jpath = new Path2D(meshvec[0], Origin, normalDirection, radius, pressure);
        JIntegral2D integralJ;
        integralJ.PushBackPath2D(Jpath);
        TPZVec<REAL> KI(2,0.);
        KI = integralJ.IntegratePath2D(0);
        
        if(cent != 1)
        {
            outJ << ",";
        }
        outJ << "{" << TimeValue << "," << KI[0] << "}";
        /////////////////////////////

        TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
        PlotWIntegral(meshvec[0], outW, TimeValue);
        
        InitialSolution = mphysics->Solution();
        cent++;
        TimeValue = cent*deltaT;
    }
    
    outW << "Qinj=" << fabs(mymaterial->Qinj()) << ";\n\n";
    outW << "TrapArea[displ_]:=Block[{displSize, area, baseMin, baseMax, h},\n";
    outW << "displSize = Length[displ];\n";
    outW << "area = 0;\n";
    outW << "For[i = 1, i < displSize,\n";
    outW << "baseMin = displ[[i, 2]];\n";
    outW << "baseMax = displ[[i + 1, 2]];\n";
    outW << "h = displ[[i + 1, 1]] - displ[[i, 1]];\n";
    outW << "area += (baseMin + baseMax)/2.*h;\n";
    outW << "i++;];\n";
    outW << "area];\n\n";
    outW << "Areas = {";
    int nsteps = maxTime/deltaT;
    for(int ig = 1; ig<=nsteps; ig++)
    {
        outW << "{" << ig*deltaT << ",TrapArea[displ" << (int)(ig*deltaT/timeScale) << "]}";
        if(ig != nsteps)
        {
            outW << ",";
        }
    }
    outW << "};\n\n";
    outW << "WintegralPlot =ListPlot[Areas, AxesOrigin -> {0, 0},PlotStyle -> {PointSize[0.01]},AxesLabel->{\"t\",\"V\"},Filling->Axis];\n";
    outW << "vInj[t_]=Qinj*t;\n";
    outW << "vfiltrado[t_]=" << mymaterial->Lf() << "*(Integrate[2*(" << mymaterial->Cl() << "/Sqrt[tau]*Sqrt[((press[tau]+" << mymaterial->SigmaConf() << "-" << mymaterial->Pe() << ")/" << mymaterial->Pref() << ")]),{tau,0,t}]);\n";
    outW << "vf[t_]=vInj[t]-vfiltrado[t];\n";
    outW << "QinjPlot = Plot[vf[t], {t, 1, " << maxTime << "}, PlotStyle -> Red,AxesOrigin->{0,0}];\n";
    outW << "Show[QinjPlot,WintegralPlot]\n";
    
    
    //saida para mathematica
    outP << "SAIDAS={";
    for(int i = 1; i < cent; i++)
    {
        outP << "Saida" << (int)(i*deltaT/timeScale);
        if(i < cent-1)
        {
            outP << ",";
        }
    }
    outP << "};\n\n";
    
    outP << "minx=Min[Transpose[Flatten[SAIDAS,1]][[1]]];\n";
    outP << "maxx=Max[Transpose[Flatten[SAIDAS,1]][[1]]];\n";
    outP << "miny=Min[Transpose[Flatten[SAIDAS,1]][[2]]];\n";
    outP << "maxy=Max[Transpose[Flatten[SAIDAS,1]][[2]]];\n\n";
    
    outP << "Manipulate[ListPlot[SAIDAS[[n]],Joined->True,AxesOrigin->{0,0},PlotRange->{{0,maxx},{0,maxy}},AxesLabel->{\"pos\",\"p\"}],{n,1,Length[SAIDAS],1}]\n\n";
    outP << "TimePressVec = {};\n";
    outP << "For[pos = 1, pos <= Length[SAIDAS], pos++,\n";
    outP << "AppendTo[TimePressVec, {pos*" << deltaT << ",(SAIDAS[[pos]])[[Round[Length[SAIDAS[[pos]]]/2], 2]]}];\n];\n";
    outP << "ListPlot[TimePressVec, Joined -> True,AxesLabel->{\"t\",\"p\"},AxesOrigin->{0,0}]\n";
    outP << "press = Interpolation[TimePressVec,InterpolationOrder->0];\n";
    
    
    outJ << "};\n";
    outJ << "ListPlot[J, Joined -> True,AxesLabel->{\"t\",\"J\"},AxesOrigin->{0,0},Filling->Axis]\n";
    
    outPW << "(*** PRESSAO ***)\n" << outP.str() << "\n\n\n\n\n(*** W ***)\n" << outW.str() << "\n\n\n\n\n(*** J ***)\n" << outJ.str();
    outPW.close();
}

void ToolsTransient::PlotWIntegral(TPZCompMesh *cmesh, std::stringstream & outW, int solNum)
{
    TPZCompMeshReferred * cmeshref = dynamic_cast<TPZCompMeshReferred*>(cmesh);
    outW << "displ" << (int)(solNum/timeScale) << "={";
    int npts = 1;
    
    bool isFirstTime = true;
    for(int el = 0; el < cmeshref->NElements(); el++)
    {
        TPZCompEl *cel = cmeshref->ElementVec()[el];
        if(!cel) continue;
        TPZGeoEl * gel1D = cel->Reference();
        int crak1DMatId = 2;
        if(!gel1D || gel1D->HasSubElement() || gel1D->Dimension() != 1 || gel1D->MaterialId() != crak1DMatId)
        {
            continue;
        }
        
        {
            TPZVec<REAL> qsi1D(1,0.), qsi2D(2,0.), XX(3,0.);
            
            for(int p = -npts; p <= +npts; p++)
            {
                if(isFirstTime == false && p == -npts)
                {
                    continue;
                }
                qsi1D[0] = double(p)/npts;
                gel1D->X(qsi1D, XX);
                int inilIndex = 0;
                TPZGeoEl * gel2D = cmesh->Reference()->FindElement(XX, qsi2D, inilIndex, 2);
                
                TPZElasticityMaterial * elast2D = dynamic_cast<TPZElasticityMaterial *>(gel2D->Reference()->Material());
                
                TPZVec<REAL> Solout(3);
                
                int var = 0;
                TPZReducedSpace * intpEl = dynamic_cast<TPZReducedSpace *>(gel2D->Reference());
                TPZMaterialData data;
                intpEl->InitMaterialData(data);
                
                intpEl->ComputeShape(qsi2D, data);
                intpEl->ComputeSolution(qsi2D, data);
                elast2D->Solution(data, var, Solout);
                
                REAL posX = XX[0];
                REAL posY = 2.*(Solout[1]);
                if(fabs(posX) < 1.E-5)
                {
                    posX = 0;
                }
                if(fabs(posY) < 1.E-5)
                {
                    posY = 0;
                }
                if(isFirstTime)
                {
                    isFirstTime = false;
                }
                else
                {
                    outW << ",";
                }
                outW << "{" << posX << "," << posY << "}";
            }
        }//<<<
    }
    outW << "};\n";
}

REAL ToolsTransient::SaidaMathPressao(TPZVec<TPZCompMesh *> meshvec, TPZCompMesh* mphysics, std::stringstream & outP)
{
    TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
    
    std::map<REAL,REAL> time_pressure;
    
    for(int i = 0;  i< meshvec[1]->ElementVec().NElements(); i++)
    {
        TPZCompEl * cel = meshvec[1]->ElementVec()[i];
        TPZInterpolatedElement * sp = dynamic_cast <TPZInterpolatedElement*>(cel);
        if(!sp) continue;
        TPZVec<REAL> qsi(1,0.),out(3,0.);
        TPZMaterialData data;
        sp->InitMaterialData(data);
        
        qsi[0] = -1.;
        sp->ComputeShape(qsi, data);
        sp->ComputeSolution(qsi, data);
        TPZVec<REAL> SolP = data.sol[0];
        cel->Reference()->X(qsi,out);
        REAL pos = out[0];
        REAL press = data.sol[0][0];
        time_pressure[pos] = press;

        if(out[0] > 50.) continue;
    }
    
    int sz = time_pressure.size();
    int middle = sz/2, posCount = 0;
    REAL pressMiddle = -1.;
    if(middle == 0)
    {
        DebugStop();
    }
        
    std::map<REAL,REAL>::iterator it, itaux;
    for(it = time_pressure.begin(); it != time_pressure.end(); it++)
    {
        itaux = it;
        itaux++;
        REAL pos = it->first;
        REAL press = it->second;
        outP << "{" << pos << "," << press << "}";
        if(itaux != time_pressure.end())
        {
            outP << ",";
        }
        if(posCount == middle)
        {
            pressMiddle = press;
        }
        posCount++;
    }
    
    return pressMiddle;
}


void ToolsTransient::PosProcessMult(TPZVec<TPZCompMesh *> meshvec, TPZCompMesh* mphysics, TPZAnalysis *an, std::string plotfile)
{
    //TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
	TPZManVector<std::string,10> scalnames(5), vecnames(1);
	
	scalnames[0] = "DisplacementX";
	scalnames[1] = "DisplacementY";
    scalnames[2] = "SigmaX";
    scalnames[3] = "SigmaY";
    scalnames[4] = "Pressure";
    vecnames[0] = "Displacement";
	
	const int dim = 2;
	int div =0;
	an->DefineGraphMesh(dim,scalnames,vecnames,plotfile);
	an->PostProcess(div);
}

TPZFMatrix<REAL> ToolsTransient::SetSolution(TPZGeoMesh *gmesh, TPZCompMesh *cmesh, int pOrder, int matId, REAL valIni){
    
    TPZAnalysis an(cmesh);
    int dim = cmesh->Dimension();
	int nrs = an.Solution().Rows();
    TPZVec<REAL> loadvec(nrs,valIni);
    TPZCompMesh  * cmesh_projL2 = ToolsTransient::CMeshProjectionL2(gmesh,dim, matId, pOrder, loadvec);
    TPZAnalysis anL2(cmesh_projL2);
    
    //Solve
	TPZSkylineStructMatrix full(cmesh_projL2);
	anL2.SetStructuralMatrix(full);
	TPZStepSolver<REAL> step;
	step.SetDirect(ELDLt);
	anL2.SetSolver(step);
	anL2.Run();
    
    TPZFMatrix<REAL> InitSol;
    InitSol = anL2.Solution();
    
    return InitSol;
}
