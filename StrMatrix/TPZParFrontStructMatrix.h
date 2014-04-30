/**
 * @file
 * @brief Contains the TPZParFrontStructMatrix class which is a structural matrix with parallel techniques included.
 */

#ifndef TPZPARFRONTSTRUCTMATRIX_H
#define TPZPARFRONTSTRUCTMATRIX_H

#include "TPZFrontStructMatrix.h"
#include "pzstrmatrix.h"
#include "pzcmesh.h" 

#include "TPZFrontMatrix.h"
#include "TPZFrontNonSym.h"
#include "TPZFrontSym.h"

#include "pzelmat.h"

#include <signal.h>
#include <time.h>

//#ifndef PZPAR
#include <pthread.h>
//#endif

struct TPZElementMatrix;

template<class TVar>
class TPZMatrix;
template<class TVar>
class TPZFMatrix;
class TPZCompMesh;
template<class TVar>
class TPZFileEqnStorage;

#define STACKSTORAGE

/**
 * @brief Is a structural matrix with parallel techniques included. \ref structural "Structural Matrix" \ref frontal "Frontal"
 * @ingroup frontal structural
 */
/** 
 * TPZParFrontStructMatrix is derived from TPZFrontStructMatrix.
 * It uses TPZParFrontMatrix as its FrontalMatrix
 */
template<class front>
class TPZParFrontStructMatrix : public TPZFrontStructMatrix<front> {
	
private:
	
	TPZAutoPointer<TPZGuiInterface> fGuiInterface;
	
public:     
		
	/** @brief It clones a TPZStructMatrix */
	/** Virtual function must return same type */
	TPZStructMatrix *Clone();
	
	/** @brief Constructor passing as parameter a TPZCompMesh */
	TPZParFrontStructMatrix(
							TPZCompMesh *mesh //! Mesh to refer to
							);
	
	TPZParFrontStructMatrix(const TPZParFrontStructMatrix &copy);
	
	/**
	* @brief Destructor
	*/
	~TPZParFrontStructMatrix();
	
	/**
	 * @brief Returns a pointer to TPZMatrix
	 * @param rhs Load matrix
	 * @param guiInterface pointer to user interface
	 */
	virtual TPZMatrix<STATE> * CreateAssemble( TPZFMatrix<STATE> &rhs,TPZAutoPointer<TPZGuiInterface> guiInterface);
	
	
	virtual void Assemble(TPZMatrix<STATE> & mat, TPZFMatrix<STATE> & rhs,TPZAutoPointer<TPZGuiInterface> guiInterface);
	
	/** Used only for testing */
	static int main();
	
	/** @brief It computes element matrices in an independent thread. */
	/** 
	 * It is passed as a parameter to the  pthread_create() function. \n
	 * It is a 'static void *' to be used by pthread_create
	 */
	static void *ElementAssemble(void *t);
	/** @brief It assembles element matrices in the global stiffness matrix, it is also executed in an independent thread. */
	/**
	 * It is passed as a parameter to the  pthread_create() function. \n
	 * It is a 'static void *' to be used by pthread_create
	 */
	static void *GlobalAssemble(void *t);
	
private:
	/** @brief Current computed element*/
	long fCurrentElement;
	/** @brief Current assembled element in the global stiffness matrix*/
	long fCurrentAssembled;
	/** @brief Total number of elements*/
	long fNElements;
	/** @brief Maximum stack size allowed. */
	/** Whenever this value is reached a execution of element computing is suspended */
	int fMaxStackSize;
	/** @brief Local pointer to stiffness matrix*/
	TPZMatrix<STATE> * fStiffness;
	/** @brief Local pointer to load matrix*/
	TPZFMatrix<STATE> * fRhs;
	
	/** @brief Stack containing elements to be assembled on Stiffness matrix. */
	/** 
	 * ElementAssemble pushes elements on the stack. \n
	 * GlobalAssemble pops elements from the stack.
	 */
	TPZStack <long> felnum;
	TPZStack <TPZElementMatrix *> fekstack;
	TPZStack <TPZElementMatrix *> fefstack;
	
};

#endif //TPZPARFRONTSTRUCTMATRIX_H
