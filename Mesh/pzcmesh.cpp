/**
 * @file
 * @brief Contains the implementation of the TPZCompMesh methods.
 */

#include "pzcmesh.h"
#include "pzeltype.h"
#include "pzerror.h"
#include "pzgmesh.h"
#include "pzcompel.h"
#include "pzintel.h"
#include "pzgeoelside.h"
#include "pzgeoel.h"
#include "pzconnect.h"
#include "pzbndcond.h"
#include "pzmaterial.h"

#include "pzsolve.h"
#include "pzmatrix.h"
#include "pzfmatrix.h"
#include "pzblock.h"
#include "pzelmat.h"
#include "pzsubcmesh.h"
#include "TPZCompElDisc.h"
#include "TPZInterfaceEl.h"
#include "pztrnsform.h"
#include "pztransfer.h"
#include "pzmultiphysicscompel.h"

#include "pzvec.h"
#include "pzadmchunk.h"
#include "pzsubcmesh.h"

#include "pzmetis.h"
#include "pzstream.h"
#include <map>
#include <sstream>
#include <set>

#include "pzlog.h"

#ifndef STATE_COMPLEX
	#include "TPZAgglomerateEl.h"
#endif

#ifdef LOG4CXX
static LoggerPtr logger(Logger::getLogger("pz.mesh.tpzcompmesh"));
#endif
using namespace std;


TPZCompMesh::TPZCompMesh (TPZGeoMesh* gr) : fElementVec(0),
fConnectVec(0),fMaterialVec(),
fSolution(0,1) {
	fDefaultOrder = TPZCompEl::GetgOrder();
	
	//Initializing class members
	fDimModel = 0;
	fReference = gr;
	//  fChecked = 0;
	//fName[0] = '\0';
	//fName[126] = '\0';
	if(gr) {
		SetName( gr->Name() );
		gr->ResetReference();
		gr->SetReference(this);
	}
	fBlock.SetMatrix(&fSolution);
	fSolutionBlock.SetMatrix(&fSolution);
}


TPZCompMesh::TPZCompMesh(TPZAutoPointer<TPZGeoMesh> &gmesh) : fGMesh(gmesh),fElementVec(0),
fConnectVec(0),fMaterialVec(),
fSolution(0,1)
{
    fDefaultOrder = TPZCompEl::GetgOrder();
    
    //Initializing class members
    fDimModel = 0;
    fReference = gmesh.operator->();
    //  fChecked = 0;
    //fName[0] = '\0';
    //fName[126] = '\0';
    if(fReference) {
        SetName( fReference->Name() );
        fReference->ResetReference();
        fReference->SetReference(this);
    }
    fBlock.SetMatrix(&fSolution);
    fSolutionBlock.SetMatrix(&fSolution);
	
}

TPZCompMesh::~TPZCompMesh() {
	
#ifdef LOG4CXX
    if (logger->isDebugEnabled()) {
        std::stringstream sout;
        Print(sout);
        LOGPZ_DEBUG(logger, sout.str())
    }
#endif
	// THIS NEEDS TO INCLUDE THE DELETION ROUTINES OF ALL ITEMS
	this->CleanUp();
	TPZGeoMesh * ref = this->Reference();
	if (ref){
		if(ref->Reference() == this){
			ref->ResetReference();
		}//if == this
	}//if (ref)
}//~

void TPZCompMesh::CleanUp() {
	
	// THIS ROUTINE NEEDS TO INCLUDE THE DELETION OF THE LIST POINTERS
	TPZGeoMesh *ref = Reference();
	if (ref){
		ref->ResetReference();
		this->LoadReferences();
	}
#ifdef DEBUG
    ComputeNodElCon();
#endif
	int i, nelem = this->NElements();
	
	//deleting interfaces
	for(i=0; i<nelem; i++){
		TPZCompEl *el = fElementVec[i];
		TPZInterfaceElement * face = dynamic_cast<TPZInterfaceElement*>(el);
		if(face){
			delete el;
		}
	}
	
	//deleting other elements
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		delete el;
	}
	
	fElementVec.Resize(0);
	fElementVec.CompactDataStructure(1);
	fConnectVec.Resize(0);
	fConnectVec.CompactDataStructure(1);
	nelem = NMaterials();
    std::map<int, TPZMaterial *>::iterator it;
    for (it = fMaterialVec.begin(); it != fMaterialVec.end(); it++) {
        delete it->second;
    }
	fMaterialVec.clear();
	
	fBlock.SetNBlocks(0);
	fSolutionBlock.SetNBlocks(0);
	fSolution.Redim(0,0);
}

void TPZCompMesh::SetName (const string &nm) {
	fName = nm;
}

void TPZCompMesh::Print (std::ostream & out) const {
	
	//ComputeNodElCon();
	out << "\n\t\tCOMPUTABLE GRID INFORMATIONS:\n\n";
	out << "TITLE-> " << fName << "\n\n";
	
	out << "number of connects            = " << NConnects() << std::endl;
	out << "number of elements            = " << NElements() << std::endl;
	out << "number of materials           = " << NMaterials() << std::endl;
	//  out << "number of nodal bound cond    = " << NBCConnects() << endl;
	
	out << "\n\t Connect Information:\n\n";
	int i, nelem = NConnects();
	for(i=0; i<nelem; i++) {
		if(fConnectVec[i].SequenceNumber() == -1) {
			if(fConnectVec[i].HasDependency()) {
				cout << "TPZCompMesh::Print inconsistency of connect\n";
				cout << "Index " << i << ' ';
				fConnectVec[i].Print(*this,std::cout);
			}
			continue;
		}
		out << "Index " << i << ' ';
		fConnectVec[i].Print(*this,out);
	}
	out << "\n\t Computable Element Information:\n\n";
	nelem = NElements();
	for(i=0; i<nelem; i++) {
		if(!fElementVec[i]) continue;
		TPZCompEl *el = fElementVec[i];
		out << "Index " << i << ' ';
		el->Print(out);
		if(!el->Reference()) continue;
		out << "\tReference Index = " << el->Reference()->Index() << std::endl << std::endl;
	}
	out << "\n\tMaterial Information:\n\n";
	std::map<int, TPZMaterial * >::const_iterator mit;
	nelem = NMaterials();
	for(mit=fMaterialVec.begin(); mit!= fMaterialVec.end(); mit++) {
        TPZMaterial *mat = mit->second;
        if (!mat) {
            DebugStop();
        }
		mat->Print(out);
	}
}

/**Insert a material object in the datastructure*/
int TPZCompMesh::InsertMaterialObject(TPZMaterial * mat) {
	if(!mat) return -1;
	int matid = mat->Id();
	fMaterialVec[matid] = mat;
	return fMaterialVec.size();
}

TPZMaterial * TPZCompMesh::FindMaterial(int matid){	// find the material object with id matid
	TPZMaterial * result = 0;
	std::map<int, TPZMaterial * >::iterator mit;
	mit = fMaterialVec.find(matid);
	if(mit != fMaterialVec.end())
	{
		result = mit->second;
	}
	return result;
}

void TPZCompMesh::AutoBuild(const std::set<int> *MaterialIDs) {
	TPZAdmChunkVector<TPZGeoEl *> &elvec = Reference()->ElementVec();
	int i, nelem = elvec.NElements();
	int neltocreate = 0;
	int index;
	for(i=0; i<nelem; i++) {
		TPZGeoEl *gel = elvec[i];
		if(!gel) continue;
		if(!gel->HasSubElement()) {
			neltocreate++;
		}
	}
	std::set<int> matnotfound;
	int nbl = fBlock.NBlocks();
		if(neltocreate > nbl) 
		{
				fBlock.SetNBlocks(neltocreate);
		}
				fBlock.SetNBlocks(nbl);
	
	for(i=0; i<nelem; i++) {
		TPZGeoEl *gel = elvec[i];
		if(!gel) continue;
		if(!gel->HasSubElement()) {
			int matid = gel->MaterialId();
			TPZMaterial * mat = this->FindMaterial(matid);
			if(!mat)
			{
				matnotfound.insert(matid);
				continue;
			}
			int printing = 0;
			if (printing) {
				gel->Print(cout);
			}
			
			//checking material in MaterialIDs
			if(MaterialIDs){
				std::set<int>::const_iterator found = MaterialIDs->find(matid);
				if (found == MaterialIDs->end()) continue;
			}
			
			if(!gel->Reference() && gel->NumInterfaces() == 0)
			{
				CreateCompEl(gel,index);
			}
		}
	}
	
	InitializeBlock();
	
}

void TPZCompMesh::AutoBuildContDisc(const TPZVec<TPZGeoEl*> &continuous, const TPZVec<TPZGeoEl*> &discontinuous) {
	
	TPZAdmChunkVector<TPZGeoEl *> &elvec = Reference()->ElementVec();
	int nelem = elvec.NElements();
	
	int neltocreate = 0;
	int index;
	for(int i=0; i<nelem; i++) {
		TPZGeoEl *gel = elvec[i];
		if(!gel) continue;
		if(!gel->HasSubElement()) {
			neltocreate++;
		}
	}
	
	int nbl = fBlock.NBlocks();
	if(neltocreate > nbl) fBlock.SetNBlocks(neltocreate);
	fBlock.SetNBlocks(nbl);
	
	//Creating continuous elements
	fCreate.SetAllCreateFunctionsContinuous();
	int ncont = continuous.NElements();
	for(int i = 0; i < ncont; i++){
		TPZGeoEl *gel = continuous[i];
		if(!gel) continue;
		if(!gel->HasSubElement()) {
			int printing = 0;
			if (printing) {
				gel->Print(cout);
			}
			
			if(gel->NumInterfaces() == 0){
				CreateCompEl(gel,index);
			}
		}
	}
	
	//Creating discontinuous elements
	fCreate.SetAllCreateFunctionsDiscontinuous();
	int ndisc = discontinuous.NElements();
	for(int i = 0; i < ndisc; i++){
		TPZGeoEl *gel = discontinuous[i];
		if(!gel) continue;
		if(!gel->HasSubElement()) {
			int printing = 0;
			if (printing) {
				gel->Print(cout);
			}
			
			if(gel->NumInterfaces() == 0){
				CreateCompEl(gel,index);
			}
		}
	}
	
	this->InitializeBlock();
}

void TPZCompMesh::InitializeBlock() {
	ExpandSolution();
	CleanUpUnconnectedNodes();
}

void TPZCompMesh::ExpandSolution() {
	fBlock.Resequence();
	int ibl,nblocks = fBlock.NBlocks();
	
	//TPZFMatrix<REAL> OldSolution(fSolution);
	TPZFMatrix<STATE> OldSolution(fSolution);
	
	int cols = fSolution.Cols();
	fSolution.Redim(fBlock.Dim(),cols);
	int minblocks = nblocks < fSolutionBlock.NBlocks() ? nblocks : fSolutionBlock.NBlocks();
	/*
	 int ic;
	 for(ic=0; ic<cols; ic++) {
	 for(ibl = 0;ibl<minblocks;ibl++) {
	 int oldsize = fSolutionBlock.Size(ibl);
	 int oldposition = fSolutionBlock.Position(ibl);
	 int newsize = fBlock.Size(ibl);
	 int newposition = fBlock.Position(ibl);
	 int minsize = (oldsize < newsize) ? oldsize : newsize;
	 int ieq;
	 int offset = 0;
	 if(Discontinuous)offset = newsize - oldsize;
	 for(ieq=0; ieq<minsize; ieq++) {
	 fSolution(newposition+ieq+offset,ic) = OldSolution(oldposition+ieq,ic);
	 }
	 }
	 }
	 */
	int ic;
	for(ic=0; ic<cols; ic++) {
		for(ibl = 0;ibl<minblocks;ibl++) {
			int oldsize = fSolutionBlock.Size(ibl);
			int oldposition = fSolutionBlock.Position(ibl);
			int newsize = fBlock.Size(ibl);
			int newposition = fBlock.Position(ibl);
			int minsize = (oldsize < newsize) ? oldsize : newsize;
			int ieq;
			for(ieq=0; ieq<minsize; ieq++) {
				fSolution(newposition+ieq,ic) = OldSolution(oldposition+ieq,ic);
			}
		}
	}
	fSolutionBlock = fBlock;
}

void TPZCompMesh::LoadSolution(const TPZFMatrix<REAL> &mat){
	
	int nrow = mat.Rows();
	int ncol = mat.Cols();
    int solrow = fSolution.Rows();
    fSolution.Resize(solrow, ncol);
	int i,j;
    REAL val;
	for(j=0;j<ncol;j++)
    {
        for(i=0;i<nrow;i++)
        {
            val = mat.GetVal(i,j);
            fSolution(i,j) =  val;
        }
        
    }
	int nelem = NElements();
	TPZCompEl *cel;
	for(i=0; i<nelem; i++) {
		cel = fElementVec[i];
		if(!cel) continue;
		cel->LoadSolution();
	}
}

void TPZCompMesh::LoadReferences() {
	
	//	Reference()->ResetReference();
	Reference()->SetReference(this);
	int i, nelem = NElements();
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		el->LoadElementReference();
		/*  TPZGeoEl *gel = el->Reference();
		 if(!gel) continue;
		 gel->SetReference(el);
		 */
	}
}

void TPZCompMesh::CleanUpUnconnectedNodes() {
	ComputeNodElCon();
	int i, nelem = NConnects();
	int ndepblocks = 0, nvalidblocks = 0, nremoved = 0, ncondensed = 0;
	for (i=0;i<nelem;i++)
    {
		TPZConnect &no = fConnectVec[i];
		int seq = no.SequenceNumber();
		if(!no.NElConnected() && seq != -1)
		{
			nremoved++;
		}
		else if(!no.HasDependency() && no.NElConnected() && !no.IsCondensed()) nvalidblocks++;
        else if(!no.HasDependency() && no.NElConnected() && no.IsCondensed()) ncondensed++;
		else if(no.HasDependency() && no.NElConnected()) ndepblocks++;
    }
	int need = 0;
	for (i=0;i<nelem;i++) {
		TPZConnect &no = fConnectVec[i];
		if (no.SequenceNumber() == -1) continue;
		if (no.HasDependency() && no.NElConnected() == 0) {
			PZError << "TPZCompMesh::CleanUpUnconnectedNodes node has dependency\n";
			continue;
		}
		if (!no.NElConnected() && no.SequenceNumber() != -1)
		{
			need = 1;
			break;
		}
		if (no.HasDependency() && no.SequenceNumber() < nvalidblocks)
		{
			need = 1;
			break;
		} else if(!no.HasDependency() && !no.IsCondensed() && no.SequenceNumber() >= nvalidblocks)
		{
			need = 1;
			break;
		} else if(!no.HasDependency() && no.IsCondensed() && no.SequenceNumber() < nvalidblocks)
		{
			need = 1;
			break;
		}
	}
	int nblocks = fBlock.NBlocks();
	TPZManVector<long> permute(nblocks,-1), down(nblocks,0);
	int idepblocks = 0, iremovedblocks= 0, icondensed = 0;
	
	if (need) {
		for(i=0; i<nelem; i++) {
			TPZConnect &no = fConnectVec[i];
			if(no.SequenceNumber() == -1) continue;
			int seq = no.SequenceNumber();
			if(no.NElConnected() == 0)
			{
				permute[seq] = nvalidblocks+ndepblocks+iremovedblocks+ncondensed;
				down[seq] = 1;
				fBlock.Set(seq,0);
                no.Reset();
                //				no.SetSequenceNumber(-1);
				fConnectVec.SetFree(i);
				iremovedblocks++;
			}
			else if(no.HasDependency()) {
				permute[seq] = nvalidblocks+ncondensed+idepblocks;
				down[seq] = 1;
				idepblocks++;
			}
            else if(no.IsCondensed())
            {
				permute[seq] = nvalidblocks+icondensed;
				down[seq] = 1;
				icondensed++;
                
            }
		}
		for(i=1; i<nblocks; i++) down[i] += down[i-1];
		for(i=0; i<nblocks; i++)
		{
			if(permute[i] == -1)
			{
				permute[i] = i-down[i];
			}
		}
	}
#ifdef LOG4CXX
	if(need)
        if (logger->isDebugEnabled())
    {
		std::stringstream sout;
		sout << "permute to put the free connects to the back\n";
		if(nblocks < 50) for (i=0;i<nblocks;i++) sout << permute[i] << ' ';
		sout << "need = " << need << endl;
		LOGPZ_DEBUG(logger,sout.str());
    }
#endif
	
	if (need) {
#ifdef DEBUG
		std::set<int> check;
		nelem = permute.NElements();
		for(i=0; i<nelem; i++) check.insert(permute[i]);
		if(static_cast<int>(check.size()) != nelem)
		{
			cout << __PRETTY_FUNCTION__ << " The permutation vector is not a permutation!\n" << permute << endl;
			DebugStop();
		}
#endif
		Permute(permute);
		fBlock.SetNBlocks(nblocks-nremoved);
	}
}

void TPZCompMesh::ComputeNodElCon() {
	
	int i, nelem = NConnects();
	for(i=0; i<nelem; i++) {
		TPZConnect &no = fConnectVec[i];
		if(no.SequenceNumber() == -1) continue;
		no.ResetElConnected();
	}
	
	
	TPZStack<int> nodelist;
	int numnod;
	// modified Philippe 22/7/97
	// in order to account for constrained nodes
	nelem = NElements();
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		nodelist.Resize(0);
		el->BuildConnectList(nodelist);
		numnod = nodelist.NElements();
		for (int in=0; in<numnod; ++in) {
			int dfnindex = nodelist[in];
			TPZConnect *dfn = &fConnectVec[dfnindex];
			dfn->IncrementElConnected();
		}
	}
}

void TPZCompMesh::ComputeNodElCon(TPZVec<int> &nelconnected ) const {
	
	int i, nelem = NConnects();
	nelconnected.Resize(nelem);
	nelconnected.Fill(0);
	TPZStack<int> nodelist;
	int numnod;
	// modified Philippe 22/7/97
	// in order to account for constrained nodes
	nelem = NElements();
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		nodelist.Resize(0);
		el->BuildConnectList(nodelist);
		numnod = nodelist.NElements();
		for (int in=0; in<numnod; ++in) {
			int dfnindex = nodelist[in];
			nelconnected[dfnindex]++;
		}
	}
}


int TPZCompMesh::NEquations() {
	
	int neq = 0;
	int i, ncon = NConnects();
	for(i=0; i<ncon; i++) {
		TPZConnect &df = fConnectVec[i];
		if(df.HasDependency() || df.IsCondensed() || !df.NElConnected() || df.SequenceNumber() == -1) continue;
        int dofsize = df.NShape()*df.NState();
#ifdef DEBUG
        // check the consistency between the block size and the data structure of the connect
        {
            int seqnum = df.SequenceNumber();
            int blsize = fBlock.Size(seqnum);
            if (blsize != dofsize) {
                DebugStop();
            }
        }
#endif
        neq += dofsize;
	}
	return neq;
}

/** Este metodo nao e apropriado para aproximantes descontinuas */
int TPZCompMesh::BandWidth() {
	
	int bw = 0;
	TPZStack<int> connectlist;
	// modified Philippe 24/7/97
	// in order to take dependent nodes into account
	
	int i, nelem = NElements();
	for(i=0; i<nelem; i++) {
		connectlist.Resize(0);
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		el->BuildConnectList(connectlist);
		int nnod = connectlist.NElements();
		if(!nnod) continue;
        // look for a node which has equations associated with it
		int ifirstnode = 0;
		TPZConnect *np = &fConnectVec[connectlist[ifirstnode++]];
		while(ifirstnode < nnod && (np->HasDependency() || np->IsCondensed() || !fBlock.Size(np->SequenceNumber()))) {
			np = &fConnectVec[connectlist[ifirstnode++]];
		}
		int ibl = np->SequenceNumber();
		int loweq = fBlock.Position(ibl);
		int higheq = loweq+fBlock.Size(ibl)-1;
		for(int n=ifirstnode;n<nnod;n++) {
			np = &fConnectVec[connectlist[n]];
			if(np->HasDependency() || np->IsCondensed() ) continue;
			int ibl = np->SequenceNumber();
			if(!fBlock.Size(ibl)) continue;
			int leq = fBlock.Position(ibl);
			int heq = leq+fBlock.Size(ibl)-1;
			loweq = (loweq > leq) ? leq : loweq;
			higheq = (higheq < heq) ? heq : higheq;
		}
		int elbw = higheq - loweq;
		bw = (bw < elbw) ? elbw : bw;
	}
	return bw;
}

void TPZCompMesh::Skyline(TPZVec<int> &skyline) {
	
	TPZStack<int> connectlist;
	// modified Philippe 24/7/97
	// in order to take dependent nodes into account
	
	int neq = NEquations();
	skyline.Resize(neq);
	skyline.NElements();
	//cout << "Element Equations";
	//int eleq=0;
	int i, n, l, nelem = NElements();
	for(i=0; i<neq; i++) skyline[i] = i;
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el) continue;
		//      if(!el) continue;
		connectlist.Resize(0);
		el->BuildConnectList(connectlist);
		int nnod = connectlist.NElements();
		if(!nnod) continue;
        // look for a connect with global equations associated to it
		int ifirstnode = 0;
		TPZConnect *np = &fConnectVec[connectlist[0]];
		while(ifirstnode < nnod && (np->HasDependency() || np->IsCondensed()) ) {
			ifirstnode++;
			np = &fConnectVec[connectlist[ifirstnode]];
		}
		int ibl = np->SequenceNumber();
		int loweq = fBlock.Position(ibl);
		int higheq = loweq+fBlock.Size(ibl)-1;
		for(n=ifirstnode;n<nnod;n++) {
			np = &fConnectVec[connectlist[n]];
			if(np->HasDependency() || np->IsCondensed()) continue;
			int ibl = np->SequenceNumber();
			int leq = fBlock.Position(ibl);
			int heq = leq+fBlock.Size(ibl)-1;
			//for(int _eq=leq; _eq<= heq; _eq++) {
			//   if((eleq%20==0)) cout << endl;
			//   cout << _eq << ' ';
			//   eleq++;
			//}
			loweq = (loweq > leq) ? leq : loweq;
			higheq = (higheq < heq) ? heq : higheq;
		}
		//cout << endl;
		for(n=ifirstnode;n<nnod;n++) {
			np = &fConnectVec[connectlist[n]];
			if(np->HasDependency() || np->IsCondensed()) continue;
			int ibl = np->SequenceNumber();
			int leq = fBlock.Position(ibl);
			int heq = leq+fBlock.Size(ibl);
			for(l=leq;l<heq;l++) {
				skyline[l] = skyline[l] < loweq ? skyline[l] : loweq;
			}
		}
	}
}

void TPZCompMesh::BuildTransferMatrix(TPZCompMesh &coarsemesh, TPZTransfer<STATE> &transfer) {
	
	//TPZBlock<REAL> &localblock = Block();
	TPZBlock<STATE> &localblock = Block();
	//TPZBlock<REAL> &coarseblock = coarsemesh.Block();
	TPZBlock<STATE> &coarseblock = coarsemesh.Block();
	// adapt the block size of the blocks, dividing by the number of variables
	//  of the material
	int i, nmat = NMaterials();
	if(!nmat) {
		PZError << "TPZCompMesh::BuildTransferMatrix, no material object found\n";
		return;
	}
	TPZMaterial * mat;
	mat = fMaterialVec.begin()->second;
	int nvar = mat->NStateVariables();
	int dim = mat->Dimension();
	
	transfer.SetBlocks(localblock,coarseblock,nvar,NIndependentConnects(),coarsemesh.NIndependentConnects());
	Reference()->ResetReference();
	coarsemesh.LoadReferences();
	int nelem = NElements();
	for(i=0; i<nelem; i++) {
		if(!fElementVec[i]) continue;
		TPZInterpolationSpace * finecel = dynamic_cast<TPZInterpolationSpace *> (fElementVec[i]);
		if(!finecel) continue;
		if(finecel->Dimension() != dim) continue;
		TPZGeoEl *finegel = finecel->Reference();
		TPZGeoEl *coarsegel = finegel;
		if(!finegel) {
			cout << "TPZCompMesh::BuildTransferMatrix is not implemented for super elements\n";
			continue;
		}
		
		while(coarsegel && !coarsegel->Reference()) {
			coarsegel = coarsegel->Father();
		}
		if(!coarsegel) {
			cout << "TPZCompMesh::BuildTransferMatrix corresponding coarse element not found\n";
			finecel->Print(cout);
			continue;
		}
		
		TPZInterpolationSpace * coarsel = dynamic_cast<TPZInterpolationSpace *> ( coarsegel->Reference() );
		if(!coarsel) continue;
		
		if(coarsel->Mesh() != &coarsemesh) {
			cout << "TPZCompMesh::BuildTransferMatrix is not implemented for transfers"
			" between superelements\n";
			continue;
		}
		TPZTransform t(coarsel->Dimension());
		t=finegel->BuildTransform2(finegel->NSides()-1,coarsegel,t);
		finecel->BuildTransferMatrix(*coarsel,t,transfer);
	}
}

void TPZCompMesh::BuildTransferMatrixDesc(TPZCompMesh &transfermesh,
										  TPZTransfer<STATE> &transfer) {
#ifdef STATE_COMPLEX
	DebugStop();
#else
	//TPZBlock<REAL> &localblock = Block();
	TPZBlock<STATE> &localblock = Block();
	//TPZBlock<REAL> &transferblock = transfermesh.Block();
	TPZBlock<STATE> &transferblock = transfermesh.Block();
	// adapt the block size of the blocks, dividing by the number of variables
	//  of the material
	int i, nmat = NMaterials();
	if(!nmat) {
		PZError << "TPZCompMesh::BuildTransferMatrixDesc no material object found\n";
		return;
	}
	TPZMaterial * mat;
	mat = fMaterialVec.begin()->second;
	int nvar = mat->NStateVariables();
	int dim = mat->Dimension();
	//o seguinte �igual ao nmero de conects da malha
	int ncon = NIndependentConnects(),coarncon = transfermesh.NIndependentConnects();
	transfer.SetBlocks(localblock,transferblock,nvar,ncon,coarncon);
	Reference()->ResetReference();//geom�ricos apontam para nulo
	transfermesh.LoadReferences();
	//geom�ricos apontam para computacionais da malha atual
	TPZAgglomerateElement *aggel = 0;
	TPZAdmChunkVector<TPZCompEl *> &elvec = transfermesh.ElementVec();
	int nelem = elvec.NElements();
	
	for(i=0; i<nelem; i++) {
		TPZCompEl *comp = elvec[i];
		if(!comp) continue;
		if(comp->Dimension() != dim) continue;
		if(comp->Type() != EAgglomerate){
			PZError << "TPZCompMesh::BuildTransferMatrixDesc mesh agglomerated"
			<< " with element of volume not agglomerated\n";
			continue;
		}
		aggel = dynamic_cast<TPZAgglomerateElement *>(comp);
		//TPZStack<int> elvec;
		//retorna todos os descont�uos aglomerados por aggel
		//aggel->IndexesDiscSubEls(elvec);
		//int size = elvec.NElements(),i;
		int size = aggel->NIndexes(),i;
		for(i=0;i<size;i++){
			//TPZCompElDisc *disc = dynamic_cast<TPZCompElDisc *>(fElementVec[elvec[i]]);
			//      TPZCompElDisc *disc = dynamic_cast<TPZCompElDisc *>(aggel->FineElement(i));
			TPZCompElDisc *disc = dynamic_cast<TPZCompElDisc *>(aggel->SubElement(i));
			if(!disc){
				PZError << "TPZCompMesh::BuildTransferMatrixDesc index with null"
				<< " elemento\n";
				continue;
			}
			if(disc->Type() != EDiscontinuous) {
				PZError << "TPZCompMesh::BuildTransferMatrixDesc index of not"
				<< " discontinous element\n";
				continue;
			}
			disc->BuildTransferMatrix(*aggel,transfer);
		}
	}
#endif
}

/*
 void TPZCompMesh::CreateConnectBC() {
 TPZGeoMesh *geo = Reference();
 TPZAdmChunkVector<TPZGeoNodeBC> *geobndcondvec = &geo->BCNodeVec();
 int ibc, nnodebc = geobndcondvec->NElements();
 for(ibc=0; ibc<nnodebc; ibc++) {
 TPZGeoNodeBC &gbc = (*geobndcondvec)[ibc];
 int bcnumber = gbc.fBCId;
 TPZMaterial *bc = FindMaterial(bcnumber);
 if(!bc) continue;
 // find a geometric element which has a
 //      corresponding computational element
 TPZGeoElSide gel(gbc.fGeoEl,gbc.fGeoElSide);
 TPZStack<TPZGeoElSide> neighbourset;
 gel.AllNeighbours(neighbourset);
 int in=0,nneigh;
 nneigh = neighbourset.NElements();
 while(in<nneigh && !neighbourset[in].Reference().Exists()) in++;
 //     neighbour = gel.Neighbour();
 //     while(neighbour.Exists() && neighbour != gel && !neighbour.Reference().Exists()) {
 //       neighbour = neighbour.Neighbour();
 //     }
 //     if(!neighbour.Exists() || ! neighbour.Reference().Exists()) continue;
 if(in == nneigh) continue;
 TPZCompElSide cel = neighbourset[in].Reference();
 TPZConnect *df = &cel.Element()->Connect(neighbourset[in].Side());
 if(!df) continue;
 TPZConnectBC dfbc(df,(TPZBndCond *) bc);
 int key = BCConnectVec().AllocateNewElement();
 BCConnectVec()[key] = dfbc;
 }
 }
 */

/*
 void TPZCompMesh::ComputeConnecttoElGraph(TPZVec<int> &firstel, TPZVec<int> &connectelgraph){
 int connectstackstore[50];
 TPZStack<int> connectstack(connectstackstore,50);
 int i, ncon = NConnects();
 firstel.Resize(ncon+1);
 firstel[0] = 0;
 for(i=0; i<ncon; i++) {
 TPZConnect &c = fConnectVec[i];
 int seqnum = c.SequenceNumber();
 if(seqnum == -1) {
 firstel[i+1] = firstel[i];
 } else {
 firstel[i+1] = firstel[i] + c.NElConnected();
 }
 }
 connectelgraph.Resize(firstel[ncon]);
 connectelgraph.Fill(-1);
 int nelem = NElements();
 for(i=0; i<nelem; i++) {
 TPZCompEl *el = fElementVec[i];
 if(!el) continue;
 connectstack.Resize(0);
 el->BuildConnectList(connectstack);
 int in;
 ncon = connectstack.NElements();
 for(in=0; in<ncon; in++) {
 int ic = connectstack[in];
 int first = firstel[ic];
 int last = firstel[ic+1];
 while(connectelgraph[first] != -1 && first < last) first++;
 if(first == last) {
 PZError << "TPZCompMesh::ComputeConnecttoElGraph wrong data structure\n";
 continue;
 }
 connectelgraph[first] = i;
 }
 }
 }
 */
int TPZCompMesh::NIndependentConnects() {
	int i, ncon = NConnects();
	int NIndependentConnects = 0;
	for(i=0; i<ncon; i++) {
		TPZConnect &c = fConnectVec[i];
		if(c.HasDependency() || c.IsCondensed() || c.SequenceNumber() == -1) continue;
		NIndependentConnects++;
	}
	return NIndependentConnects;
}

void TPZCompMesh::ComputeElGraph(TPZStack<int> &elgraph, TPZVec<int> &elgraphindex){
	int i, ncon;
	TPZStack<int> connectstack;
	int nelem = NElements();
	elgraphindex.Resize(nelem+1);
	elgraphindex[0] = 0;
	elgraph.Resize(0);
	int curel=0;
	for(i=0; i<nelem; i++) {
		TPZCompEl *el = fElementVec[i];
		if(!el){
			elgraphindex[curel+1]=elgraph.NElements();
			curel++;
			continue;
		}
		connectstack.Resize(0);
		el->BuildConnectList(connectstack);
		int in;
		ncon = connectstack.NElements();
		for(in=0; in<ncon; in++) {
			int ic = connectstack[in];
			TPZConnect &c = fConnectVec[ic];
			if(c.HasDependency() || c.IsCondensed()) continue;
			//      if(fBlock.Size(c.SequenceNumber()))
			//      {
			elgraph.Push(c.SequenceNumber());
			//      }
		}
		elgraphindex[curel+1]=elgraph.NElements();
		curel++;
	}
	elgraphindex.Resize(curel+1);
}

void TPZCompMesh::Divide(int index,TPZVec<int> &subindex,int interpolate) {
	
	TPZCompEl * el = fElementVec[index];
	if (!el) {
		PZError << "TPZCompMesh::Divide element not found index = " << index << endl;
		subindex.Resize(0);
		return;
	}
	if(index != el->Index()){
		PZError << "TPZCompMesh::Divide - element->Index() != index " << endl;
		subindex.Resize(0);
		return;
	}
	
	
	el->Divide(index,subindex,interpolate);
}

void TPZCompMesh::Coarsen(TPZVec<int> &elements, int &index, bool CreateDiscontinuous) {
	int i;
	const int nelem = elements.NElements();
	
	if(!nelem) {
		index = -1;
		return;
	}//if
	
	TPZGeoEl *father = 0;
	TPZCompEl *cel = fElementVec[elements[0]];
	if (!cel) {
		index = -1;
		return;
	}//if
	
	if(cel) father = cel->Reference()->Father();
	if(!father) {
		index = -1;
		return;
	}//if
	
	for(i=1;i<nelem;i++) {
		if(!fElementVec[elements[i]]) {
			index = -1;
			return;
		}//if
		
		TPZGeoEl *father2 = fElementVec[elements[i]]->Reference()->Father();
		if(!father2 || father != father2) {
			index = -1;
			return;
		}//if
		
	}//for i
	
	if(nelem != father->NSubElements()){
		cout << "TPZCompEl::Coarsen : incomplete list of elements sons\n";
	}//if
	
	for(i=0; i<nelem; i++) {
		TPZInterpolationSpace * cel = dynamic_cast<TPZInterpolationSpace*>(this->ElementVec()[elements[i]]);
		if (!cel) continue;
		cel->RemoveInterfaces();
		TPZInterpolatedElement * intel = dynamic_cast<TPZInterpolatedElement *>(cel);
		if (intel){
			intel->RemoveSideRestraintsII(TPZInterpolatedElement::EDelete);
		}//if (intel)
		
		cel->Reference()->ResetReference();
	}//for
	
	
	for(i=0; i<nelem; i++) {
		TPZCompEl * cel = this->ElementVec()[elements[i]];
		if (cel) delete cel;
	}
	
	if (CreateDiscontinuous) fCreate.SetAllCreateFunctionsDiscontinuous();
	else fCreate.SetAllCreateFunctionsContinuous();
	
	TPZCompEl * newcel = CreateCompEl(father,index);
	
	TPZCompElDisc * newdisc = dynamic_cast<TPZCompElDisc*>(newcel);
	if (newdisc){
		newdisc->SetDegree( this->GetDefaultOrder() );
	}
	
}//method

void TPZCompMesh::Discontinuous2Continuous(int disc_index, int &new_index) {
	
	TPZInterpolationSpace *cel = dynamic_cast<TPZInterpolationSpace*> (fElementVec[disc_index]);
	if (!cel) {
		new_index = -1;
		return;
	}//if
	
	TPZCompElDisc * disc = dynamic_cast<TPZCompElDisc*>(cel);
	if (!disc) {
		new_index = -1;
		return;
	}//if
	
	TPZGeoEl * ref = cel->Reference();
	if (!ref){
		LOGPZ_FATAL(logger, "Computational element without reference");
		return;
	}
	cel->RemoveInterfaces();
	cel->Reference()->ResetReference();
	//  this->fElementVec[ cel->Index() ] = NULL;
	//  delete cel;
	
	fCreate.SetAllCreateFunctionsContinuous();
	TPZCompEl * newcel = CreateCompEl(ref,new_index);
	TPZInterpolatedElement * intel = dynamic_cast<TPZInterpolatedElement*>(newcel);
	intel->CreateInterfaces(false);
	
	if (!intel){
		LOGPZ_FATAL(logger, "New created element is not an interpolated element as requested.");
	}
	
	if(!ref->Reference()){
		LOGPZ_FATAL(logger, "New created element is not referenced by geometric element");
	}
	
	if (ref->Reference() != newcel){
		LOGPZ_FATAL(logger, "New created element is not the same as referenced by geometric element");
	}
	
	ExpandSolution();
	intel->InterpolateSolution(*disc);
	delete cel;
	
}//method

void TPZCompMesh::RemakeAllInterfaceElements(){
	
	int n = this->ElementVec().NElements();
	
	for(int i = 0; i < n; i++){
		TPZCompElDisc * disc = dynamic_cast<TPZCompElDisc*>( this->ElementVec()[i] );
		if (!disc) continue;
		disc->RemoveInterfaces();
	}//for
	
#ifdef DEBUG
	{
		n = this->ElementVec().NElements();
		for(int i = 0; i < n; i++){
			TPZCompEl * cel = this->ElementVec()[i];
			if (!cel) continue;
			//    MElementType type = cel->Type();
			TPZInterfaceElement * face = dynamic_cast<TPZInterfaceElement*>(cel);
			if(face){
				PZError << __PRETTY_FUNCTION__ << " - At this point no TPZInterfaceElement may exist.\n";
			}
		}
	}
#endif
	
	n = this->ElementVec().NElements();
	for(int i = 0; i < n; i++){
		TPZCompElDisc * disc = dynamic_cast<TPZCompElDisc*>( this->ElementVec()[i] );
		if (!disc) continue;
		disc->CreateInterfaces();
	}//for
	
}//method

#ifdef LOG4CXX
#include "pzsubcmesh.h"
#endif

/**ExpandSolution must be called before calling this*/
// it is a gather permutation
void TPZCompMesh::Permute(TPZVec<long> &permute) {
	
	ExpandSolution();
	//   if (permute.NElements() != fBlock.NBlocks()) {
	//     PZError << "TPZCompMesh::Permute : permute vector size not equal to fBlock size\n";
	//   }
#ifdef LOG4CXX
	{
		std::stringstream sout;
		TPZSubCompMesh *submesh = dynamic_cast<TPZSubCompMesh *> (this);
		if (submesh) {
			sout << "Index = " << submesh->Index() << " ";
		}
		sout << "Permutation " << permute;
        std::set<int> permset;
        permset.insert(&permute[0],(&permute[permute.size()-1]+1));
        sout << " permute size " << permute.size() << " no dist elements " << permset.size();
		LOGPZ_DEBUG(logger,sout.str())
	}
#endif
	int i,j;
	int permutenel = permute.NElements();
	for (i = 0; i < permutenel; i++) fBlock.Set(permute[i],fSolutionBlock.Size(i));
	fBlock.Resequence();
	if (fSolution.Rows() != 0) {
		//TPZFMatrix<REAL>	newsol(fSolution);
		TPZFMatrix<STATE> newsol(fSolution);
		for (i=0;i<fBlock.NBlocks();i++) {
			int oldpos = fSolutionBlock.Position(i);
			int newpos;
			if(i < permutenel) {
				newpos = fBlock.Position(permute[i]);
			} else {
				newpos = fBlock.Position(i);
			}
			for (j=0;j<fSolutionBlock.Size(i);j++) fSolution(newpos+j,0) = newsol(oldpos+j,0);
		}    //a sol. inicial esta em newsol
	}
	
	fSolutionBlock = fBlock;
	int ncon = NConnects();
	for(i=0; i<ncon; i++) {
		TPZConnect &df = fConnectVec[i];
		int seqnum = df.SequenceNumber();
		if(seqnum == -1) continue;
		if(seqnum < permutenel) df.SetSequenceNumber(permute[seqnum]);
	}
}

void TPZCompMesh::ConnectSolution(std::ostream & out) {
	
	out << "\n\t\tCONNECT INDEX SOLUTION:\n\n";
	int i;
	int ncon = NConnects();
	for(i=0; i<ncon; i++) {
		out << i << ") ";
		TPZConnect &df = ConnectVec()[i];
		int seqnum = df.SequenceNumber();
		if(df.NElConnected()==0) {
			out << "free node" << endl;
		} else if (seqnum < 0 || Block().Size(seqnum)==0) {
			out << "non solution connect" << endl;
		} else {
			int pos = Block().Position(seqnum);
			for(int j=0;j<Block().Size(seqnum);j++)
				out << Solution()(pos+j,0) << "  ";
			out << std::endl;
		}
	}
}

void TPZCompMesh::EvaluateError(
								void (*fp)(const TPZVec<REAL> &loc,TPZVec<STATE> &val,TPZFMatrix<STATE> &deriv),
								TPZVec<REAL> &errorSum) {
	
	errorSum.Resize(3);
	errorSum.Fill(0.);
	
	TPZManVector<REAL,3> true_error(3);
	true_error.Fill(0.);
	
	TPZBlock<REAL> *flux = 0;
	TPZCompEl *cel;
	
	//soma de erros sobre os elementos
	for(int el=0;el< fElementVec.NElements();el++) {
		cel = fElementVec[el];
		if(!cel  || cel->Material()->Id() < 0) continue;
		cel->EvaluateError(fp,true_error,flux);
		
		int nerrors = true_error.NElements();
		errorSum.Resize(nerrors,0.);
		for(int ii = 0; ii < nerrors; ii++)
			errorSum[ii] += true_error[ii]*true_error[ii];
	}
	
	int nerrors = errorSum.NElements();
	for(int ii = 0; ii < nerrors; ii++)
		errorSum[ii] = sqrt(errorSum[ii]);
}

void TPZCompMesh::AdjustBoundaryElements() {
	int changed = 1;
	while(changed) {
		changed = 0;
		int nel = fElementVec.NElements();
		int el;
		TPZVec<int> subelindex;
		for(el=0; el<nel; el++) {
			TPZStack<TPZCompElSide> elvec;
			TPZCompEl *elp = fElementVec[el];
			
			if(!elp || !dynamic_cast<TPZInterpolatedElement*>(elp) ) continue;
			
			TPZMaterial * mat = elp->Material();
			// this statement determines thata the element is associated with a boundary condition
			TPZBndCond *bnd = dynamic_cast<TPZBndCond *>(mat);
			if(!bnd) continue;
			//      if(mat && mat->Id() >= 0) continue;
			int nsides = elp->Reference()->NSides();
			int is;
			int nc = elp->Reference()->NCornerNodes();
			for(is=nc; is<nsides; is++) {
				TPZCompElSide elpside(elp,is);
				// this should never be called
				if(elp->Reference()->SideDimension(is) == 0) continue;
				elvec.Resize(0);
				// verify if there are smaller elements connected to this boundary element
				elpside.HigherLevelElementList(elvec,0,0);
				if(elvec.NElements()) {
					// the first small element
					TPZGeoElSide fatherside = elvec[0].Reference();//el BC
					// elp is the element we will eventually refine
					int face = elp->Reference()->NSides() - 1;
					// this is the volume side of the element
					while(fatherside.Exists()) {
						if(elp->Reference()->NeighbourExists(face,fatherside)) break;
						fatherside = fatherside.Father2();
					}
					// fatherside is a neighbour of the current element
					// I wouldnt know when this test could fail
					if(fatherside.Exists()) {
#ifdef LOG4CXX
						{
							std::stringstream sout;
							sout << "Dividing element " << el << " of type " << elp->Reference()->TypeName();
							LOGPZ_DEBUG(logger,sout.str().c_str());
						}
#endif
						Divide(el,subelindex,0);
						changed = 1;
						break;
					}
				}
				// we are working on the last side and nothing was divided
				if(is == nsides-1) {
					TPZInterpolatedElement *elpint = dynamic_cast <TPZInterpolatedElement *> (elp);
					if(!elpint) continue;
					int porder = elpint->PreferredSideOrder(is);
					int maxorder = 0;
					elvec.Resize(0);
					elpside.EqualLevelElementList(elvec,0,0);
					int eq;
					for(eq=0; eq<elvec.NElements(); eq++) {
						TPZInterpolatedElement *eqel = dynamic_cast<TPZInterpolatedElement *> (elvec[eq].Element());
						int eqside = elvec[eq].Side();
						if(!eqel) continue;
						if(maxorder < eqel->PreferredSideOrder(eqside)) maxorder =  eqel->PreferredSideOrder(eqside);
					}
					// set the order to the largest order of all connecting elements
					if(porder < maxorder) {
#ifdef LOG4CXX
                        if(logger->isDebugEnabled())
						{
							std::stringstream sout;
							sout << "Refining element " << el << " to order " << maxorder;
							LOGPZ_DEBUG(logger,sout.str().c_str());
						}
#endif
						elpint->PRefine(maxorder);
						changed = 1;
					}
				}
			}
		}
	}
}

int TPZCompMesh::PutinSuperMesh (int local, TPZCompMesh *super){
	if (super != this) return -1;
	else return local;
}

int TPZCompMesh::GetFromSuperMesh (int superind, TPZCompMesh *super){
	if (super != this) return -1;
	else return superind;
}

REAL TPZCompMesh::CompareMesh(int var, char *matname){
	
	REAL error = 0.;
	int i=0;
	for (i=0;i<fElementVec.NElements();i++){
		TPZCompEl *el = fElementVec[i];
		if(el) error+= el->CompareElement(var,matname);
	}
	return (error);
}

void TPZCompMesh::SetElementSolution(int i, TPZVec<REAL> &sol) {
	if(sol.NElements() != NElements()) {
		cout << "TPZCompMesh::SetElementSolution size of the vector doesn't match\n";
	}
	if(fElementSolution.Cols() <= i) fElementSolution.Resize(NElements(),i+1);
	int el,nel= NElements();
	for(el=0; el<nel; el++) {
		fElementSolution(el,i) = sol[el];
	}
}

void TPZCompMesh::GetRefPatches(std::set<TPZGeoEl *> &grpatch){
	int i;
	int nel = NElements();
	//	cout << "GetRefPatches\n" << nel << endl;
	for (i=0; i<nel; i++){
		if (fElementVec[i]){
			TPZGeoEl *gel = fElementVec[i]->Reference();
			if (gel) gel = fElementVec[i]->GetRefElPatch();
			if (gel)
			{
				grpatch.insert(gel);
			}
		}
	}
	return;
}


void  TPZCompMesh::GetNodeToElGraph(TPZVec<int> &nodtoelgraph, TPZVec<int> &nodtoelgraphindex, TPZStack<int> &elgraph, TPZVec<int> &elgraphindex){
	
	ComputeElGraph(elgraph,elgraphindex);
	
	//  int i,j;
	//  for (i=0; i<elgraphindex.NElements()-1; i++){
	//    cout << "Block: " << i << "\t";
	//    for (j = elgraphindex[i]; j<elgraphindex[i+1]; j++){
	//      cout << elgraph[j] << "\t";
	//    }
	//   cout << endl;
	//  }
	
	TPZMetis renum(elgraphindex.NElements() -1 ,NIndependentConnects());
	renum.SetElementGraph(elgraph,elgraphindex);
	renum.NodeToElGraph(elgraph,elgraphindex,nodtoelgraph, nodtoelgraphindex);
	/*   TPZRenumbering *re = &renum; */
	/*   re->Print(nodtoelgraph, nodtoelgraphindex , "Grapho de n� para elementos "); */
	
	
}


void TPZCompMesh::GetElementPatch(TPZVec<int> nodtoelgraph, TPZVec<int> nodtoelgraphindex, TPZStack<int> &elgraph, TPZVec<int> &elgraphindex,int elind ,TPZStack<int> &patch){
	
	//  int aux =0;
	//TPZAVLMap<int,int> elconmap(aux);
	std::set<int > elconmap;
	int i,j;
	for (i= elgraphindex[elind]; i<elgraphindex[elind+1];i++){
		int node = elgraph[i];
		for (j = nodtoelgraphindex[node];  j<nodtoelgraphindex[node+1]; j++){
			elconmap.insert(nodtoelgraph[j]);
			
		}
	}
	patch.Resize(0);
	
	//TPZPix iter = elconmap.First();
	set<int >::iterator iter = elconmap.begin();
	while(iter!=elconmap.end()){
		//patch.Push(elconmap.Key(iter));
		patch.Push(*iter);//elconmap.Key(iter));
		iter++;//elconmap.Next(iter);
	}
}

TPZCompMesh::TPZCompMesh(const TPZCompMesh &copy) :
fReference(copy.fReference),fConnectVec(copy.fConnectVec),
fMaterialVec(), fSolutionBlock(copy.fSolutionBlock),
fSolution(copy.fSolution), fBlock(copy.fBlock),
fElementSolution(copy.fElementSolution)
{
	fDefaultOrder = copy.fDefaultOrder;
	fReference->ResetReference();
	fBlock.SetMatrix(&fSolution);
	fSolutionBlock.SetMatrix(&fSolution);
	copy.CopyMaterials(*this);
	int nel = copy.fElementVec.NElements();
	fElementVec.Resize(nel);
	int iel;
	for(iel = 0; iel<nel; iel++) fElementVec[iel] = 0;
	for(iel = 0; iel<nel; iel++) {
		TPZCompEl *cel = copy.fElementVec[iel];
		if(cel && !dynamic_cast<TPZInterfaceElement* >(cel) )
		{
			/*TPZCompEl *clone  = */ cel->Clone(*this);
			/*#ifdef LOG4CXX
			 {
			 std::stringstream sout;
			 sout << "original\n";
			 cel->Print(sout);
			 sout << "cloned\n";
			 clone->Print(sout);
			 LOGPZ_DEBUG(logger,sout.str())
			 }
			 #endif*/
		}
	}
	/*#ifdef LOG4CXX
	 {
	 std::stringstream sout;
	 Print(sout);
	 LOGPZ_DEBUG(logger,sout.str())
	 }
	 #endif*/
	for(iel = 0; iel<nel; iel++) {
		TPZCompEl *cel = copy.fElementVec[iel];
		if(cel && dynamic_cast<TPZInterfaceElement* >(cel) ) cel->Clone(*this);
	}
	fDimModel = copy.fDimModel;
	fName = copy.fName;
}

TPZCompMesh* TPZCompMesh::Clone() const {
	return new TPZCompMesh(*this);
}
/*
 TPZGeoMesh *gmesh = Reference();
 gmesh->ResetReference();
 TPZCompMesh *cmesh = new TPZCompMesh(gmesh);
 CopyMaterials(cmesh);
 
 int iel, nel = fElementVec.NElements();
 for (iel=0;iel<nel;iel++){
 if (!fElementVec[iel]) continue;
 TPZInterpolatedElement *intel = dynamic_cast<TPZInterpolatedElement *> (fElementVec[iel]);
 
 TPZGeoEl *gel = fElementVec[iel]->Reference();
 if (!gel) continue;
 int indexcomp;
 gel->CreateCompEl(*cmesh,indexcomp);
 
 TPZInterpolatedElement *clintel = dynamic_cast<TPZInterpolatedElement *> (cmesh->ElementVec()[indexcomp]);
 int side, nsides = intel->NConnects();
 for (side =0;side<nsides;side++){
 int porder = intel->SideOrder(side);
 clintel->PRefine(side,porder);
 int clorder = clintel->SideOrder(side);
 if(porder != clorder) {
 cout << "PZCompMesh::Clone order was not honoured side " <<
 side << " porder " << porder << " clorder " << clorder <<
 endl;
 }
 int elndof = intel->Connect(side).NDof(*this);
 int clelndof = clintel->Connect(side).NDof(*cmesh);
 if(elndof != clelndof) {
 cout << "PZCompMesh::Clone ndof different side " <<
 side << " porder " << porder << " clorder " << clorder <<
 " elndof = "<< elndof << " clelndof " << clelndof << endl;
 }
 }
 }
 
 cmesh->InitializeBlock();
 
 gmesh->ResetReference();
 LoadReferences();
 
 
 int clnel = cmesh->ElementVec().NElements();
 //  if (clnel != nel){
 //    cout << "Clone mesh inconsistancy: clone elements failed!\n";
 //    return 0;
 //  }
 
 for (iel=0;iel<clnel;iel++){
 TPZCompEl *cel = cmesh->ElementVec()[iel];
 TPZGeoEl *gel = cel->Reference();
 TPZCompEl *el = gel->Reference();
 int ic,nc = el->NConnects();
 for (ic=0;ic<nc;ic++){
 int elseqnum = el->Connect(ic).SequenceNumber();
 int elndof = el->Connect(ic).NDof(*this);
 int clelseqnum = cel->Connect(ic).SequenceNumber();
 int clelndof = cel->Connect(ic).NDof(*cmesh);
 if (elndof != clelndof){
 cout << "Number of degree of freedom incompatible between clone and original mesh!\n";
 cout << "Clone connect id: " << ic << "  Clone dof: " << clelndof << "  Original dof: " << clelndof << endl;
 continue;
 }
 int idof;
 for (idof=0; idof<elndof; idof++){
 cmesh->fSolutionBlock(clelseqnum,0,idof,0) = Block()(elseqnum,0,idof,0);
 }
 }
 }
 return cmesh;
 }
 */

void TPZCompMesh::CopyMaterials(TPZCompMesh &mesh) const {
	//  int nmat = fMaterialVec.size();
	std::map<int, TPZMaterial * >::const_iterator mit;
	//  int m;
	for(mit=fMaterialVec.begin(); mit!=fMaterialVec.end(); mit++) {
		if(!dynamic_cast<TPZBndCond *> (mit->second)) mit->second->Clone(mesh.fMaterialVec);
	}
	for(mit=fMaterialVec.begin(); mit!=fMaterialVec.end(); mit++) {
		if(dynamic_cast<TPZBndCond *> (mit->second)) mit->second->Clone(mesh.fMaterialVec);
	}
	
}

REAL TPZCompMesh::DeltaX(){
	
	int nel = ElementVec().NElements(),i,j;
	if(nel == 0) cout << "\nTPZCompMesh::DeltaX no elements\n";
	REAL maxdist = 0.0,dist=0.0;
	TPZVec<REAL> point0(3),point1(3);
	TPZGeoNode *node0,*node1;
	for(i=0;i<nel;i++){
		TPZCompEl *com = ElementVec()[i];
		if(!com) continue;
		if(com->Type() == EInterface || com->Material()->Id() < 0) continue;
		node0 = com->Reference()->NodePtr(0);
		node1 = com->Reference()->NodePtr(1);
		for(j=0;j<3;j++){
			point0[j] = node0->Coord(j);
			point1[j] = node1->Coord(j);
		}
		dist = TPZGeoEl::Distance(point0,point1);
		if(dist > maxdist) maxdist = dist;
	}
	return maxdist;
}

REAL TPZCompMesh::MaximumRadiusOfMesh(){
	
	int nel = ElementVec().NElements(),i;
	if(nel == 0) cout << "\nTPZCompMesh::MaximumRadiusOfMesh no elements\n";
	REAL maxdist = 0.0,dist=0.0;
	TPZVec<REAL> point0(3),point1(3);
	for(i=0;i<nel;i++){
		TPZCompEl *com = ElementVec()[i];
		if(!com) continue;
		if(com->Type() == EInterface || com->Material()->Id() < 0) continue;
		dist = com->MaximumRadiusOfEl();
		if(dist > maxdist) maxdist = dist;
	}
	return maxdist;
}

REAL TPZCompMesh::LesserEdgeOfMesh(){
	
	int nel = ElementVec().NElements(),i;
	if(nel == 0) cout << "\nTPZCompMesh::MaximumRadiusOfMesh no elements\n";
	REAL mindist =10000.0,dist=0.0;
	for(i=0;i<nel;i++){
		TPZCompEl *com = ElementVec()[i];
		if(!com) continue;
		int type = com->Type();
		if( type == EInterface || com->Material()->Id() < 0 ) continue;
		if(type == EAgglomerate) dist = dynamic_cast<TPZCompElDisc *>(com)->LesserEdgeOfEl();
		else dist = com->LesserEdgeOfEl();
		if(dist < mindist) mindist = dist;
	}
	return mindist;
}

/** This method will fill the matrix passed as parameter with a representation of the fillin of the global stiffness matrix, based on the sequence number of the connects
 @param resolution Number of rows and columns of the matrix
 @param fillin Matrix which is mapped onto the global system of equations and represents the fillin be assigning a value between 0. and 1. in each element */
void TPZCompMesh::ComputeFillIn(int resolution, TPZFMatrix<REAL> &fillin){
	ComputeNodElCon();
	int nequations = NEquations();
	int divider = nequations/resolution;
	if(divider*resolution != nequations) divider++;
	REAL factor = 1./(divider*divider);
	fillin.Redim(resolution,resolution);
	
	TPZStack<int> graphelindex, graphel, graphnodeindex, graphnode;
	this->ComputeElGraph(graphel,graphelindex);
	TPZMetis renum(fElementVec.NElements(),fConnectVec.NElements());
	renum.ConvertGraph(graphel,graphelindex,graphnode,graphnodeindex);
	std::map<int,TPZConnect *> seqtoconnect;
	int ic,ncon = fConnectVec.NElements();
	for(ic=0; ic<ncon; ic++) {
		TPZConnect &c = fConnectVec[ic];
		if(c.HasDependency() || c.IsCondensed() || c.SequenceNumber() < 0) continue;
		seqtoconnect[c.SequenceNumber()] = &c;
	}
	int iseqnum;
	for(iseqnum = 0; iseqnum < graphnodeindex.NElements()-1; iseqnum++) {
		if(!seqtoconnect.count(iseqnum)) continue;
		int firstieq = Block().Position(iseqnum);
		int lastieq = Block().Size(iseqnum)+firstieq;
		int firstnode = graphnodeindex[iseqnum];
		int lastnode = graphnodeindex[iseqnum+1];
		{
			int ieq;
			for(ieq=firstieq; ieq<lastieq; ieq++) {
				int rowp = ieq/divider;
				int ieq2;
				for(ieq2=firstieq; ieq2<lastieq; ieq2++) {
					int rowp2 = ieq2/divider;
					fillin(rowp,rowp2) += factor;
				}
			}
		}
		int in;
		for(in=firstnode; in<lastnode; in++) {
			int jseqnum = graphnode[in];
			int firstjeq = Block().Position(jseqnum);
			int lastjeq = Block().Size(jseqnum)+firstjeq;
			int ieq;
			for(ieq=firstieq; ieq<lastieq; ieq++) {
				int rowp = ieq/divider;
				int jeq;
				for(jeq=firstjeq; jeq<lastjeq; jeq++) {
					int colp = jeq/divider;
					fillin(rowp,colp) += factor;
				}
			}
		}
	}
}
void TPZCompMesh::ProjectSolution(TPZFMatrix<STATE> &projectsol) {
	
	//  * * A MALHA ATUAL DEVE SER AGLOMERADA * * *
	
	//   TPZBlock &localblock = Block();
	//   TPZBlock &transferblock = finemesh.Block();
	// adapt the block size of the blocks, dividing by the number of variables
	//  of the material
	int neq = NEquations();
	projectsol.Redim(neq,1);
	projectsol.Zero();
	int nmat = NMaterials();
	if(!nmat) {
		PZError << "TPZCompMesh::BuildTransferMatrixDesc2 no material object found\n";
		return;
	}
	TPZMaterial * mat = fMaterialVec.begin()->second;
	//int nvar = mat->NStateVariables();
	Reference()->ResetReference();//geom�ricos apontam para nulo
	LoadReferences();
	
#ifdef STATE_COMPLEX
	DebugStop();
#else
	//geom�ricos apontam para computacionais da malha atual
    int dim = mat->Dimension();
	TPZAgglomerateElement *aggel = 0;
	TPZAdmChunkVector<TPZCompEl *> &elvec = ElementVec();
	int nelem = elvec.NElements();
	
	
	for(int i=0; i<nelem; i++) {
		TPZCompEl *comp = elvec[i];
		if(!comp) continue;
		if(comp->Dimension() != dim) continue;
		if(comp->Type() != EAgglomerate){
			PZError << "TPZCompMesh::BuildTransferMatrixDesc2 mesh agglomerated"
			<< " with element of volume not agglomerated\n";
			continue;
		}
		aggel = dynamic_cast<TPZAgglomerateElement *>(comp);
		aggel->ProjectSolution(projectsol);
	}
#endif
}
/**
 * returns the unique identifier for reading/writing objects to streams
 */
int TPZCompMesh::ClassId() const
{
	return TPZCOMPMESHID;
}
/**
 Save the element data to a stream
 */
void TPZCompMesh::Write(TPZStream &buf, int withclassid)
{
	TPZSaveable::Write(buf,withclassid);
	//Reference()->Write(buf,1);
	buf.Write(&fName,1);
	buf.Write(&fDimModel,1);
	TPZSaveable::WriteObjects<TPZConnect>(buf,fConnectVec);
	std::map<int,TPZMaterial * >::iterator it;
	std::map<int,TPZMaterial * > temp1,temp2;
	for(it=fMaterialVec.begin(); it!=fMaterialVec.end(); it++)
	{
		if(dynamic_cast<TPZBndCond *>(it->second))
		{
			temp1[it->first]=it->second;
		}
		else
		{
			temp2[it->first]=it->second;
		}
	}
	WriteObjectPointers<TPZMaterial>(buf,temp2);
	WriteObjectPointers<TPZMaterial>(buf,temp1);
    
	WriteObjectPointers<TPZCompEl>(buf,fElementVec);
	fSolution.Write(buf,0);
	fSolutionBlock.Write(buf,0);
	fBlock.Write(buf,0);
	
}

/**
 Read the element data from a stream
 */
void TPZCompMesh::Read(TPZStream &buf, void *context)
{
	TPZSaveable::Read(buf,context);
	TPZGeoMesh *gmesh;
    //	TPZSaveable *obj = TPZSaveable::Restore(buf,0);
	gmesh = (TPZGeoMesh *)(context);
    //	context = gmesh;
	
	this->fReference = (TPZGeoMesh *) context;
	LoadReferences();
	Reference()->RestoreReference(this);
	
	buf.Read(&fName,1);
	
	buf.Read(&fDimModel,1);
	ReadObjects<TPZConnect>(buf,fConnectVec,0);
	// first the material objects, then the boundary conditions
	ReadObjectPointers<TPZMaterial>(buf,fMaterialVec,this);
	ReadObjectPointers<TPZMaterial>(buf,fMaterialVec,this);
	
	ReadObjectPointers<TPZCompEl>(buf,fElementVec,this);
	fSolution.Read(buf,0);
	fSolutionBlock.Read(buf,&fSolution);
	fBlock.Read(buf,&fSolution);
}

#include "TPZGeoElement.h"
#include "pzgeoelrefless.h"
#include "TPZGeoCube.h"
#include "pzshapecube.h"
#include "TPZRefCube.h"
#include "pzshapelinear.h"
#include "TPZGeoLinear.h"
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


void TPZCompMesh::ConvertDiscontinuous2Continuous(REAL eps, int opt, int dim, TPZVec<STATE> &celJumps){
	const int nelements = this->NElements();
	celJumps.Resize(nelements);
    int numbersol = Solution().Cols();
	TPZVec<TPZCompEl*> AllCels(nelements);
	AllCels.Fill(NULL);
	celJumps.Fill(0.0);
	
	for(int i = 0; i < nelements; i++){
		TPZCompEl * cel = this->ElementVec()[i];
		if (!cel) continue;
		TPZInterfaceElement * face = dynamic_cast<TPZInterfaceElement *>(cel);
		if (!face){
			AllCels[i] = cel;
			continue;
		}
		TPZSolVec facejump;
		face->EvaluateInterfaceJump(facejump,opt);
		const int leftel  = face->LeftElement()->Index();
		const int rightel = face->RightElement()->Index();
#ifdef DEBUG
		if(this->ElementVec()[leftel]  != face->LeftElement()){
			LOGPZ_FATAL(logger, "inconsistent data structure");
			DebugStop();
		}
		if(this->ElementVec()[rightel] != face->RightElement()){
			LOGPZ_FATAL(logger, "inconsistent data structure");
			DebugStop();
		}
#endif
		
		STATE jumpNorm = 0.;
        for (int is=0; is<numbersol; is++) {
            for(int ij = 0; ij < facejump.NElements(); ij++){
                jumpNorm += facejump[is][ij]*facejump[is][ij];
            }
        }
		jumpNorm = sqrt(jumpNorm);
		
		celJumps[leftel] += jumpNorm;
		celJumps[rightel] += jumpNorm;
	}//for i
	
	for(int i = 0; i < nelements; i++){
		if (!AllCels[i]) continue;
		TPZCompElDisc * disc = dynamic_cast<TPZCompElDisc*>(AllCels[i]);
		if (!disc) continue;
		if(disc->Reference()->Dimension() != dim) continue;
		const STATE celJumpError = celJumps[i];
		//const STATE celJumpError = celJumps[i];
		if (abs(celJumpError) < eps){
			int index;
			this->Discontinuous2Continuous(i, index);
		}//if
	}//for i
	
	this->CleanUpUnconnectedNodes();
	this->AdjustBoundaryElements();
	
}//method

void TPZCompMesh::AssembleError(TPZFMatrix<REAL> &estimator, int errorid){
	int iel, i;
	const int nelem = this->NElements();
	TPZManVector<REAL> locerror(7), errorL(7), errorR(7);
	
	estimator.Resize(nelem, 7);
	estimator.Zero();
	for(iel=0; iel < nelem; iel++) {
		TPZCompEl *el = fElementVec[iel];
		if (!el) continue;
		TPZInterfaceElement * face = dynamic_cast<TPZInterfaceElement*>(el);
		if (face){
			errorL.Fill(0.); errorR.Fill(0.);
			face->ComputeErrorFace(errorid, errorL, errorR);
			int n = errorL.NElements();
			if (errorR.NElements() > n) n = errorR.NElements();
			//if number of errors > 1 then resize matrix.
			//Method Resize keeps previous values and zero new values.
			//estimator.Resize(nelem, n);
			for(i = 0; i < errorL.NElements()-1; i++){
				estimator(face->LeftElement()->Index(),i) += errorL[i];
			}//for i
			for(i = 0; i < errorR.NElements()-1; i++){
				estimator(face->RightElement()->Index(),i) += errorR[i];
			}//for i
		}//if
		else{
			locerror.Fill(0.);
			el->ComputeError(errorid, locerror);
			//if number of errors > 1 then resize matrix.
			//Method Resize keeps previous values and zero new values.
			//estimator.Resize(nelem, locerror.NElements());
			for(i = 0; i < locerror.NElements()-1; i++){
				estimator(iel, i) += locerror[i];
			}//for i
			//        estimator.Print("Estimator", std::cout, EFormatted);
			
		}//else
	}
	for(iel=0; iel < nelem; iel++) {
		if(fabs(estimator(iel,5))>=0.0000001){  // Possivel erro ao plotar os indíces de efetividade
            estimator(iel,6) = estimator(iel,2)/estimator(iel,5) ;
		}
	}
	
}

void TPZCompMesh::SaddlePermute()
{
    
#ifdef LOG4CXX
    {
        std::stringstream sout;
        sout<< "Implementando permutacao para problemas de ponto de sela"<< std::endl;
        LOGPZ_DEBUG(logger, sout.str().c_str());
    }
#endif
    TPZVec<long> permute;
    int numinternalconnects = NIndependentConnects();
    permute.Resize(numinternalconnects,0);
    
    TPZSubCompMesh *submesh = dynamic_cast<TPZSubCompMesh *> (this);
    if(submesh)
    {
        int nexternal = submesh->NConnects();
        numinternalconnects -= nexternal;
    }
    //	else {
    //		DebugStop();
    //	}
    
    int jperm=0;
    int nel=ElementVec().NElements();
    for (int jel=0; jel<nel; jel++) {
        
        for (long ip=0; ip<permute.NElements(); ip++) {
            permute[ip]=ip;
        }
        
        TPZCompEl *elvec= ElementVec()[jel];
        //	int idtroca=0;
        int eqmax=0;
        if(!elvec)continue;
//        int ncon=elvec->NConnects();
        std::set<int> connects;
        elvec->BuildConnectList(connects );
        //	if(ncon==1) continue;
        int pressureconectindex = elvec->PressureConnectIndex();
        if(pressureconectindex == -1) continue;
        long eqpress=elvec->Connect(pressureconectindex).SequenceNumber();

        for (std::set<int>::const_iterator it= connects.begin(); it != connects.end(); it++) {
//        for (int icon=0; icon< ncon-1; icon++) {
            if(*it == pressureconectindex) continue;
            TPZConnect &coel= fConnectVec[*it];
            if(coel.HasDependency()) continue;
            int eqflux=coel.SequenceNumber();
            eqmax = max(eqmax,eqflux);
        }
        
        
        if(eqpress<eqmax) {
            
            permute[eqpress]=eqmax;
            
        }
        
        
        for ( jperm = eqpress+1; jperm<=eqmax; jperm++) {
            permute[jperm]=jperm-1;
            
        }
        /*
         #ifdef LOG4CXX
         {
         std::stringstream sout;
         sout << "vetor SaddlePermute  do elemento - "<<jel<< " - " <<permute;
         LOGPZ_DEBUG(logger, sout.str().c_str());
         }
         #endif
         */
        Permute(permute);
        
    }		
}

/** @brief adds the connect indexes associated with base shape functions to the set */
void TPZCompMesh::BuildCornerConnectList(std::set<int> &connectindexes) const
{
    int nel = NElements();
    for (int el=0; el<nel ; el++) {
        TPZCompEl *cel = ElementVec()[el];
        if (!cel) {
            continue;
        }
        cel->BuildCornerConnectList(connectindexes);
    }
}

