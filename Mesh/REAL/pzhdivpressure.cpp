/**
 * @file
 * @brief Contains the implementation of the TPZCompElHDivPressure methods.
 */

#include "pzcmesh.h"
#include "pzhdivpressure.h"
#include "pzquad.h"
#include "pzgeoel.h"
#include "pzmaterial.h"
#include "pzlog.h"
#include "pzgeoquad.h"
#include "TPZShapeDisc.h"
#include "TPZCompElDisc.h"
#include "pzmaterialdata.h"

#ifdef LOG4CXX
static LoggerPtr logger(Logger::getLogger("pz.mesh.TPZCompElHDivPressure"));
#endif

using namespace std;


// TESTADO
template<class TSHAPE>
TPZCompElHDivPressure<TSHAPE>::TPZCompElHDivPressure(TPZCompMesh &mesh, TPZGeoEl *gel, int &index) :
TPZCompElHDiv<TSHAPE>(mesh,gel,index) {

	
    fPressureOrder = mesh.GetDefaultOrder()-1;
	//criando o connect da variavel dual
	int nshape;
	if (TSHAPE::Type()==EQuadrilateral) {
		nshape =  pzshape::TPZShapeDisc::NShapeF(this->fPressureOrder, this->Dimension(), pzshape::TPZShapeDisc::  ETensorial);
	}
	if (TSHAPE::Type()==ETriangle) {
		nshape =  pzshape::TPZShapeDisc::NShapeF(this->fPressureOrder, this->Dimension(), pzshape::TPZShapeDisc::  EOrdemTotal);
	}
    int nstate = 1;
	int newnodeindex = mesh.AllocateNewConnect(nshape,nstate,fPressureOrder);
	TPZConnect &newnod = mesh.ConnectVec()[newnodeindex];
    newnod.SetPressure(true);
	this->fConnectIndexes[this->NConnects()]=newnodeindex;
	int seqnum = newnod.SequenceNumber();
    newnod.SetPressure(true);
    mesh.Block().Set(seqnum,nshape);
    mesh.ConnectVec()[this->fConnectIndexes[this->NConnects()]].IncrementElConnected();
	/*
	 
	 #ifdef LOG4CXX
	 {
	 std::stringstream sout;
	 sout << "After creating last connect " << i << std::endl;
	 this->Print(sout);
	 LOGPZ_DEBUG(logger,sout.str())
	 }
	 #endif
    */	
}


template<class TSHAPE>
TPZCompElHDivPressure<TSHAPE>::TPZCompElHDivPressure(TPZCompMesh &mesh, const TPZCompElHDivPressure<TSHAPE> &copy) :
TPZCompElHDiv<TSHAPE>(mesh,copy)
{
	fPressureOrder = copy.fPressureOrder;
	
}


template<class TSHAPE>
TPZCompElHDivPressure<TSHAPE>::TPZCompElHDivPressure(TPZCompMesh &mesh,
									 const TPZCompElHDivPressure<TSHAPE> &copy,
									 std::map<int,int> & gl2lcConMap,
									 std::map<int,int> & gl2lcElMap) :
TPZCompElHDiv<TSHAPE>(mesh,copy,gl2lcConMap,gl2lcElMap)
{
	
	fPressureOrder = copy.fPressureOrder;
}

template<class TSHAPE>
TPZCompElHDivPressure<TSHAPE>::TPZCompElHDivPressure() :
TPZCompElHDiv<TSHAPE>()
{
	fPressureOrder = -1;
}

template<class TSHAPE>
TPZCompElHDivPressure<TSHAPE>::~TPZCompElHDivPressure(){
	
}

template<class TSHAPE>
MElementType TPZCompElHDivPressure<TSHAPE>::Type() {
	return TSHAPE::Type();
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::SetPressureOrder(int order){
	fPressureOrder = order;
#ifdef LOG4CXX
	{
		std::stringstream sout;
		sout << endl<<"Ordem da Variavel dual: "<< fPressureOrder<<std::endl;
		LOGPZ_DEBUG(logger,sout.str())
	}
#endif
	
}

template<class TSHAPE>
int TPZCompElHDivPressure<TSHAPE>::DualOrder() {
	return fPressureOrder;
}

template<class TSHAPE>
int TPZCompElHDivPressure<TSHAPE>::NConnects() const {
	
    return TPZCompElHDiv<TSHAPE>::NConnects()+1;//acrescentando um connect mais pra variavel dual
    //int dimension = Dimension()-1;
	//return TSHAPE::NumSides(dimension) + 2;//acrescentando um connect mais pra variavel dual
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::SetConnectIndex(int i, int connectindex){
#ifndef NODEBUG
	if(i<0 || i>= this->NConnects()) {
		std::cout << " TPZCompElHDivPressure<TSHAPE>::SetConnectIndex index " << i <<
		" out of range\n";
		DebugStop();
		return;
	}
#endif
	this-> fConnectIndexes[i] = connectindex;
#ifdef LOG4CXX
	{
		std::stringstream sout;
		sout << endl<<"Setting Connect : " << i << " to connectindex " << connectindex<<std::endl;
		LOGPZ_DEBUG(logger,sout.str())
	}
#endif
}

template<class TSHAPE>
int TPZCompElHDivPressure<TSHAPE>::NConnectShapeF(int connect)const
{
    if(connect< NConnects()-1){//tirando o connect da pressao
        return TPZCompElHDiv<TSHAPE>::NConnectShapeF(connect);   
    }
    else{
        int dualorder=this->fPressureOrder;
        if (TSHAPE::Type()==EQuadrilateral){
            int numshape=pzshape::TPZShapeDisc::NShapeF(dualorder, this->Dimension(), pzshape::TPZShapeDisc:: ETensorial);
            return(numshape-1);//estou tirando apenas a funcao de maior ordem em xi e eta
        }
        if (TSHAPE::Type()==ETriangle){
            return pzshape::TPZShapeDisc::NShapeF(dualorder, this->Dimension(), pzshape::TPZShapeDisc:: EOrdemTotal);
        }
        else{
            std::cout<< "unhandled case "<<std::endl;
            DebugStop();
        }
    }
#ifdef LOG4CXX
    {
        std::stringstream sout;
        sout <<__PRETTY_FUNCTION__<< "unhandled case ";
        LOGPZ_ERROR(logger,sout.str());
    }
#endif
    return -1;
}



//template<class TSHAPE>
//void TPZCompElHDivPressure<TSHAPE>::SetIntegrationRule(int ord) {
//	TPZManVector<int,3> order(TSHAPE::Dimension,ord);
//	this->fIntRule.SetOrder(order);
//}


//
//template<class TSHAPE>
//int TPZCompElHDivPressure<TSHAPE>::NSideConnects(int side) const{
//	if(TSHAPE::SideDimension(side)<= Dimension()-2) return 0;
//	if(TSHAPE::SideDimension(side)==Dimension()-1) return 1;
//	if(TSHAPE::SideDimension(side)== Dimension()) return this->NConnects()-1;//tirando o connect da varivael dual
//#ifdef LOG4CXX
//	{
//		std::stringstream sout;
//		sout << __PRETTY_FUNCTION__ << "Side: " << side <<"unhandled case ";
//		LOGPZ_ERROR(logger,sout.str())
//	}
//#endif
//	return -1;
//	
//}

/** return the local index for side*/
//template<class TSHAPE>
//int TPZCompElHDivPressure<TSHAPE>::SideConnectLocId(int node,int side) const {
// 	if(TSHAPE::SideDimension(side)<= TSHAPE::Dimension - 2 || node >= NSideConnects(side)) {
//		PZError << "TPZCompElHDivPressure<TSHAPE>::SideConnectLocId no connect associate " <<  endl;
//		return -1;
//	}
//	
//	return side - TSHAPE::NSides + NConnects() - 1;//tirei o connect da pressao
//}


//Identifies the interpolation order on the connects of the element
template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::GetInterpolationOrder(TPZVec<int> &ord) {
	ord.Resize(NConnects());
	int i;
	for(i=0; i<NConnects(); i++) {
		ord[i] = ConnectOrder(i);
	}
}

//return the preferred order of the polynomial along connect connect
//template<class TSHAPE>
//int TPZCompElHDivPressure<TSHAPE>::PreferredSideOrder(int side) {
//	if(TSHAPE::SideDimension(side) < Dimension()-1)
//	{
//		PZError << __PRETTY_FUNCTION__ << " side " << side << std::endl;
//	}
//	int connect= TPZCompElHDiv<TSHAPE>::SideConnectLocId(0,side);
//	if(connect<0 || connect > NConnects()) {
//		PZError << "TPZCompElHDivPressure<TSHAPE>::PreferredSideOrder no polynomial associate " <<  endl;
//		return -1;
//	}
//	if(connect<NConnects()) {//connects de fluxo
//       return  TPZCompElHDiv<TSHAPE>::PreferredSideOrder(side);
//        
////		int order =this->fPreferredOrder;
////		return this->AdjustPreferredSideOrder(side,order);
//	}
//    else {
//        int order =this->fPreferredOrder;
//        return this->AdjustPreferredSideOrder(side,order);
//    }
//	PZError << "TPZCompElHDivPressure<TSHAPE>::PreferredSideOrder called for connect = " << connect << "\n";
//	return 0;
//	
//}

template<class TSHAPE>
int TPZCompElHDivPressure<TSHAPE>::ConnectIndex(int con) const{
#ifndef NODEBUG
	if(con<0 || con>= this->NConnects()) {
		std::cout << "TPZCompElHDivPressure::ConnectIndex wrong parameter connect " << con <<
		" NConnects " << this-> NConnects() << std::endl;
		DebugStop();
		return -1;
	}
	
#endif
	
	return this->fConnectIndexes[con];
}



//Sets the preferred interpolation order along a side
template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::SetPreferredOrder(int order)
{
	this->fPreferredOrder = order;
}

//sets the interpolation order of side to order
//template<class TSHAPE>
//void TPZCompElHDivPressure<TSHAPE>::SetSideOrder(int side, int order) {
//	int connectaux= SideConnectLocId(0,side);
// 	if(connectaux<0 || connectaux > this-> NConnects()) {
//		PZError << "TPZCompElHDivPressure::SetSideOrder. Bad paramenter side " << side << " order " << order << std::endl;
//#ifdef LOG4CXX
//		std::stringstream sout;
//		sout << __PRETTY_FUNCTION__ << " Bad side or order " << side << " order " << order;
//		LOGPZ_DEBUG(logger,sout.str())
//#endif
//		return;
// 	}
//	TPZConnect &c = this->Connect(connectaux);
//    c.SetOrder(order);
//    int seqnum = c.SequenceNumber();
//    int nvar = 1;
//    TPZAutoPointer<TPZMaterial> mat =this-> Material();
//    if(mat) nvar = mat->NStateVariables();
//    c.SetNState(nvar);
//    int nshape = NConnectShapeF(connectaux);
//    c.SetNShape(nshape);
//	this-> Mesh()->Block().Set(seqnum,nshape*nvar);
//    if(connectaux == NConnects()-1) {
//		SetIntegrationRule(2*order);
//		
//	}
//}

/**
 return the interpolation orderof the polynomial for connect
 **/
template<class TSHAPE>
int TPZCompElHDivPressure<TSHAPE>::ConnectOrder(int connect) const
{
	if (connect < NConnects() - 1) {
        return TPZCompElHDiv<TSHAPE>::ConnectOrder(connect);
    }
    else {
		return (this->fPressureOrder);//definindo ordem de interpolacao para o connect da pressao
	}
}

//template<class TSHAPE>
//int TPZCompElHDivPressure<TSHAPE>::SideOrder(int side) const
//{
//	//se side=connect devolve ConnectOrder
//	//int node;
//	if(!NSideConnects(side)) return -1;
//	int corder =SideConnectLocId(0, side);
//	int maxorder = 0;
//	int conectaux;
//	if(corder>=0 || corder <= NConnects()) return ConnectOrder(corder);
//	//caso contrario devolve a maior ordem de interpolacao
//	TPZStack< int > high;
//	TSHAPE::HigherDimensionSides(side, high);
//	int highside= high.NElements();
//	
//	
//	for(int j=0;j<highside;j++)//percorro todos os lados de dimensao maior
//	{
//		conectaux =SideConnectLocId(0, high[j]);
//		maxorder = (ConnectOrder(conectaux) > maxorder) ? ConnectOrder(conectaux) : maxorder;
//	}
//	
//	return maxorder;
//	
//}

//template<class TSHAPE>
//void TPZCompElHDivPressure<TSHAPE>::FirstShapeIndex(TPZVec<int> &Index){
//	
//	Index.Resize(TSHAPE::NSides+1);
//	Index[0]=0;
//	
//	for(int iside=0;iside<TSHAPE::NSides;iside++)
//	{
//		int order= SideOrder(iside);
//		Index[iside+1] = Index[iside] + TSHAPE::NConnectShapeF(iside,order);
//		
//	}
//	
//#ifdef LOG4CXX
//    std::stringstream sout;
//    sout << "First  Index" << Index;
//    LOGPZ_DEBUG(logger,sout.str())
//#endif
//	
//	
//}
//return a matrix with index shape and vector associate to element

//template<class TSHAPE>
//void TPZCompElHDivPressure<TSHAPE>::IndexShapeToVec(TPZVec<int> &VectorSide,TPZVec<std::pair<int,int> > & ShapeAndVec){
//	
//	
//    int count=0;
//    int tamanho= this->NShapeF() - NConnectShapeF(NConnects()-1);//estou tirando as funcoes da variavel dual
//    ShapeAndVec.Resize(tamanho);
//    
//    // VectorSide indicates the side associated with each vector entry
//    TPZManVector<int,27> FirstIndex;
//    // the first index of the shape functions
//    FirstShapeIndex(FirstIndex);
//    //#ifdef LOG4CXX
//    //        {
//    //                std::stringstream sout;
//    //                sout << "FirstIndex of shape functions " << FirstIndex;
//    //                LOGPZ_DEBUG(logger,sout.str())
//    //        }
//    //#endif
//    
//    ShapeAndVec.Resize(tamanho);
//    for(int jvec=0;jvec< VectorSide.NElements();jvec++)
//      {
//            int lside=VectorSide[jvec];
//            int fshape1= FirstIndex[lside];
//            int fshape2= FirstIndex[lside+1];
//            for (int ishape=fshape1; ishape<fshape2; ishape++)
//               {
//                   ShapeAndVec[count++]=std::pair<int,int>(jvec,ishape);
//                    }
//            
//           }	
//	
//#ifdef LOG4CXX
//	std::stringstream sout;
//	sout << " ShapeAndVec" << ShapeAndVec;
//	LOGPZ_DEBUG(logger,sout.str())
//#endif
//	
//}
//compute the values of the shape function of the side

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::SideShapeFunction(int side,TPZVec<REAL> &point,TPZFMatrix<REAL> &phi,TPZFMatrix<REAL> &dphi) {
	
	// this is an exception
	// when the sides parameter is "out of bounds", the method appends the dualshape functions
	// and assumes the point coordinates are already referring to the interior of the element
	// I hate exceptions...
	if( side == TSHAPE::NSides)
	{
		int nshapedual = NConnectShapeF(NConnects()-1);
		TPZFNMatrix<300> phi1(nshapedual,1),phi2(phi);
		TPZFNMatrix<900> dphi1(TSHAPE::Dimension,nshapedual),dphi2(dphi);
		ShapeDual(point,phi1,dphi1);
		Append(phi2, phi1, phi);
		Append(dphi2, dphi1, dphi);
		return;
	}
    else TPZCompElHDiv<TSHAPE>::SideShapeFunction(side,point,phi,dphi);
       
	/*
	 TPZFNMatrix<300> phi1,dphi1,phi2,dphi2;
	 TPZTransform tr = TSHAPE::SideToSideTransform(side,TSHAPE::NSides-1);
	 TPZManVector<REAL,5> interior;
	 tr.Apply(point,interior);
	 ShapeDual(interior,phi2,dphi2);
	 Append(phi1, phi2, phi);
	 Append(dphi1, dphi2, dphi);
	 */
	
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>:: Solution(TPZVec<REAL> &qsi,int var,TPZVec<STATE> &sol){
	TPZMaterialData data;
	InitMaterialData(data);
	this->ComputeShape(qsi, data.x,data.jacobian,data.axes, data.detjac,data.jacinv,data.phi, data.dphix);
	
	
	this->ComputeSolutionPressureHDiv(data);
	this->Material()->Solution(data,var,sol);
    
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::ComputeSolutionPressureHDiv(TPZVec<REAL> &qsi, TPZMaterialData &data){
	this->ComputeShape(qsi, data.x,data.jacobian,data.axes, data.detjac,data.jacinv,data.phi, data.dphix);
    this->ComputeSolutionPressureHDiv(data);
	
    
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::ComputeSolution(TPZVec<REAL> &qsi, TPZFMatrix<REAL> &phi, TPZFMatrix<REAL> &dphix,
                                            const TPZFMatrix<REAL> &axes, TPZSolVec &sol, TPZGradSolVec &dsol){
    TPZMaterialData data;
    InitMaterialData(data);
    this->ComputeSolutionPressureHDiv(data);
	
    
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::ComputeSolution(TPZVec<REAL> &qsi, TPZSolVec &sol, TPZGradSolVec &dsol,TPZFMatrix<REAL> &axes){
	
	//	TPZFMatrix<REAL> dphix,phi;
	//	ComputeShape()
	//	this->ComputeSolution(qsi,phi,dphix,axes,sol,dsol);
	
	TPZGeoEl * ref = this->Reference();
	const int nshape = this->NShapeF();
	const int dim = ref->Dimension();
	TPZFMatrix<REAL> phix(nshape,1),dphix(dim,nshape);
	
	TPZFNMatrix<9> jacobian(dim,dim);
	TPZFNMatrix<9> jacinv(dim,dim);
	REAL detjac;
	
	TPZManVector<REAL,3> x(3,0.);
	this->ComputeShape(qsi,x,jacobian,axes,detjac,jacinv,phix,dphix);
	this->ComputeSolution(qsi, phix, dphix, axes, sol, dsol);
	
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::ComputeSolutionPressureHDiv(TPZMaterialData &data){
	//const int dim = this->Reference()->Dimension();
    const int numdof = this->Material()->NStateVariables();
    const int ncon = this->NConnects();
	
	
    
	TPZBlock<STATE> &block =this->Mesh()->Block();
    TPZFMatrix<STATE> &MeshSol = this->Mesh()->Solution();
    int numbersol = MeshSol.Cols();
    
	int nsol= this->Dimension()+2;
    data.sol.Resize(numbersol);
    data.dsol.Resize(numbersol);
    for (int is=0; is<numbersol; is++) {
        data.sol[is].Resize(nsol,1);//2 componente para fluxo+ 1 para pressao +1 para div
        data.sol[is].Fill(0);

    }
	//solucao associada a fluxo
	int iv = 0,ishape=0,ivec=0,cols, jv=0;
    for(int in=0; in<ncon-1 ; in++) {//estou tirando o connect da pressao
		TPZConnect *df = &this->Connect(in);
		int dfseq = df->SequenceNumber();
		int dfvar = block.Size(dfseq);
		int pos = block.Position(dfseq);
		
		for(int jn=0; jn<dfvar; jn++) {
			ivec=data.fVecShapeIndex[jv ].first;
			ishape=data.fVecShapeIndex[jv].second;
			
			TPZFNMatrix<3> ivecDiv(3,1);
			ivecDiv(0,0) = data.fNormalVec(0,ivec);
			ivecDiv(1,0) = data.fNormalVec(1,ivec);
			ivecDiv(2,0) = data.fNormalVec(2,ivec);
			TPZFNMatrix<3> axesvec(3,1);
			data.axes.Multiply(ivecDiv,axesvec);
			
			for (int ilinha=0; ilinha<this->Dimension(); ilinha++) {
				cols=iv%numdof;
				
				//	 #ifdef LOG4CXX
				//	 std::stringstream sout;
				//	 sout << " vetor  " << ivec << " shape  " << ishape<<" coef "<< MeshSol(pos+jn,0)<<endl;
				//	 LOGPZ_DEBUG(logger,sout.str())
				//	 #endif
				for (int is=0; is<numbersol; is++) {
                    data.sol[is][ilinha] += data.fNormalVec(ilinha,ivec)* data.phi(ishape,0)*MeshSol(pos+jn,is);
                    data.sol[is][nsol-1] +=  axesvec(ilinha,0)*data.dphix(ilinha,ishape)*MeshSol(pos+jn,is);//divergente
                    
                }
           		
			}
			
			jv++;
		}
		
		iv++;
	}
	
	
    //colocando a solucao para o connect interno usando shape descontinua
    
    TPZConnect *df2 = &this->Connect(ncon-1);
    int dfseq2 = df2->SequenceNumber();
    int pos2 = block.Position(dfseq2);
    
    for (int idesc=0; idesc<data.numberdualfunctions; idesc++) {
		int iphi= data.phi.Rows()-data.numberdualfunctions +idesc;
        for (int is=0; is<numbersol; is++) {
            data.sol[is][nsol-2]+= data.phi(iphi,0)*MeshSol(pos2+idesc,is);            
        }
		
    }
    
}


template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::Append(TPZFMatrix<REAL> &u1, TPZFMatrix<REAL> &u2, TPZFMatrix<REAL> &u12)
{
	bool Is_u1PHI = (u1.Cols() == 1) ? true : false;
	bool Is_u2PHI = (u2.Cols() == 1) ? true : false;
	
	if(Is_u1PHI && Is_u2PHI)
	{
		int nu1 = u1.Rows(),nu2 = u2.Rows();
		u12.Redim(nu1+nu2,1);
		int i;
		for(i=0; i<nu1; i++) u12(i,0) = u1(i,0);
		for(i=0; i<nu2; i++) u12(i+nu1,0) = u2(i,0);
		
		
	}
	else if(!Is_u1PHI || !Is_u2PHI) // Se u1 e u2 não são Phi's, implica em serem dPhi's
	{
		int ru1 = u1.Rows(), cu1 = u1.Cols(), ru2 = u2.Rows(), cu2 = u2.Cols();
		int ru12 = ru1 < ru2 ? ru2 : ru1;
		int cu12 = cu1+cu2;
		u12.Redim(ru12,cu12);
		int i,j;
		for(i=0; i<ru1; i++) for(j=0; j<cu1; j++) u12(i,j) = u1(i,j);
		for(i=0; i<ru2; i++) for(j=0; j<cu2; j++) u12(i,j+cu1) = u2(i,j);//---modifiquei--
	}
	else
	{
		PZError << "TPZCompElHDivPressure::Append. Bad input parameters " << std::endl;//Este metodo so serve para u1 E u2 do mesmo tipo
		
		
	}
	
}

/** compute the shape functions corresponding to the dual space
 *
 */
template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::ShapeDual(TPZVec<REAL> &qsi, TPZFMatrix<REAL> &phi, TPZFMatrix<REAL> &dphi)
{
	int dimension= TSHAPE::Dimension;
	REAL C=1;//fator de escala utilizado neste metodo
	TPZManVector<REAL,3> X0(3,0.);//centro do elemento
	
    int degree= this->fPressureOrder;
	//	const int nshapedisc = pzshape::TPZShapeDisc::NShapeF(degree, dimension, pzshape::TPZShapeDisc:: ETensorial);
	pzshape::TPZShapeDisc::Shape(dimension,C,X0,qsi,degree,phi,dphi, pzshape::TPZShapeDisc:: ETensorial);
	
}

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::Shape(TPZVec<REAL> &pt, TPZFMatrix<REAL> &phi, TPZFMatrix<REAL> &dphi) {
    
	TPZFMatrix<REAL> phiCont;
  	TPZFMatrix<REAL> dphiCont;
	TPZCompElHDiv<TSHAPE>::Shape(pt,phiCont,dphiCont);
    
	// acrescentar as funcoes de pressao (via descontinuo)
	REAL C=1;//fator de escala utilizado neste metodo
	TPZManVector<REAL,3> X0(3,0.);//centro do elemento
  
    int dimension= TSHAPE::Dimension;
	
    int degree= this->fPressureOrder;
    int nshapedisc;
    if (TSHAPE::Type()==EQuadrilateral) 
    {
        nshapedisc = pzshape::TPZShapeDisc::NShapeF(degree, dimension, pzshape::TPZShapeDisc:: ETensorial);//ETensorial);ETensorial);
    }
    else if(TSHAPE::Type() == ETriangle)
    {
        nshapedisc = pzshape::TPZShapeDisc::NShapeF(degree, dimension, pzshape::TPZShapeDisc:: EOrdemTotal);//ETensorial);ETensorial);
    }
    else
    {
        DebugStop();
    }
	TPZFNMatrix<660> phiDisc(nshapedisc,1);
	
	TPZFNMatrix<660> dphiDisc(dimension,nshapedisc);
    
    if (TSHAPE::Type()==EQuadrilateral) {
        pzshape::TPZShapeDisc::Shape(dimension,C,X0,pt,degree,phiDisc,dphiDisc, pzshape::TPZShapeDisc::ETensorial);// EOrdemTotal);//ETensorial);
    } 
    else if (TSHAPE::Type()==ETriangle) 
    {
        pzshape::TPZShapeDisc::Shape(dimension,C,X0,pt,degree,phiDisc,dphiDisc, pzshape::TPZShapeDisc::EOrdemTotal);
    }
    else
    {
        DebugStop();
    }
	this->Append(phiCont,phiDisc,phi);
	this->Append(dphiCont,dphiDisc,dphi);
	/*
	 #ifdef LOG4CXX
	 std::stringstream sout;
	 sout << " Tamanho do vetor de Shape continuas " << phiCont.Rows()<<" Tamanho do vetor de Shape descontinuas " << phiDisc.Rows()<<" Tamanho do vetor de Shape Total " << phi.Rows()<<endl;
	 sout << " Tamanho do vetor de DPHI continuas (linhas)" << dphiCont.Rows()<<" Tamanho do vetor de Shape descontinuas " << dphiDisc.Rows()<<" Tamanho do vetor de DPHI Total " << dphi.Rows()<<endl;
	 sout << " Tamanho do vetor de DPHI continuas (colunas)" << dphiCont.Cols()<<" Tamanho do vetor de DPHI descontinuas " << dphiDisc.Cols()<<" Tamanho do vetor de DPHI Total " << dphi.Cols()<<endl;
	 LOGPZ_DEBUG(logger,sout.str())
	 #endif
	 */
}



template<class TSHAPE>
TPZTransform TPZCompElHDivPressure<TSHAPE>::TransformSideToElement(int side){
	return TSHAPE::TransformSideToElement(side);
}

//template<class TSHAPE>
//void TPZCompElHDivPressure<TSHAPE>::ComputeShapeIndex(TPZVec<int> &sides, TPZVec<int> &shapeindex){
//	
//	TPZManVector<int> firstshapeindex;
//	FirstShapeIndex(firstshapeindex);
//	int nshape = TPZCompElHDiv<TSHAPE>::NShapeF();
//	shapeindex.Resize(nshape);
//	int nsides = sides.NElements();
//	int is, count=0;
//	for(is=0 ; is<nsides; is++)
//	{
//		int side = sides[is];
//		int sideorder= this->SideOrder(side);
//		int NShapeFace = TSHAPE::NConnectShapeF(side,sideorder);
//		int ishapeface;
//		for(ishapeface=0; ishapeface<NShapeFace; ishapeface++)
//		{
//			shapeindex[count++] = is;
//		}
//	}
//	shapeindex.Resize(count);
//#ifdef LOG4CXX
//	{
//		std::stringstream sout;
//		sout << "count = " << count << " nshape " << nshape;
//		sout << endl<<"sides associated with the normals "<< sides <<
//		"\nnormal associated with each shape function : shape function indexes " << shapeindex;
//		LOGPZ_DEBUG(logger,sout.str())
//	}
//#endif
//}

/** Initialize a material data and its attributes based on element dimension, number
 * of state variables and material definitions
 */

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::InitMaterialData(TPZMaterialData &data)
{
//	TPZCompElHDiv<TSHAPE>::InitMaterialData(data);
//	
//#ifdef LOG4CXX
//	{
//		LOGPZ_DEBUG(logger,"Initializing normal vectors")
//	}
//#endif
//	TPZManVector<int> normalsides;
//	TPZCompElHDiv<TSHAPE>::Reference()->ComputeNormals(data.fNormalVec, normalsides);
//	// vecindex : lado associado a cada normal
//	// vecindex indica apenas o numero do lado associado a cada normal
//	// agora temos que expandir para formar pares : vecIndex e shapeindex
//	//	ComputeShapeIndex(data.fVecIndex,data.fShapeIndex);
//	data.numberdualfunctions = NConnectShapeF(NConnects()-1);
//	IndexShapeToVec(normalsides,data.fVecShapeIndex);
    
    TPZCompElHDiv<TSHAPE>::InitMaterialData(data);
    data.numberdualfunctions = NConnectShapeF(NConnects()-1);
}

///  Save the element data to a stream
template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::Write(TPZStream &buf, int withclassid)
{
	TPZInterpolatedElement::Write(buf,withclassid);
	TPZManVector<int,3> order(3,0);
	this-> fIntRule.GetOrder(order);
	this->  WriteObjects(buf,order);
	buf.Write(this->fConnectIndexes,TSHAPE::NSides);
	buf.Write(&this->fPreferredOrder,1);
	int classid = this->ClassId();
	buf.Write ( &classid, 1 );
}


//Read the element data from a stream

template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::Read(TPZStream &buf, void *context)
{
	TPZInterpolatedElement::Read(buf,context);
	TPZManVector<int,3> order;
	this-> ReadObjects(buf,order);
	this-> fIntRule.SetOrder(order);
	buf.Read(this->fConnectIndexes,TSHAPE::NSides);
	buf.Read(&this->fPreferredOrder,1);
	int classid = -1;
	buf.Read( &classid, 1 );
	if ( classid != this->ClassId() )
	{
		std::stringstream sout;
		sout << "ERROR - " << __PRETTY_FUNCTION__
        << " trying to restore an object id " << this->ClassId() << " for an package of id = " << classid;
		LOGPZ_ERROR ( logger, sout.str().c_str() );
	}
}


#include "pzshapecube.h"
#include "TPZRefCube.h"
#include "pzshapelinear.h"
#include "TPZRefLinear.h"
#include "pzrefquad.h"
#include "pzshapequad.h"
#include "pzgeoquad.h"
#include "pzshapetriang.h"
#include "pzreftriangle.h"
#include "pzgeotriangle.h"
#include "pzshapeprism.h"
#include "pzrefprism.h"
#include "pzgeoprism.h"
#include "pzshapetetra.h"
#include "pzreftetrahedra.h"
#include "pzgeotetrahedra.h"
#include "pzshapepiram.h"
#include "pzrefpyram.h"
#include "pzgeopyramid.h"
#include "pzrefpoint.h"
#include "pzgeopoint.h"
#include "pzshapepoint.h"
#include "pzgraphelq2dd.h"
#include "tpzgraphelt3d.h"
#include "pzgraphel1dd.h"
#include "pztrigraphd.h"
#include "pzgraphelq3dd.h"
#include "tpzgraphelprismmapped.h"
#include "tpzgraphelpyramidmapped.h"
#include "tpzgraphelt2dmapped.h"

using namespace pztopology;

#include "tpzpoint.h"
#include "tpzline.h"
#include "tpzquadrilateral.h"
#include "tpztriangle.h"
#include "tpzcube.h"
#include "tpztetrahedron.h"
#include "tpzprism.h"
#include "tpzpyramid.h"

#include "pzmeshid.h"

#include "pzelchdivbound2.h"

using namespace pzgeom;
using namespace pzshape;

template<>
void TPZCompElHDivPressure<TPZShapePoint>::CreateGraphicalElement(TPZGraphMesh &grafgrid, int dimension) {
	if(dimension == 0) std::cout << "A point element has no graphical representation\n";
}



template<class TSHAPE>
void TPZCompElHDivPressure<TSHAPE>::CreateGraphicalElement(TPZGraphMesh &grafgrid, int dimension) {
	if(dimension == TSHAPE::Dimension && this->Material()->Id() > 0) {
		new typename TSHAPE::GraphElType(this,&grafgrid);
	}
}

template<>
int TPZCompElHDivPressure<TPZShapePoint>::ClassId() const
{
	return TPZHDIVPOINTID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapePoint>, TPZHDIVPOINTID>;


template<>
int TPZCompElHDivPressure<TPZShapeLinear>::ClassId() const
{
	return TPZHDIVLINEARID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapeLinear>, TPZHDIVLINEARID>;

template<>
int TPZCompElHDivPressure<TPZShapeTriang>::ClassId() const
{
	return TPZHDIVTRIANGLEID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapeTriang>, TPZHDIVTRIANGLEID>;

template<>
int TPZCompElHDivPressure<TPZShapeQuad>::ClassId() const
{
	return TPZHDIVQUADID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapeQuad>, TPZHDIVQUADID>;

template<>
int TPZCompElHDivPressure<TPZShapeCube>::ClassId() const
{
	return TPZHDIVCUBEID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapeCube>, TPZHDIVCUBEID>;

template<>
int TPZCompElHDivPressure<TPZShapeTetra>::ClassId() const
{
	return TPZHDIVTETRAID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapeTetra>, TPZHDIVTETRAID>;

template<>
int TPZCompElHDivPressure<TPZShapePrism>::ClassId() const
{
	return TPZHDIVPRISMID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapePrism>, TPZHDIVPRISMID>;

template<>
int TPZCompElHDivPressure<TPZShapePiram>::ClassId() const
{
	return TPZHDIVPYRAMID;
}

template class
TPZRestoreClass< TPZCompElHDivPressure<TPZShapePiram>, TPZHDIVPYRAMID>;

/*
template class TPZCompElHDivPressure<TPZShapeTriang>;
template class TPZCompElHDivPressure<TPZShapePoint>;
template class TPZCompElHDivPressure<TPZShapeLinear>;
template class TPZCompElHDivPressure<TPZShapeQuad>;
template class TPZCompElHDivPressure<TPZShapeTetra>;
template class TPZCompElHDivPressure<TPZShapePrism>;
template class TPZCompElHDivPressure<TPZShapePiram>;
template class TPZCompElHDivPressure<TPZShapeCube>;


TPZCompEl * CreateHDivPointEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure<TPZShapePoint>(mesh,gel,index);
}


TPZCompEl * CreateHDivLinearEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivBound2< TPZShapeLinear>(mesh,gel,index);
}

TPZCompEl * CreateHDivQuadEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapeQuad>(mesh,gel,index);
}

TPZCompEl * CreateHDivTriangleEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapeTriang >(mesh,gel,index);
}

TPZCompEl * CreateHDivCubeEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapeCube >(mesh,gel,index);
}

TPZCompEl * CreateHDivPrismEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapePrism>(mesh,gel,index);
}

TPZCompEl * CreateHDivPyramEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapePiram >(mesh,gel,index);
}

TPZCompEl * CreateHDivTetraEl(TPZGeoEl *gel,TPZCompMesh &mesh,int &index) {
	return new TPZCompElHDivPressure< TPZShapeTetra >(mesh,gel,index);
}

*/