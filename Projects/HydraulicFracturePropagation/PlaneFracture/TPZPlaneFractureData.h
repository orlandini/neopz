//
//  pznlfluidstructureData.h
//  PZ
//
//  Created by Cesar Lucci on 05/06/13.
//
//

#ifndef PZ_TPZPlaneFractureData_h
#define PZ_TPZPlaneFractureData_h

#include <set>
#include <math.h>
#include "pzerror.h"

////////// Materials //////////////////////////////

//std::set<int> globReservoirMatId;

//int const globReservMatId1   = 1; //elastic
//int const globReservMatId2   = 2; //elastic
//int const globPressureMatId = 3; //pressure
//int const globMultiFisicMatId1 = 1;//multiphisics
//int const globMultiFisicMatId2 = 2;//multiphisics
//
//int const globDirichletElastMatId1 = -1;
//int const globDirichletElastMatId2 = -2;
//int const globMixedElastMatId   = -3;
//
//int const globBCfluxIn  = -10; //bc pressure
//int const globCracktip = -20; //bc pressure
//
//int const typeDirichlet = 0;
//int const typeNeumann   = 1;
//int const typeMixed     = 2;
//
//int const typeDir_elast     = 0;
//int const typeNeum_pressure = 2;
//int const typeMix_elast     = 3;

///////////////////////////////////////////////////


/**
 * To understand what is implemented here, see table (MaterialIds Table.xls) on folder (/NeoPZ/Projects/HydraulicFracturePropagation/PlaneFracture)
 */
struct MaterialIdGen
{
public:
    
    MaterialIdGen()
    {
        
    }
    
    ~MaterialIdGen()
    {
        
    }
    
    int Aux1DMatId()
    {
        return -1;
    }
    
    int CrackTipMatId()
    {
        return -2;
    }
    
    int RockMatId(int layer)
    {
#ifdef DEBUG
        if(layer < 0 || layer > 99)
        {
            //Soh pode ter 100 camadas (de 0 a 99)
            DebugStop();
        }
#endif
        
        return (layer+1)*10;
    }
    
    int BulletMatId(int layer)
    {
        return -RockMatId(layer);
    }
    
    int InsideFractMatId(int layer, int stripe)
    {
#ifdef DEBUG
        if(stripe < 0 || stripe > 9)
        {
            //Soh pode ter 10 faixas de pressao (de 0 a 9)
            DebugStop();
        }
#endif
        
        return -(RockMatId(layer) + 1000 + stripe);
    }
    
    int OutSideFractMatId(int layer)
    {
        return -(RockMatId(layer) + 2000);
    }
    
    int FarfieldMatId(int layer)
    {
        return -(RockMatId(layer) + 3000);
    }
    
    int LeftMatId(int layer)
    {
        return -(RockMatId(layer) + 4000);
    }
    
    int RightMatId(int layer)
    {
        return -(RockMatId(layer) + 5000);
    }
    
    int TopMatId()
    {
        return -6010;
    }
    
    int BottomMatId()
    {
        return -7010;
    }
    
    bool IsInsideFractMat(int matId)
    {
        if(matId <= -1010 && matId >= -2009)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    bool IsOutsideFractMat(int matId)
    {
        if(matId <= -2010 && matId >= -3000)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    bool IsBoundaryMaterial(int matId)
    {
        if(matId <= -3010 && matId >= -7010)
        {//is 2D BC between: left, right, farfield, top or bottom
            return true;
        }
        else
        {
            return false;
        }
    }
    
    bool IsBulletMaterial(int matId)
    {
        if(matId <= -10 && matId >= -1000)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    //------------------------------------------------------------------------------------------------------------
};

#include "pzanalysis.h"
#include "pzreal.h"
#include <map>
#include <fstream>

//class Input3DDataStruct
//{
//public:
//    
//    Input3DDataStruct();
//    ~Input3DDataStruct();
//    
//    void SetData(int NStripes, REAL Visc, REAL SigN, REAL QinjTot, REAL Ttot, REAL maxDeltaT, int nTimes,
//                 REAL Cl, REAL Pe, REAL SigmaConf, REAL Pref, REAL vsp, REAL KIc, REAL Jradius);
//    
//    void SetLf(REAL Lf);
//    
//    int NStripes();
//    std::map< int,std::pair<int,int> > & GetPressureMatIds_StripeId_ElastId();
//    int StripeId(int bcId);
//    int ElastId(int bcId);
//    void InsertBCId_StripeId_ElastId(int BCId, int StripeId, int ElastId);
//    bool IsBC(int matId);
//    
//    REAL Visc();
//    std::map<int,REAL> & GetLeakoffmap();
//    REAL SigN();
//    REAL Qinj();
//    REAL Ttot();
//    REAL actTime();
//    REAL actDeltaT();
//    REAL Cl();
//    REAL Pe();
//    REAL SigmaConf();
//    REAL Pref();
//    REAL vsp();
//    REAL KIc();
//    REAL Jradius();
//    void SetMinDeltaT();
//    void SetNextDeltaT();
//    void UpdateActTime();
//    
//    //Leafoff methods
//    void UpdateLeakoff(TPZCompMesh * cmesh);
//    REAL VlFtau(REAL pfrac, REAL tau);
//    REAL FictitiousTime(REAL VlAcum, REAL pfrac);
//    REAL QlFVl(int gelId, REAL pfrac);
//    REAL dQlFVl(int gelId, REAL pfrac);
//    
//private:
//    
//
//    int fNStripes;//Amounth of pressure stripes for reduced space elastic references
//    std::map< int,std::pair<int,int> > fPressureMatIds_StripeId_ElastId;//Correspondence between MaterialIds of Pressures BCs and < Stripe Number , ElastMatId >
//    
//    //Fluid property:
//    REAL fVisc;//viscosidade do fluido de injecao
//    
//    //Leakoff data
//    std::map<int,REAL> fLeakoffmap;
//    
//    //BCs:
//    REAL fSigN;//Sigma.n no problema elastico que servira de espaco de aproximacao para o elastico multifisico
//    REAL fQinj;//vazao de 1 asa de fratura dividido pela altura da fratura
//    
//    //time:
//    REAL fTtot;//Tempo total da simulacao
//    REAL factTime;//tempo atual (em segundos)
//    REAL fmaxDeltaT;//delta T maximo
//    REAL fminDeltaT;//delta T minimo
//    REAL factDeltaT;//delta T atual
//    int fNDeltaTsteps;//quantidade de incrementos do deltaT para definir o deltaT minimo
//    
//    //Leakoff:
//    REAL fCl;//Carter
//    REAL fPe;//Pressao estatica
//    REAL fSigmaConf;//Tensao de confinamento
//    REAL fPref;//Pressao de referencia da medicao do Cl
//    REAL fvsp;//spurt loss
//    
//    //Propagation criterion
//    REAL fJradius;
//    REAL fKIc;
//};




class Output3DDataStruct
{
public:
    
    Output3DDataStruct();
    ~Output3DDataStruct();
    
    int NTimes();
    void InsertTposP(int time, std::map<REAL,REAL> & posPmap);
    void InsertTposVolLeakoff(int time, REAL pos, REAL Ql);
    void InsertTAcumVolW(int time, REAL vol);
    void InsertTAcumVolLeakoff(int time, REAL vol);
    void InsertTKI(int time, REAL KI);
    void SetQinj1WingAndLfracmax(REAL Qinj1wing, REAL Lfracmax);
    
    void PlotElasticVTK(TPZAnalysis * an, int anCount = -1);
    void PrintMathematica(std::ofstream & outf);
    
    struct posP
    {
    public:
        
        posP()
        {
            fposP.clear();
        }
        ~posP()
        {
            fposP.clear();
        }
        void PrintMathematica(std::ofstream & outf)
        {
#ifdef DEBUG
            if(fposP.size() == 0)
            {
                DebugStop();
            }
#endif
            std::map<REAL,REAL>::iterator itposP;
            std::map<REAL,REAL>::iterator itposPLast = fposP.end();
            itposPLast--;
            
            outf << "{";
            for(itposP = fposP.begin(); itposP != fposP.end(); itposP++)
            {
                outf << "{" << itposP->first << "," << itposP->second << "}";
                if(itposP != itposPLast)
                {
                    outf << ",";
                }
            }
            outf << "}";
        }
        
        std::map<REAL,REAL> fposP;
    };
    
    struct posVolLeakoff
    {
    public:
        posVolLeakoff()
        {
            fposVolLeakoff.clear();
        }
        ~posVolLeakoff()
        {
            fposVolLeakoff.clear();
        }
        void InsertPoint(REAL pos, REAL Ql)
        {
            fposVolLeakoff[pos] = Ql;
        }
        void PrintMathematica(std::ofstream & outf)
        {
#ifdef DEBUG
            if(fposVolLeakoff.size() == 0)
            {
                DebugStop();
            }
#endif
            std::map<REAL,REAL>::iterator itposVolLeakoff;
            std::map<REAL,REAL>::iterator itposVolLeakoffLast = fposVolLeakoff.end();
            itposVolLeakoffLast--;
            
            outf << "{";
            for(itposVolLeakoff = fposVolLeakoff.begin(); itposVolLeakoff != fposVolLeakoff.end(); itposVolLeakoff++)
            {
                outf << "{" << itposVolLeakoff->first << "," << itposVolLeakoff->second << "}";
                if(itposVolLeakoff != itposVolLeakoffLast)
                {
                    outf << ",";
                }
            }
            outf << "}";
        }
        
        std::map<REAL,REAL> fposVolLeakoff;
    };
    
    //maps indexed by time
    std::map<int,posP> fTposP;
    std::map<int,posVolLeakoff> fTposVolLeakoff;
    std::map<int,REAL> fTAcumVolW;
    std::map<int,REAL> fTAcumVolLeakoff;
    std::map<int,REAL> fTKI;
    REAL fQinj1wing;
    REAL fLfracMax;
};

extern MaterialIdGen globMaterialIdGen;

//extern Input3DDataStruct globFractInput3DData;

extern Output3DDataStruct globFractOutput3DData;

#endif
