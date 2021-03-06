#include "TPZHCurlNedFLinEl.h"
#include "pzmaterial.h"
using namespace pzshape;

#ifdef LOG4CXX
static LoggerPtr logger(Logger::getLogger("pz.mesh.TPZCompElHDivBound2"));
#endif

TPZHCurlNedFLinEl::TPZHCurlNedFLinEl(TPZCompMesh &mesh, TPZGeoEl *gel,
                                     long &index)
    : TPZInterpolatedElement(mesh, gel, index), fSideOrient(1) {
    this->TPZInterpolationSpace::fPreferredOrder = mesh.GetDefaultOrder();
    gel->SetReference(this);
    fConnectIndexes.resize(1);

    this->fConnectIndexes[0] =
        this->CreateMidSideConnect(TPZShapeLinear::NSides - 1);
#ifdef LOG4CXX
    if (logger->isDebugEnabled()) {
        std::stringstream sout;
        sout << "After creating boundary hcurl connect "
             << this->fConnectIndexes[0] << std::endl;
        LOGPZ_DEBUG(logger, sout.str())
    }
#endif

    mesh.ConnectVec()[this->fConnectIndexes[0]].IncrementElConnected();

#ifdef LOG4CXX
    if (logger->isDebugEnabled()) {
        std::stringstream sout;

        sout << std::endl << " Creating Connects: " << std::endl;
        for (int j = 0; j < NConnects(); j++) {
            sout << " " << this->fConnectIndexes[j];
        }
        LOGPZ_DEBUG(logger, sout.str())
    }
#endif
    int sideorder = EffectiveSideOrder(TPZShapeLinear::NSides - 1);
    sideorder = 2 * sideorder; // TODO:WHY?
    if (sideorder > this->fIntRule.GetMaxOrder())
        sideorder = this->fIntRule.GetMaxOrder();

    TPZManVector<int, 3> order(3, sideorder);

    this->fIntRule.SetOrder(order);

#ifdef LOG4CXX
    if (logger->isDebugEnabled()) {
        std::stringstream sout;
        sout << "Finalizando criacao do elemento ";
        this->Print(sout);
        LOGPZ_DEBUG(logger, sout.str())
    }
#endif
}

TPZHCurlNedFLinEl::TPZHCurlNedFLinEl(TPZCompMesh &mesh,
                                     const TPZHCurlNedFLinEl &copy) {
    DebugStop();
}

TPZHCurlNedFLinEl::TPZHCurlNedFLinEl(TPZCompMesh &mesh,
                                     const TPZHCurlNedFLinEl &copy,
                                     std::map<long, long> &gl2lcElMap) {
    DebugStop();
}

TPZHCurlNedFLinEl::TPZHCurlNedFLinEl(TPZCompMesh &mesh,
                                     const TPZHCurlNedFLinEl &copy,
                                     std::map<long, long> &gl2lcConMap,
                                     std::map<long, long> &gl2lcElMap) {
    DebugStop();
}

TPZHCurlNedFLinEl::TPZHCurlNedFLinEl() { DebugStop(); }
/** @brief Destructor, does nothing */
TPZHCurlNedFLinEl::~TPZHCurlNedFLinEl() {
    TPZGeoEl *gel = this->Reference();
    if (gel && gel->Reference() != this) {
        return;
    }
    int side = TPZShapeLinear::NSides-1;
    TPZGeoElSide gelside(this->Reference(),side);
    TPZStack<TPZCompElSide> celstack;
    TPZCompElSide largecel = gelside.LowerLevelCompElementList2(0);
    if (largecel) {
        int cindex = SideConnectLocId(0, side);
        TPZConnect &c = this->Connect(cindex);
        c.RemoveDepend();
    }
    gelside.HigherLevelCompElementList3(celstack, 0, 1);
    long ncel = celstack.size();
    for (long el=0; el<ncel; el++) {
        TPZCompElSide celsidesmall = celstack[el];
        TPZGeoElSide gelsidesmall = celsidesmall.Reference();
        if (gelsidesmall.Dimension() != gel->Dimension()) {
            continue;
        }
        TPZCompEl *cel = celsidesmall.Element();
        TPZInterpolatedElement *intel = dynamic_cast<TPZInterpolatedElement *>(cel);
        if (!intel) {
            DebugStop();
        }
        int cindex = intel->SideConnectLocId(0, celsidesmall.Side());
        TPZConnect &c = intel->Connect(cindex);
        c.RemoveDepend();
    }
    if (gel){
        gel->ResetReference();
    }
}

TPZHCurlNedFLinEl *TPZHCurlNedFLinEl::Clone(TPZCompMesh &mesh) const {
    DebugStop();
    return nullptr;
}

TPZHCurlNedFLinEl *
TPZHCurlNedFLinEl::ClonePatchEl(TPZCompMesh &mesh,
                                std::map<long, long> &gl2lcConMap,
                                std::map<long, long> &gl2lcElMap) const {
    DebugStop();
    return nullptr;
}

int TPZHCurlNedFLinEl::Dimension() const { return TPZShapeLinear::Dimension; }

int TPZHCurlNedFLinEl::NCornerConnects() const { return 0; }

int TPZHCurlNedFLinEl::NConnects() const { return 1; }

long TPZHCurlNedFLinEl::ConnectIndex(int i) const { return fConnectIndexes[i]; }

void TPZHCurlNedFLinEl::SetConnectIndex(int i, long connectindex) {
    if (i) // just one connect
    {
        DebugStop();
    }
    this->fConnectIndexes[i] = connectindex;
}

int TPZHCurlNedFLinEl::NSideConnects(int side) const {
    if (side == TPZShapeLinear::NSides - 1) {
        return 1;
    }
    return 0;
}

int TPZHCurlNedFLinEl::SideConnectLocId(int con, int side) const {
    if (side == TPZShapeLinear::NSides - 1 && con == 0) {
        return 0;
    } else {
        return -1;
    }
}

int TPZHCurlNedFLinEl::NConnectShapeF(int con, int order) const {
#ifdef PZDEBUG
    if (con != 0)
        DebugStop();
    if (order < 0)
        DebugStop();
#endif
    return order;
}

void TPZHCurlNedFLinEl::SideShapeFunction(int side, TPZVec<REAL> &point,
                                          TPZFMatrix<REAL> &phi,
                                          TPZFMatrix<REAL> &curlPhi) {
    int nc = TPZShapeLinear::NContainedSides(side);
    int nn = TPZShapeLinear::NSideNodes(side);
    nc -= nn;//nedelec elements of the 1st kind dont have
    // connects associated to sides with dim == 0
    TPZManVector<long,27> id(nn);
    TPZManVector<int,27> order(nc);
    TPZManVector<int,27> nShapeF(nc);
    int n,c;
    TPZGeoEl *ref = Reference();
    for (n=0;n<nn;n++){
        int nodloc = TPZShapeLinear::SideNodeLocId(side,n);
        id [n] = ref->NodePtr(nodloc)->Id();
    }
    for (c=0;c<nc;c++){
        int conloc = SideConnectLocId(c,side);
        order[c] = Connect(conloc).Order();
        nShapeF[c] = NConnectShapeF(conloc, order[c]);
    }
    //calculate jacobian for Piola mapping
    TPZGeoElSide gelside = TPZGeoElSide(this->Reference(),side);
    int dim = this->Reference()->SideDimension(side);
    TPZFNMatrix<9,REAL> jac(dim,dim),jacinv(dim,dim),axes(dim,3);
    REAL detjac;
    gelside.Jacobian(point, jac, axes, detjac, jacinv);
    TPZFMatrix<REAL> phiHat(phi);
    TPZFMatrix<REAL> curlPhiHat(curlPhi);
    TPZHCurlNedFLinEl::CalcShape(point,phiHat,curlPhiHat,order,nShapeF);
    const int firstSide = TPZShapeLinear::NSides - TPZShapeLinear::NFaces - 1;
    int iPhi = 0;
    for (int iCon = 0; iCon < order.size(); iCon++) {
        const int sideIt =
                iCon + TPZShapeLinear::NSides -
                TPZShapeLinear::NumSides(TPZShapeLinear::Dimension - 1) - 1;
        const int orient =
                sideIt == firstSide + 3 ? 1 : fSideOrient;
        const int nShapeConnect = nShapeF[iCon];
        for (int iPhiAux = 0; iPhiAux < nShapeConnect; iPhiAux++) {
            const int funcOrient = (iPhiAux % 2)*orient + ((iPhiAux+1) % 2)*1;
            phi(iPhi, 0) = funcOrient * jacinv.GetVal(0, 0) * phiHat.GetVal(iPhi, 0);
            iPhi++;
        }
    }
}

void TPZHCurlNedFLinEl::SetIntegrationRule(int ord) {
    TPZManVector<int, 3> order(TPZShapeLinear::Dimension, ord);
    fIntRule.SetOrder(order);
}

const TPZIntPoints &TPZHCurlNedFLinEl::GetIntegrationRule() const {
    if (fIntegrationRule) {
        return *fIntegrationRule;
    } else {
        return fIntRule;
    }
}

TPZIntPoints &TPZHCurlNedFLinEl::GetIntegrationRule() {
    if (fIntegrationRule) {
        return *fIntegrationRule;
    } else {
        return fIntRule;
    }
}

void TPZHCurlNedFLinEl::SetPreferredOrder(int order) {
#ifdef PZDEBUG
    if(order<1) DebugStop();
#endif
    this->fPreferredOrder = order;
}

void TPZHCurlNedFLinEl::GetInterpolationOrder(TPZVec<int> &ord) { DebugStop(); }

int TPZHCurlNedFLinEl::PreferredSideOrder(int side) {
    if (side != TPZShapeLinear::NSides - 1) {
        PZError << __PRETTY_FUNCTION__ << " side " << side << std::endl;
    }
    int connect = 0;
    int order = this->fPreferredOrder;
    return this->AdjustPreferredSideOrder(connect, order);
}

int TPZHCurlNedFLinEl::ConnectOrder(int connect) const {
    if (connect < 0 || connect >= this->NConnects()) {
#ifdef LOG4CXX
        {
            std::stringstream sout;
            sout << "Connect index out of range connect " << connect
                 << " nconnects " << NConnects();
            LOGPZ_DEBUG(logger, sout.str())
        }
#endif
        return -1;
    }

    if (this->fConnectIndexes[connect] == -1) {
        std::stringstream sout;
        sout << __PRETTY_FUNCTION__ << " connect " << connect
             << " is not initialized" << std::endl;
#ifdef LOG4CXX
        LOGPZ_ERROR(logger, sout.str());
        DebugStop();
#else
        std::cout << sout.str() << std::endl;
#endif
        return -1;
    }
    const TPZConnect &c = this->Connect(connect);
    return c.Order();
}

int TPZHCurlNedFLinEl::EffectiveSideOrder(int side) const {
    if (side == TPZShapeLinear::NSides - 1) {
        return ConnectOrder(0);
    } else {
        return -1;
    }
}

void TPZHCurlNedFLinEl::SetSideOrder(int side, int order) {
    int connectaux = SideConnectLocId(0, side);
    if (connectaux < 0 || connectaux > this->NConnects()) {
        PZError << "TPZHCurlNedFLinEl::SetSideOrder. Bad parameter side "
                << side << " order " << order << std::endl;
#ifdef LOG4CXX
        std::stringstream sout;
        sout << __PRETTY_FUNCTION__ << " Bad side or order " << side
             << " order " << order;
        LOGPZ_DEBUG(logger, sout.str())
#endif
        return;
    }
    TPZConnect &c = this->Connect(connectaux);
    c.SetOrder(order, this->fConnectIndexes[connectaux]);
    long seqnum = c.SequenceNumber();
    int nvar = 1;
    TPZMaterial *mat = this->Material();
    if (mat)
        nvar = mat->NStateVariables();
    int nshape = NConnectShapeF(connectaux, order);
    c.SetNShape(nshape);
    c.SetNState(nvar);
    this->Mesh()->Block().Set(seqnum, nshape * nvar);
    this->SetIntegrationRule(2 * order);
}

TPZTransform<> TPZHCurlNedFLinEl::TransformSideToElement(int side) {
    DebugStop();
    return TPZTransform<>(2);//silence warning only
}

void TPZHCurlNedFLinEl::Shape(TPZVec<REAL> &qsi, TPZFMatrix<REAL> &phi,
                              TPZFMatrix<REAL> &curlPhiHat) {
    TPZVec<int> order(NConnects(),0);
    TPZVec<int> nShapeF(NConnects(),0);
    for(int iCon = 0; iCon < NConnects() ; iCon++){
        order[iCon] = ConnectOrder(iCon);
        nShapeF[iCon] = NConnectShapeF(iCon, ConnectOrder(iCon));
    }
    TPZHCurlNedFLinEl::CalcShape(qsi,phi,curlPhiHat,order,nShapeF);
}

void TPZHCurlNedFLinEl::ComputeShape(TPZVec<REAL> &intpoint, TPZVec<REAL> &X,
                                     TPZFMatrix<REAL> &jacobian,
                                     TPZFMatrix<REAL> &axes, REAL &detjac,
                                     TPZFMatrix<REAL> &jacinv,
                                     TPZFMatrix<REAL> &phi,
                                     TPZFMatrix<REAL> &curlPhiHat,
                                     TPZFMatrix<REAL> &curlPhi) {
    TPZGeoEl *ref = this->Reference();
#ifdef PZDEBUG
    if (!ref) {
        PZError << "\nERROR AT " << __PRETTY_FUNCTION__
                << " - this->Reference() == NULL\n";
        return;
    } // if
#endif
    TPZFMatrix<REAL> phiHat;

    ref->Jacobian(intpoint, jacobian, axes, detjac, jacinv);
    this->Shape(intpoint, phiHat, curlPhiHat);
    this->ShapeTransform(phiHat, jacinv, phi);
    this->CurlTransform(curlPhiHat, jacinv, curlPhi);
}

void TPZHCurlNedFLinEl::ShapeTransform(const TPZFMatrix<REAL> &phiHat,
                                       const TPZFMatrix<REAL> &jacinv,
                                       TPZFMatrix<REAL> &phi) {
    int nshape = phiHat.Rows();
    TPZGeoEl *gel = this->Reference();
//#ifdef PZDEBUG
//    if (gel->Neighbour(2).Side() > 6 || gel->Neighbour(2).Side() < 3) {
//        DebugStop();
//		    return;
//    }
//#endif
	phi.Redim(phiHat.Rows(), phiHat.Cols());
	
	for (int iPhi = 0; iPhi < nshape; iPhi++) {
		const int funcOrient = (iPhi % 2)*1 + ((iPhi+1) % 2)*fSideOrient;
		phi(iPhi, 0) = funcOrient * jacinv.GetVal(0, 0) * phiHat.GetVal(iPhi, 0);
	}
}

void TPZHCurlNedFLinEl::CurlTransform(const TPZFMatrix<REAL> &curlPhiHat,
                                      const TPZFMatrix<REAL> &jacinv,
                                      TPZFMatrix<REAL> &curlPhi) {
    return; // The curl wont be calculated in boundary elements
}

void TPZHCurlNedFLinEl::CreateGraphicalElement(TPZGraphMesh &grafgrid,
                                               int dimension) {
    if (this->Material()->Id() > 0) {
        DebugStop(); // graphical elements shouldnt be created for boundary
                     // elements. this is supposed to be a boundary el.
    }
}

TPZCompEl *CreateHCurlNedFLinEl(TPZGeoEl *gel, TPZCompMesh &mesh, long &index) {
    return new TPZHCurlNedFLinEl(mesh, gel, index);
}

#ifdef HCURL_HIERARCHICAL_SCALED
#include "TPZHCurlNedFLinElShapeScaled.cpp"
#else
#include "TPZHCurlNedFLinElShape.cpp"
#endif
