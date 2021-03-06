//
// Created by Francisco Teixeira Orlandini on 11/27/17.
//

#ifndef PZ_TPZLAPACKWRAPPER_H
#define PZ_TPZLAPACKWRAPPER_H


#ifdef USING_LAPACK

#ifdef USING_MKL //mkl version is faster
#include <mkl.h>
typedef MKL_Complex16 vardoublecomplex;
typedef MKL_Complex8 varfloatcomplex;
#elif MACOSX //accelerate
#include <Accelerate/Accelerate.h>
typedef __CLPK_doublecomplex vardoublecomplex;
typedef __CLPK_complex varfloatcomplex;
#else //openblas, standalone lapack, etc
#define lapack_complex_double std::complex<double>
#define lapack_complex_float std::complex<float>
#include "lapacke.h"
typedef lapack_complex_double vardoublecomplex;
typedef lapack_complex_float varfloatcomplex;
#endif//using_mkl

#include <TPZEigenSolver.h>
#include <pzfmatrix.h>
#include <pzsbndmat.h>

template<class TVar>
class SPZAlwaysComplex;

template<class TVar>
class TPZLapackWrapper : public TPZEigenSolver<TVar> {
    friend class TPZFMatrix<TVar>;
    friend class TPZSBMatrix<TVar>;

  TPZSolver<TVar> *Clone() const override{
    //@TODO: Implement me!
    return (TPZSolver<TVar> *)this;
  }
  /**
   * @brief Solves the Ax=w*x eigenvalue problem and calculates the eigenvectors
   * @param A The matrix (input)
   * @param w Stores the eigenvalues
   * @param eigenVectors Stores the correspondent eigenvectors
   * @return it returns 1 if executed correctly
   */
  int SolveEigenProblem(TPZMatrix <TVar> &A, TPZVec<typename SPZAlwaysComplex<TVar>::type> &w,
                                TPZFMatrix<typename SPZAlwaysComplex<TVar>::type> &eigenVectors) override;

  /**
   * @brief Solves the Ax=w*x eigenvalue problem and does not calculate the eigenvectors
   * @param A The matrix (input)
   * @param w Stores the eigenvalues
   * @return it returns 1 if executed correctly
   */
  int SolveEigenProblem(TPZMatrix <TVar> &A, TPZVec<typename SPZAlwaysComplex<TVar>::type> &w) override;

  /**
   * @brief Solves the generalised Ax=w*B*x eigenvalue problem and calculates the eigenvectors
   * @param A The lhs matrix (input)
   * @param B The rhs matrix (input)
   * @param w Stores the eigenvalues
   * @param eigenVectors Stores the correspondent eigenvectors
   * @return it returns 1 if executed correctly
   */
  int SolveGeneralisedEigenProblem(TPZMatrix <TVar> &A, TPZMatrix <TVar> &B,
                                           TPZVec<typename SPZAlwaysComplex<TVar>::type> &w,
                                           TPZFMatrix<typename SPZAlwaysComplex<TVar>::type> &eigenVectors) override;

  /**
   * @brief Solves the generalised Ax=w*B*x eigenvalue problem and does NOT calculates the eigenvectors
   * @param A The lhs matrix (input)
   * @param B The rhs matrix (input)
   * @param w Stores the eigenvalues
   * @return it returns 1 if executed correctly
   */
  int SolveGeneralisedEigenProblem(TPZMatrix <TVar> &A, TPZMatrix <TVar> &B,
                                           TPZVec<typename SPZAlwaysComplex<TVar>::type> &w) override;


    /*******************
    *    TPZFMATRIX    *
    *******************/
    int SolveEigenProblem(TPZFMatrix<TVar> &A, TPZVec < typename SPZAlwaysComplex<TVar>::type > &w, TPZFMatrix < typename SPZAlwaysComplex<TVar>::type  > &eigenVectors);

    /** @brief Solves the Ax=w*x eigenvalue problem and does NOT calculates the eigenvectors
     * @param w Stores the eigenvalues
     */
    int SolveEigenProblem(TPZFMatrix<TVar> &A, TPZVec < typename SPZAlwaysComplex<TVar>::type > &w);

    /** @brief Solves the generalised Ax=w*B*x eigenvalue problem and calculates the eigenvectors
     * @param w Stores the eigenvalues
     * @param Stores the correspondent eigenvectors
     */
    int SolveGeneralisedEigenProblem(TPZFMatrix<TVar> &A, TPZFMatrix< TVar > &B , TPZVec < typename SPZAlwaysComplex<TVar>::type > &w, TPZFMatrix < typename SPZAlwaysComplex<TVar>::type > &eigenVectors);
    /** @brief Solves the generalised Ax=w*B*x eigenvalue problem and does NOT calculates the eigenvectors
     * @param w Stores the eigenvalues
     */
    int SolveGeneralisedEigenProblem(TPZFMatrix<TVar> &A, TPZFMatrix< TVar > &B , TPZVec < typename SPZAlwaysComplex<TVar>::type > &w);

    /*******************
    *    TPSBMATRIX    *
    *******************/
    int SolveEigenProblem(TPZSBMatrix<TVar> &A, TPZVec < typename SPZAlwaysComplex<TVar>::type > &w, TPZFMatrix < typename SPZAlwaysComplex<TVar>::type  > &eigenVectors);

    /** @brief Solves the Ax=w*x eigenvalue problem and does NOT calculates the eigenvectors
     * @param w Stores the eigenvalues
     */
    int SolveEigenProblem(TPZSBMatrix<TVar> &A, TPZVec < typename SPZAlwaysComplex<TVar>::type > &w);

    /** @brief Solves the generalised Ax=w*B*x eigenvalue problem and calculates the eigenvectors
     * @param w Stores the eigenvalues
     * @param Stores the correspondent eigenvectors
     */
    int SolveGeneralisedEigenProblem(TPZSBMatrix<TVar> &A, TPZSBMatrix< TVar > &B , TPZVec < typename SPZAlwaysComplex<TVar>::type > &w, TPZFMatrix < typename SPZAlwaysComplex<TVar>::type > &eigenVectors);
    /** @brief Solves the generalised Ax=w*B*x eigenvalue problem and does NOT calculates the eigenvectors
     * @param w Stores the eigenvalues
     */
    int SolveGeneralisedEigenProblem(TPZSBMatrix<TVar> &A, TPZSBMatrix< TVar > &B , TPZVec < typename SPZAlwaysComplex<TVar>::type > &w);
};

#endif//using_lapack
#endif //PZ_TPZLAPACKWRAPPER_H
