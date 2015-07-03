//
//  TPZCompMeshTools.cpp
//  PZ
//
//  Created by Philippe Devloo on 6/4/15.
//
//

#include "TPZCompMeshTools.h"
#include "pzelchdiv.h"
#include "pzshapepiram.h"
#include "TPZOneShapeRestraint.h"
#include "pzshapetriang.h"
#include "pzshapetetra.h"

static TPZOneShapeRestraint SetupPyramidRestraint(TPZCompEl *cel, int side);

using namespace pzshape;

void TPZCompMeshTools::AddHDivPyramidRestraints(TPZCompMesh *cmesh)
{
    long nel = cmesh->NElements();
    for (long el=0; el<nel; el++) {
        TPZCompEl *cel = cmesh->Element(el);
        TPZCompElHDiv< pzshape::TPZShapePiram > *pyram = dynamic_cast<TPZCompElHDiv< pzshape::TPZShapePiram > *>(cel);
        if (!pyram) {
            continue;
        }
        
        if (cel->Type() != EPiramide)
        {
            DebugStop();
        }
        TPZGeoEl *gel = cel->Reference();
        /// we need to identify the connect to restraint
        /// start with face 14
        int is;
        for (is=14; is<18; is++) {
            TPZGeoElSide gelside(gel,is);
            TPZCompElSide large = gelside.LowerLevelCompElementList2(1);
            // if the side is constrained, it wont be used as a reference
            if (large) {
                continue;
            }
            // if the side has smaller elements connected to it, discard the side
            TPZStack<TPZCompElSide> celstack;
            gelside.HigherLevelCompElementList2(celstack, 1, 0);
            if (celstack.size()) {
                continue;
            }
            // see if there is a pyramid element  neighbour of the pyramid
            gelside.EqualLevelCompElementList(celstack, 1, 0);
            int localel;
            for (localel=0; localel<celstack.size(); localel++) {
                if (celstack[localel].Reference().Element()->Type() == ETetraedro) break;
            }
            TPZCompElHDiv<pzshape::TPZShapeTetra> *pir;
            /// the pyramid has a neighbour of pyramid type
            if (localel < celstack.size()) {
                // find out if the face is the restrained face
                pir = dynamic_cast<TPZCompElHDiv<pzshape::TPZShapeTetra> *>(celstack[localel].Element());
                if (pir == NULL) {
                    DebugStop();
                }
                const int restrainedface = pir->RestrainedFace();
                const int neighbourside = celstack[localel].Side();
                if (restrainedface == neighbourside) {
                    continue;
                }
                break;
            }
        }
        if (is == 18 || is == 13) {
            cmesh->Print();
            DebugStop();
        }
        TPZOneShapeRestraint rest = SetupPyramidRestraint(pyram, is);
        pyram->AddShapeRestraint(rest);
        /// add the restraint datastructure to the neighbour
        TPZGeoElSide gelside(cel->Reference(),is);
        TPZGeoElSide neighbour = gelside.Neighbour();
        TPZCompElSide celneigh = neighbour.Reference();
        TPZInterpolatedElement *interp = dynamic_cast<TPZInterpolatedElement *>(celneigh.Element());
        interp->AddShapeRestraint(rest);
    }
    
        
    ExpandHDivPyramidRestraints(cmesh);
}

static int idf[6] = {2,1,1,2,0,0};

static int nextface(int side, int inc)
{
    return ((side-14)+inc)%4+14;
}

static void GetGeoNodeIds(TPZGeoEl *gel, int side, TPZVec<long> &ids)
{
    int nsidenodes = gel->NSideNodes(side);
    if (nsidenodes != 3) {
        DebugStop();
    }
    for (int i=0; i<3; i++) {
        ids[i] = gel->SideNodePtr(side, i)->Id();
    }
}

static int SideIdf(TPZCompElHDiv<TPZShapePiram> *cel, int side)
{
    TPZManVector<long,3> ids(3);
    GetGeoNodeIds(cel->Reference(),side,ids);
    int transformid = TPZShapeTriang::GetTransformId2dT(ids);
    return idf[transformid];
}
static TPZOneShapeRestraint SetupPyramidRestraint(TPZCompEl *cel, int side)
{
    TPZCompElHDiv<TPZShapePiram> *pir = dynamic_cast<TPZCompElHDiv<TPZShapePiram> *>(cel);
    if (!pir) {
        DebugStop();
    }
    TPZOneShapeRestraint result;
    int mult = 1;
    for (int fc=0; fc<4; fc++)
    {
        int face = nextface(side, fc);
        long nodeindex = pir->SideConnectIndex(0, face);
        result.fFaces[fc] = std::make_pair(nodeindex, SideIdf(pir, face));
        result.fOrient[fc] = mult*pir->SideOrient(face-13);
        mult *= -1;
    }
    return result;
}

void TPZCompMeshTools::ExpandHDivPyramidRestraints(TPZCompMesh *cmesh)
{
    /// Add the shapeonerestraints of all restrained connects build a map indexed on the restrained connect
    std::map<long, TPZOneShapeRestraint> AllRestraints;
    typedef std::list<TPZOneShapeRestraint>::iterator itlist;
    if (!cmesh) {
        DebugStop();
    }
    long nelem = cmesh->NElements();
    for (long iel=0; iel<nelem; iel++) {
        TPZCompEl *cel = cmesh->Element(iel);
        std::list<TPZOneShapeRestraint> ellist;
        ellist = cel->GetShapeRestraints();
        for (itlist it = ellist.begin(); it != ellist.end(); it++) {
            long elindex = it->fFaces[0].first;
            AllRestraints[elindex] = *it;
        }
    }
    /// For each element that has a oneshape restraint, add the oneshape restraints of all connects
    for (long iel=0; iel<nelem; iel++) {
        TPZCompEl *cel = cmesh->Element(iel);
        std::list<TPZOneShapeRestraint> ellist;
        std::map<long, TPZOneShapeRestraint> restraintmap;
        std::set<long> connectset;
        ellist = cel->GetShapeRestraints();
        // we will insert the restraints afterwards
        cel->ResetShapeRestraints();
        for (itlist it = ellist.begin(); it != ellist.end(); it++) {
            std::cout << "1 including connect index " << it->fFaces[0].first << " to restraint map\n";
            it->Print(std::cout);
            restraintmap[it->fFaces[0].first] = *it;
            for (int i=1; i<4; i++) {
                connectset.insert(it->fFaces[i].first);
            }
        }
        while (connectset.size()) {
            long cindex = *connectset.begin();
            connectset.erase(cindex);
            if (AllRestraints.find(cindex) != AllRestraints.end()) {
                TPZOneShapeRestraint restloc = AllRestraints[cindex];
                std::cout << "2 including connect index " << cindex << " to restraint map\n";
                restloc.Print(std::cout);
                restraintmap[cindex] = restloc;
                for (int i=1; i<4; i++) {
                    long locindex = restloc.fFaces[i].first;
                    std::cout << "3 verifying connect index " << locindex << "\n";
                    if (restraintmap.find(locindex) != restraintmap.end()) {
                        std::cout << "********************* CIRCULAR RESTRAINT\n";
                    }
                    else
                    {
                        connectset.insert(locindex);
                    }
                }
            }
        }
        for (std::map<long,TPZOneShapeRestraint>::iterator it = restraintmap.begin(); it != restraintmap.end(); it++) {
            cel->AddShapeRestraint(it->second);
        }
    }
    
}

void TPZCompMeshTools::LoadSolution(TPZCompMesh *cpressure, TPZFunction<STATE> &Forcing)
{
    long nel = cpressure->NElements();
    for (long iel=0; iel<nel; iel++) {
        TPZCompEl *cel = cpressure->Element(iel);
        if (!cel) {
            continue;
        }
        TPZGeoEl *gel = cel->Reference();
        if (gel->Dimension() != 3) {
            continue;
        }
        if (gel->Type() == EPiramide) {
            TPZGeoNode *top = gel->NodePtr(4);
            TPZManVector<REAL,3> topco(3),valvec(1);
            top->GetCoordinates(topco);
            Forcing.Execute(topco, valvec);
            STATE topval = valvec[0];
            TPZConnect &c = cel->Connect(0);
            long seqnum = c.SequenceNumber();
            for (int i=0; i<4; i++) {
                cpressure->Block()(seqnum,0,i,0) = topval;
            }
            for (int i=0; i<4; i++) {
                TPZConnect &c = cel->Connect(i+1);
                TPZGeoNode *no = gel->NodePtr(i);
                no->GetCoordinates(topco);
                Forcing.Execute(topco, valvec);
                STATE nodeval = valvec[0];
                seqnum = c.SequenceNumber();
                cpressure->Block()(seqnum,0,0,0) = nodeval-topval;
            }
        }
        else
        {
            int ncorner = gel->NCornerNodes();
            for (int i=0; i<ncorner; i++) {
                TPZConnect &c = cel->Connect(i);
                TPZGeoNode *no = gel->NodePtr(i);
                TPZManVector<REAL,3> topco(3);
                no->GetCoordinates(topco);
                TPZManVector<STATE,3> valvec(1);
                Forcing.Execute(topco, valvec);
                STATE nodeval = valvec[0];
                long seqnum = c.SequenceNumber();
                cpressure->Block()(seqnum,0,0,0) = nodeval;
            }
        }
    }
}
