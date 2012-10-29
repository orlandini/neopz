//
//  pznlfluidstructure2d.cpp
//  PZ
//
//  Created by Agnaldo Farias on 9/17/12.
//
//

#include "pznlfluidstructure2d.h"


#include <iostream>
#include <string>

#include "pzelasmat.h"
#include "pzbndcond.h"
#include "pzaxestools.h"

#include "pzlog.h"
#ifdef LOG4CXX
static LoggerPtr logdata(Logger::getLogger("pz.material.elastpressure"));
#endif

TPZNLFluidStructure2d::EState TPZNLFluidStructure2d::gState = ECurrentState;

TPZNLFluidStructure2d::TPZNLFluidStructure2d():TPZDiscontinuousGalerkin(){
    
    fmatId = 0;
    fDim = 2;
    fPlaneStress = 1.;
    
	fE = 0.;
    fnu =0.;
    fG =0.;
    fHw=0.;
    
    fvisc = 0.;
    fQL =0.;
     fSigConf =0.;
    
	ff.resize(fDim);
	ff[0]=0.;
	ff[1]=0.;
}

TPZNLFluidStructure2d::TPZNLFluidStructure2d(int matid, int dim):TPZDiscontinuousGalerkin(matid){
    
    fmatId = matid;
    fDim = dim;
    fPlaneStress = 1.;
    
	fE = 0.;
    fnu =0.;
    fG =0.;
    fHw=0.;
    
    fvisc = 0.;
    fQL =0.;
    fSigConf =0.;
    
	ff.resize(fDim);
	ff[0]=0.;
	ff[1]=0.;
}

TPZNLFluidStructure2d::~TPZNLFluidStructure2d(){
}


int TPZNLFluidStructure2d::NStateVariables() {
	return 1;
}

void TPZNLFluidStructure2d::Print(std::ostream &out) {
	out << "name of material : " << Name() << "\n";
	out << "properties : \n";
	out << "\t E   = " << fE   << std::endl;
	out << "\t nu   = " << fnu   << std::endl;
	out << "\t Forcing function F   = " << ff[0] << ' ' << ff[1]   << std::endl;
    out << "altura da fratura fHw "<< fHw << std::endl;
	out << "Viscosidade do fluido fvisc "<< fvisc << std::endl;
	out << "2D problem " << fPlaneStress << std::endl;
	out << "Base Class properties :";
	TPZMaterial::Print(out);
	out << "\n";
}


void TPZNLFluidStructure2d::Contribute(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<REAL> &ek, TPZFMatrix<REAL> &ef){
    
    if(gState == ELastState) return;
    
    int nref =  datavec.size();
	if (nref != 2 ) {
		std::cout << " Error.!! the size of datavec is equal to two\n";
        std::cout << " datavec[0]->elasticity and datavec[1]->pressure\n";
		DebugStop();
	}
    
    TPZMaterialData::MShapeFunctionType shapetype = datavec[0].fShapeType;
    if(shapetype!=datavec[0].EVecShape && datavec[0].phi.Cols()!=0){
        
        std::cout << " The space to elasticity problem must be reduced space.\n";
		DebugStop();
    }
    
    //Calculate the matrix contribution for elastic problem.
    TPZFMatrix<REAL> &dphi_u = datavec[0].dphix;
	TPZFMatrix<REAL> &phi_u = datavec[0].phi;
	TPZFMatrix<REAL> &axes=datavec[0].axes;
    
    TPZManVector<REAL,3> sol_u=datavec[0].sol[0];
	TPZFMatrix<REAL> &dsol_u=datavec[0].dsol[0];
    
	int phcu,efcu;
	phcu = phi_u.Cols();
	efcu = ef.Cols();
	
	if(fForcingFunction) {// phi(in, 0) :  node in associated forcing function
		TPZManVector<STATE> res(3);
		fForcingFunction->Execute(datavec[0].x,res);
		ff[0] = res[0];
		ff[1] = res[1];
		ff[2] = res[2];
	}
	
	TPZFNMatrix<4,STATE> dphix_i(2,1),dphiy_i(2,1), dphix_j(2,1),dphiy_j(2,1), dsolx_j(2,1), dsoly_j(2,1);
	/*
	 * Plain strain materials values
	 */
    REAL fEover1MinNu2 = fE/(1-fnu*fnu);  ///4G(lamb+G)/(lamb+2G)
    REAL fEover21PlusNu = 2.*fE/(2.*(1.+fnu));/*fE/(2.*(1+fnu));*/ ///2G=2mi
	REAL nu1 = 1 - fnu;//(1-nu)
	REAL nu2 = (1.-2.*fnu)/2.;
	REAL F = fE/((1.+fnu)*(1.-2.*fnu));
    
	for(int in = 0; in < phcu; in++) {
		dphix_i(0,0) = dphi_u(0,in)*axes(0,0)+dphi_u(1,in)*axes(1,0);
		dphix_i(1,0) = dphi_u(0,in)*axes(0,1)+dphi_u(1,in)*axes(1,1);
		dphiy_i(0,0) = dphi_u(2,in)*axes(0,0)+dphi_u(3,in)*axes(1,0);
		dphiy_i(1,0) = dphi_u(2,in)*axes(0,1)+dphi_u(3,in)*axes(1,1);
		
        //Residuo
        for (int col = 0; col < efcu; col++)
        {
            //termo f*u
            ef(in,col) += weight*(ff[0]*phi_u(0, in) + ff[1]*phi_u(1, in));
            
            //termos da matriz
            dsolx_j(0,0) = dsol_u(0,0)*axes(0,0)+dsol_u(1,0)*axes(1,0);
            dsolx_j(1,0) = dsol_u(0,0)*axes(0,1)+dsol_u(1,0)*axes(1,1);
            dsoly_j(0,0) = dsol_u(0,1)*axes(0,0)+dsol_u(1,1)*axes(1,0);
            dsoly_j(1,0) = dsol_u(0,1)*axes(0,1)+dsol_u(1,1)*axes(1,1);
            
			
			if (fPlaneStress != 1){/* Plain Strain State */
                ef(in,col) += (-1.)*weight*(nu1*dphix_i(0,0)*dsolx_j(0,0) + nu2*dphix_i(1,0)*dsolx_j(1,0) +
                                     
                                     fnu*dphix_i(0,0)*dsoly_j(1,0) + nu2*dphix_i(1,0)*dsoly_j(0,0) +
                                     
                                     fnu*dphiy_i(1,0)*dsolx_j(0,0) + nu2*dphiy_i(0,0)*dsolx_j(1,0) +
                                     
                                     nu1*dphiy_i(1,0)*dsoly_j(1,0) + nu2*dphiy_i(0,0)*dsoly_j(0,0))*F;
			}
			else{/* Plain stress state */
                ef(in,col) += (-1.)*weight*(fEover1MinNu2*dphix_i(0,0)*dsolx_j(0,0) + fEover21PlusNu*dphix_i(1,0)*dsolx_j(1,0) +
                                     
                                     fEover1MinNu2*dphix_i(0,0)*dsoly_j(1,0) + fEover21PlusNu*dphix_i(1,0)*dsoly_j(0,0) +
                                     
                                     fEover1MinNu2*dphiy_i(1,0)*dsolx_j(0,0) + fEover21PlusNu*dphiy_i(0,0)*dsolx_j(1,0) +
                                     
                                     fEover1MinNu2*dphiy_i(1,0)*dsoly_j(1,0) + fEover21PlusNu*dphiy_i(0,0)*dsoly_j(0,0));
            }
		}//fim para o residuo
        
        //Matriz Tangente (Jacobiana)
        for( int jn = 0; jn < phcu; jn++ ) {
            
            dphix_j(0,0) = dphi_u(0,jn)*axes(0,0)+dphi_u(1,jn)*axes(1,0);
            dphix_j(1,0) = dphi_u(0,jn)*axes(0,1)+dphi_u(1,jn)*axes(1,1);
            dphiy_j(0,0) = dphi_u(2,jn)*axes(0,0)+dphi_u(3,jn)*axes(1,0);
            dphiy_j(1,0) = dphi_u(2,jn)*axes(0,1)+dphi_u(3,jn)*axes(1,1);
			
			
			if (fPlaneStress != 1){/* Plain Strain State */
                ek(in,jn) += weight*(nu1*dphix_i(0,0)*dphix_j(0,0) + nu2*dphix_i(1,0)*dphix_j(1,0) +
                                     
                                     fnu*dphix_i(0,0)*dphiy_j(1,0) + nu2*dphix_i(1,0)*dphiy_j(0,0) +
                                     
                                     fnu*dphiy_i(1,0)*dphix_j(0,0) + nu2*dphiy_i(0,0)*dphix_j(1,0) +
                                     
                                     nu1*dphiy_i(1,0)*dphiy_j(1,0) + nu2*dphiy_i(0,0)*dphiy_j(0,0))*F;
			}
			else{/* Plain stress state */
                ek(in,jn) += weight*(fEover1MinNu2*dphix_i(0,0)*dphix_j(0,0) + fEover21PlusNu*dphix_i(1,0)*dphix_j(1,0) +
                                     
                                     fEover1MinNu2*dphix_i(0,0)*dphiy_j(1,0) + fEover21PlusNu*dphix_i(1,0)*dphiy_j(0,0) +
                                     
                                     fEover1MinNu2*dphiy_i(1,0)*dphix_j(0,0) + fEover21PlusNu*dphiy_i(0,0)*dphix_j(1,0) +
                                     
                                     fEover1MinNu2*dphiy_i(1,0)*dphiy_j(1,0) + fEover21PlusNu*dphiy_i(0,0)*dphiy_j(0,0));
            }
		}//fim para a tangente

	}
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//		std::stringstream sout;
//		ek.Print("ek_reduced = ",sout,EMathematicaInput);
//		ef.Print("ef_reduced = ",sout,EMathematicaInput);
//		LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
    
    // Calculate the matrix contribution for pressure
    ContributePressure(datavec, weight, ek, ef);
}

void TPZNLFluidStructure2d::ContributePressure(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<REAL> &ek, TPZFMatrix<REAL> &ef){
        
    if(!datavec[1].phi) return;
    TPZFMatrix<REAL>  &phi_p =  datavec[1].phi;
    TPZFMatrix<REAL>  &dphi_p =  datavec[1].dphix;
    TPZManVector<REAL,3> sol_p=datavec[1].sol[0];
    TPZFMatrix<REAL> &dsol_p=datavec[1].dsol[0];
    int phrp = phi_p.Rows();
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
    TPZManVector<REAL,3> sol_u=datavec[0].sol[0];
    int phcu = phi_u.Cols();
    
    REAL sol_un = sol_u[0]*datavec[0].normal[0] + sol_u[1]*datavec[0].normal[1];
    
    
	if(gState == ECurrentState) //current state (n+1): Matrix stiffnes
    {
            
        //---- CASO 1 ------
//        for(int in = 0; in<phrp; in++){
//            
//            //Residuo
//            ef(in+phcu,0)+=(-1.)*(fQL)*phi_p(in,0)*weight;
//            
//            ef(in+phcu,0)+=(-1.)*weight*dphi_p(0,in)*dsol_p(0,0);
//            
//            ef(in+phcu,0)+=(-1.)*weight*phi_p(in,0)*sol_p[0]/fTimeStep;
//            
//            //Tangente
//            for(int jn=0; jn<phrp; jn++){
//                ek(in+phcu, jn+phcu)+= weight*dphi_p(0,in)*dphi_p(0,jn);
//                
//                ek(in+phcu, jn+phcu)+= weight*phi_p(in,0)*phi_p(jn,0)/fTimeStep;
//            }
//        }
        
        //---- CASO 2 e 3------
        for(int in = 0; in<phrp; in++){
            
            //Residuo
            ef(in+phcu,0)+=(-1.)*(fQL)*phi_p(in,0)*weight;
            
            ef(in+phcu,0)+=(-1.)*weight*dphi_p(0,in)*dsol_p(0,0);
            
            ef(in+phcu,0)+=(-1.)*weight*phi_p(in,0)*sol_un/fTimeStep;
            
            //Tangente
            for(int jn=0; jn<phrp; jn++){
                ek(in+phcu, jn+phcu)+= weight*dphi_p(0,in)*dphi_p(0,jn);
            }
            
            for(int jn=0; jn<phcu; jn++){
                
                ek(in+phcu, jn)+=phi_p(in,0)*(phi_u(0, jn)*datavec[0].normal[0] + phi_u(1, jn)*datavec[0].normal[1])*weight/fTimeStep;
            }
        }
        
        //CASO 4
//        REAL factor_un3 = (sol_un*sol_un*sol_un)/(12.*fvisc);
//        REAL factor_un2 = (sol_un*sol_un)/(4.*fvisc);
//        
//        for(int in = 0; in<phrp; in++)
//        {
//            //----Residuo----
//            //termo Ql*v
//            ef(in+phcu,0)+=(-1.)*(fQL)*phi_p(in,0)*weight;
//            
//            //termo (unˆ3/12*mi)*(dv/dx)*(dp/dx)
//            ef(in+phcu,0)+=(-1.)*weight*factor_un3*dphi_p(0,in)*dsol_p(0,0);
//            
//            //termo un*v/deltaT
//            ef(in+phcu,0)+=(-1.)*weight*phi_p(in,0)*sol_un/fTimeStep;
//
//            
//            //------Matriz tangente-----
//            //termo (phip_i)*(phiun_j)/deltaT
//            for(int jn=0; jn<phcu; jn++){
//                
//                ek(in+phcu, jn)+=phi_p(in,0)*(phi_u(0, jn)*datavec[0].normal[0] + phi_u(1, jn)*datavec[0].normal[1])*weight/fTimeStep;
//            }
//            
//            //termo (unˆ2/4*mi)*(dp/dx)*(dphip_i)*(phiun_j)
//             for(int jn=0; jn<phcu; jn++){
//                 
//                 ek(in+phcu, jn)+= weight*factor_un2*dsol_p(0,0)*dphi_p(0,in)*(phi_u(0, jn)*datavec[0].normal[0] +
//                                                                               phi_u(1, jn)*datavec[0].normal[1]);
//             }
//            
//            //termo (unˆ3/12*mi)*(dphip_i)*(dphip_j)
//            for(int jn=0; jn<phrp; jn++){
//                
//                ek(in+phcu, jn+phcu)+=factor_un3*dphi_p(0,in)*dphi_p(0,jn)*weight;
//                
//            }
//        }
    }
    
    //Last state (n): Matrix mass
    //termo (phip_i)*(phiu_j)/deltaT
	if(gState == ELastState)
    {
        for(int in = 0; in<phrp; in++){
            
            for(int jn=0; jn<phcu; jn++){
                
                ek(in+phcu, jn)+=phi_p(in,0)*(phi_u(0, jn)*datavec[0].normal[0] + phi_u(1, jn)*datavec[0].normal[1])*weight/fTimeStep;
            }
        }
    }
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//		std::stringstream sout;
//		ek.Print("ek_reduced = ",sout,EMathematicaInput);
//		ef.Print("ef_reduced = ",sout,EMathematicaInput);
//		LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
}

void TPZNLFluidStructure2d::ApplyDirichlet_U(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek,TPZFMatrix<> &ef,TPZBndCond &bc){
    
    //  if(gState == ELastState) return;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
    TPZManVector<REAL,3> sol_u = datavec[0].sol[0];
	const REAL big  = TPZMaterial::gBigNumber;
	int phc = phi_u.Cols();
    
    for(int in = 0 ; in < phc; in++) {
        for (int il = 0; il <fNumLoadCases; il++)
        {
            //termo big*u*v do vetor de carga
            TPZFNMatrix<3,STATE> v2 = bc.Val2(il);
            ef(in,il) += big*(v2(0,il)*phi_u(0,in) + v2(1,il)*phi_u(1,in))*weight;
            
            //termo big*u*v da matriz
            ef(in,il) += (-1.)*big*(phi_u(0,in)*sol_u[0] + phi_u(1,in)*sol_u[1])*weight;
        }
        
        for (int jn = 0; jn < phc; jn++) {
            
            ek(in,jn) += big*(phi_u(0,in)*phi_u(0,jn) + phi_u(1,in)*phi_u(1,jn))*weight;
        }
    }
    

}

void TPZNLFluidStructure2d::ApplyNeumann_U(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek, TPZFMatrix<> &ef,TPZBndCond &bc){
    
    if(gState == ELastState) return;
    REAL auxvar = 0.817*(1-fnu)*fHw;
    REAL factor =0.;//fG/auxvar;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
    TPZManVector<REAL,3> sol_u = datavec[0].sol[0];
    
    REAL sol_un = sol_u[0]*datavec[0].normal[0] + sol_u[1]*datavec[0].normal[1];
	int nc_u = phi_u.Cols();
    
    TPZFMatrix<REAL> &phi_p = datavec[1].phi;
    TPZManVector<REAL,3> sol_p = datavec[1].sol[0];
    int phrp = phi_p.Rows();

    //----- CASO 1 e 2 -----
//    for (int in = 0; in < nc_u; in++){
//        for (int il = 0; il <fNumLoadCases; il++){
//            
//            TPZFNMatrix<3,STATE> v2 = bc.Val2(il);
//            ef(in,il)+= weight*(v2(0,il)*phi_u(0,in) + v2(1,il)*phi_u(1,in));
//        }
//    }
    
    //------ CASO 3 e 4 -----
    for (int in = 0; in < nc_u; in++){
        
        for (int il = 0; il <fNumLoadCases; il++){
            
            TPZFNMatrix<3,STATE> v2 = bc.Val2(il);
            ef(in,il)+= (-1.)*weight*factor*(phi_u(0,in)*sol_un*datavec[0].normal[0] + phi_u(1,in)*sol_un*datavec[0].normal[1])
                        
                        + weight*(phi_u(0,in)*sol_p[0]*datavec[0].normal[0] + phi_u(1,in)*sol_p[0]*datavec[0].normal[1]);
            
        }
        
        //matriz tangente
        for (int jn = 0; jn <nc_u; jn++) {
            
            ek(in,jn) += weight*factor*(phi_u(0,in)*phi_u(0,jn)*datavec[0].normal[0] + phi_u(1,in)*phi_u(1,jn)*datavec[0].normal[1]);
        }
        
        for (int jp = 0; jp <phrp; jp++) {
            
            ek(in,jp+nc_u) += (-1.)*weight*(phi_u(0,in)*phi_p(jp,0)*datavec[0].normal[0] + phi_u(1,in)*phi_p(jp,0)*datavec[0].normal[1]);
        }
    }
    
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//		std::stringstream sout;
//		ek.Print("ek_reduced = ",sout,EMathematicaInput);
//		ef.Print("ef_reduced = ",sout,EMathematicaInput);
//		LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
    
}

void TPZNLFluidStructure2d::ApplyMixed_U(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek, TPZFMatrix<> &ef, TPZBndCond &bc){
    
    if(gState == ELastState) return;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
    TPZManVector<REAL,3> sol_u =datavec[0].sol[0];
	int phc = phi_u.Cols();
    
    for(int in = 0 ; in < phc; in++)
    {
        for (int il = 0; il <fNumLoadCases; il++)
        {
            TPZFNMatrix<3,STATE> v2 = bc.Val2(il);
            ef(in,il)+= weight*(v2(0,il)*phi_u(0,in) + v2(1,il)*phi_u(1,in));
            
            //termo da matriz
            ef(in,il) += (-1.)*(bc.Val1()(0,0)*phi_u(0,in)*sol_u[0]*weight
            
            + bc.Val1()(1,0)*phi_u(1,in)*sol_u[0]*weight
            
            + bc.Val1()(0,1)*phi_u(0,in)*sol_u[1]*weight
            
            + bc.Val1()(1,1)*phi_u(1,in)*sol_u[1]*weight);
        }
        
        for (int jn = 0; jn <phc; jn++) {
            
            ek(in,jn) += bc.Val1()(0,0)*phi_u(0,in)*phi_u(0,jn)*weight
            
            + bc.Val1()(1,0)*phi_u(1,in)*phi_u(0,jn)*weight
            
            + bc.Val1()(0,1)*phi_u(0,in)*phi_u(1,jn)*weight
            
            + bc.Val1()(1,1)*phi_u(1,in)*phi_u(1,jn)*weight;
        }
    }
}

void TPZNLFluidStructure2d::ApplyDirichlet_P(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek, TPZFMatrix<> &ef, TPZBndCond &bc){
    
    if(gState == ELastState) return;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
	int c_u = phi_u.Cols();
    
    TPZFMatrix<REAL> &phi_p = datavec[1].phi;
    TPZManVector<REAL,3> sol_p = datavec[1].sol[0];
    int phrp = phi_p.Rows();
    
    for(int in = 0; in<phrp; in++){
        //termo do vetor de carga
        ef(in+c_u,0)+=gBigNumber*bc.Val2()(2,0)*phi_p(in,0)*weight;
        
        //termo da matriz
        ef(in+c_u,0)+=(-1.)*gBigNumber*sol_p[0]*phi_p(in,0)*weight;
        
        for(int jn = 0; jn<phrp; jn++){
            ek(in+c_u,jn+c_u)+=gBigNumber*phi_p(in,0)*phi_p(jn,0)*weight;
        }
    }
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//		std::stringstream sout;
//		phi_p.Print("phi = ",sout,EMathematicaInput);
//        bc.Val2().Print("val2 = ",sout);
//        sout << datavec[1].sol[0][0];
//		LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
}

void TPZNLFluidStructure2d::ApplyNeumann_P(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek, TPZFMatrix<> &ef, TPZBndCond &bc){
    
    if(gState == ELastState) return;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
	int c_u = phi_u.Cols();
    TPZFMatrix<REAL> &phi_p = datavec[1].phi;
    int  phrp = phi_p.Rows();

    for(int in=0; in<phrp; in++){
        ef(in+c_u,0) += (-1.)*bc.Val2()(2,0)*phi_p(in,0)*weight;
    }
}

void TPZNLFluidStructure2d::ApplyMixed_P(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<> &ek,TPZFMatrix<> &ef,TPZBndCond &bc){
    
    if(gState == ELastState) return;
    
    TPZFMatrix<REAL> &phi_u = datavec[0].phi;
	int c_u = phi_u.Cols();
    
    TPZFMatrix<REAL> &phi_p = datavec[1].phi;
    TPZManVector<REAL,3> sol_p = datavec[1].sol[0];
    int phrp = phi_p.Rows();
    
    for(int in=0; in<phrp; in++){
        //termo do vetor de carga
        ef(in+c_u,0)+=bc.Val2()(2,0)*phi_p(in,0)*weight;
        
        //termo da matriz
        ef(in+c_u,0) += (-1.)*bc.Val1()(2,0)*phi_p(in,0)*sol_p[0]*weight;
        
        for(int jn = 0; jn<phrp; jn++){
            ek(in+c_u,jn+c_u) += bc.Val1()(2,0)*phi_p(in,0)*phi_p(jn,0)*weight;
        }
    }
}

void TPZNLFluidStructure2d::ContributeBC(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc){
    
    TPZMaterialData::MShapeFunctionType shapetype = datavec[0].fShapeType;
    if(shapetype!=datavec[0].EVecShape && datavec[0].phi.Cols()!=0){
        
        std::cout << " The space to elasticity problem must be reduced space.\n";
		DebugStop();
    }
	
	switch (bc.Type()) {
		case 0 :			// Dirichlet condition in  both elasticity and pressure equations
			
            ApplyDirichlet_U(datavec, weight, ek,ef,bc);
            ApplyDirichlet_P(datavec, weight, ek,ef,bc);
            break;
			
		case 1 :			// Neumann condition in  both elasticity and pressure equations
            ApplyNeumann_U(datavec, weight,ek,ef,bc);
            ContributePressure(datavec, weight, ek, ef);
            break;
			
		case 2 :            // Mixed condition in  both elasticity and pressure equations
            ApplyMixed_U(datavec, weight, ek,ef,bc);
            ApplyMixed_P(datavec, weight, ek,ef,bc);
            break;
            
        case 10:        // Neumann condition only on the elasticity equation
        {
            // Calculate the matrix contribution for pressure
            ApplyNeumann_U(datavec, weight,ek, ef,bc);
            break;
        }
            
        case 20:        // Mixed condition only on the elasticity equation
            ApplyMixed_U(datavec, weight, ek,ef,bc);
            break;
            
        case 21:        // Neumann condition only on the pressure equation pressure
            ApplyNeumann_P(datavec, weight, ek,ef,bc);
            break;
            
        case 22:        // Dirichlet condition only on the pressure equation pressure
            ApplyDirichlet_P(datavec, weight, ek,ef,bc);
            break;
            
    }
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//		std::stringstream sout;
//		ek.Print("ek_reduced = ",sout,EMathematicaInput);
//		ef.Print("ef_reduced = ",sout,EMathematicaInput);
//		LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
    
}

int TPZNLFluidStructure2d::VariableIndex(const std::string &name){
    if(!strcmp("Pressure",name.c_str()))        return  1;
    if(!strcmp("MinusKGradP",name.c_str()))     return  2;
    if(!strcmp("DisplacementX",name.c_str()))  return 3;
	if(!strcmp("DisplacementY",name.c_str()))  return 4;
    if(!strcmp("SigmaX",name.c_str()))        return  5;
	if(!strcmp("SigmaY",name.c_str()))        return  6;
    
    return TPZMaterial::VariableIndex(name);
}

int TPZNLFluidStructure2d::NSolutionVariables(int var){
    if(var == 1) return 1;
    if(var == 2) return 1;
    if(var == 3) return 1;
    if(var == 4) return 1;
    if(var == 5) return 1;
    if(var == 6) return 1;
    return TPZMaterial::NSolutionVariables(var);
}


void TPZNLFluidStructure2d::Solution(TPZVec<TPZMaterialData> &datavec, int var, TPZVec<REAL> &Solout){
    
    Solout.Resize(this->NSolutionVariables(var));
    
    TPZVec<REAL> SolP, SolU;
	TPZFMatrix<> DSolP, DSolU;
	TPZFMatrix<> axesP, axesU;
    SolP=datavec[1].sol[0];
	DSolP=datavec[1].dsol[0];
    axesP=datavec[1].axes;
    
    SolU=datavec[0].sol[0];
    DSolU=datavec[0].dsol[0];
    axesU=datavec[0].axes;
    
    if(var == 1)
    {
        if(!datavec[1].phi) return;
		Solout[0] = SolP[0];
		return;
	}//var1
    
    if (var == 2)
    {
        if(!datavec[1].phi) return;
        REAL un = 0.817*(1-fnu)*(SolP[0]-fSigConf)*fHw/fG;
        REAL factor = (un*un*un)/(12.*fvisc);
        
		int id;
		TPZFNMatrix<9,REAL> dsoldx;
		TPZAxesTools<REAL>::Axes2XYZ(DSolP, dsoldx, axesP);
		for(id=0 ; id<1; id++){
			Solout[id] = -1.*factor*dsoldx(id,0);
		}
		return;
	}//var2
    
    //function (state variable ux)
	if(var == 3)
    {
		Solout[0] = SolU[0];
		return;
	}//var3
	
	//function (state variable uy)
	if(var == 4)
    {
		Solout[0] = SolU[1];
		return;
	}//var4
    
    
    REAL DSolxy[2][2];
    REAL SigX, SigY, epsx, epsy;
    
    DSolxy[0][0] = DSolU(0,0)*axesU(0,0)+DSolU(1,0)*axesU(1,0);
    DSolxy[1][1] = DSolU(0,1)*axesU(0,1)+DSolU(1,1)*axesU(1,1);
//    epsx = DSolxy[0][0];// du/dx
//	epsy = DSolxy[1][1];// dv/dy
    
    epsx = DSolU(0,0);// du/dx
    epsy = DSolU(1,1);// dv/dy
	REAL Gmodule = fE/(1-fnu*fnu);
	
    
	if (this->fPlaneStress){
		SigX = Gmodule*(epsx+fnu*epsy);
		SigY = Gmodule*(fnu*epsx+epsy);
	}
	else{
		SigX = fE/((1.-2.*fnu)*(1.+fnu))*((1.-fnu)*epsx+fnu*epsy);
		SigY = fE/((1.-2.*fnu)*(1.+fnu))*(fnu*epsx+(1.-fnu)*epsy);
	}
    
    if(var == 5) {
		Solout[0] = SigX;
		return;
	}//var5
	
	if(var == 6) {
		Solout[0] = SigY;
		return;
	}//var6
    

}

void TPZNLFluidStructure2d::FillDataRequirements(TPZVec<TPZMaterialData > &datavec)
{
	int nref = datavec.size();
	for(int i = 0; i<nref; i++)
	{
		datavec[i].SetAllRequirements(false);
        datavec[i].fNeedsSol = true;
		datavec[i].fNeedsNeighborSol = false;
		datavec[i].fNeedsNeighborCenter = false;
		datavec[i].fNeedsNormal = true;
	}
}

void TPZNLFluidStructure2d::FillBoundaryConditionDataRequirement(int type,TPZVec<TPZMaterialData > &datavec){
    int nref = datavec.size();
	for(int i = 0; i<nref; i++)
	{
        datavec[i].fNeedsSol = true;
		datavec[i].fNeedsNormal = true;
	}
}