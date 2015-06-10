/*
Copyright (c) 2009-2011, UT-Battelle, LLC
All rights reserved

[PsimagLite, Version 1.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."

*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/
/** \ingroup PsimagLite */
/*@{*/

/*! \file LanczosSolver.h
 *
 *  A class to represent a generic Lanczos Solver
 *
 */

#ifndef LANCZOSSOLVER_HEADER_H
#define LANCZOSSOLVER_HEADER_H
#include "LanczosVectors.h"
#include "ProgressIndicator.h"
#include "TridiagonalMatrix.h"
#include <cassert>
#include "Vector.h"
#include "Matrix.h"
#include "Random48.h"
#include "ContinuedFraction.h"
#include "LanczosOrDavidsonBase.h"

namespace PsimagLite {

//! MatrixType must have the following interface:
//! 	RealType type to indicate the matrix type
//! 	rank() member function to indicate the rank of the matrix
//! 	matrixVectorProduct(typename Vector< RealType>::Type& x,const
//!     typename Vector< RealType>::Type& const y)
//!    	   member function that implements the operation x += Hy

template<typename SolverParametersType,typename MatrixType,typename VectorType>
class LanczosSolver : public LanczosOrDavidsonBase<SolverParametersType,MatrixType,VectorType> {

	typedef typename SolverParametersType::RealType RealType;
	typedef LanczosVectors<MatrixType,VectorType> LanczosVectorsType;
	typedef typename LanczosVectorsType::DenseMatrixType DenseMatrixType;
	typedef typename LanczosVectorsType::DenseMatrixRealType DenseMatrixRealType;

public:

	typedef SolverParametersType ParametersSolverType;
	typedef MatrixType LanczosMatrixType;
	typedef typename LanczosVectorsType::TridiagonalMatrixType TridiagonalMatrixType;
	typedef typename VectorType::value_type VectorElementType;
	typedef ContinuedFraction<TridiagonalMatrixType> PostProcType;

	enum {WITH_INFO=1,DEBUG=2,ALLOWS_ZERO=4};

	LanczosSolver(MatrixType const &mat,
	              const SolverParametersType& params,
	              Matrix<VectorElementType>* storageForLanczosVectors=0)
	    : progress_("LanczosSolver",params.threadId),
	      mat_(mat),
	      steps_(params.steps),
	      eps_(params.tolerance),
	      mode_(WITH_INFO),
	      stepsForEnergyConvergence_(params.stepsForEnergyConvergence),
	      rng_(343311),
	      lanczosVectors_(mat_,params.lotaMemory,storageForLanczosVectors)
	{
		setMode(params.options);
		OstringStream msg;
		msg<<"Constructing... mat.rank="<<mat_.rank();
		msg<<" maximum steps="<<steps_<<" maximum eps="<<eps_<<" requested";
		progress_.printline(msg,std::cout);
	}

	// FIXME : Deprecate this function
	virtual void computeGroundState(RealType& gsEnergy,VectorType& z)
	{
		SizeType n =mat_.rank();
		RealType atmp=0.0;
		VectorType y(n);

		for (SizeType i=0;i<n;i++) {
			y[i]=rng_()-0.5;
			atmp += std::real(y[i]*std::conj(y[i]));
		}
		if (mode_ & DEBUG) {
			computeExcitedStateTest(gsEnergy,z,y,0);
			return;
		}
		atmp = 1.0 / sqrt (atmp);
		for (SizeType i = 0; i < n; i++) y[i] *= atmp;
		computeGroundState(gsEnergy,z,y);
	}

	// FIXME : Deprecate this function
	virtual void computeGroundState(RealType &gsEnergy,
	                                VectorType &z,
	                                const VectorType& initialVector)
	{
		if (mode_ & DEBUG) {
			computeExcitedStateTest(gsEnergy,z,initialVector,0);
			return;
		}

		SizeType n=mat_.rank();
		VectorType y(n);

		RealType atmp=0.0;
		for (SizeType i=0;i<n;i++) {
			y[i]=initialVector[i];
			atmp += std::real(y[i]*std::conj(y[i]));
		}
		atmp = 1.0 / sqrt (atmp);
		for (SizeType i = 0; i < mat_.rank(); i++) y[i] *= atmp;

		TridiagonalMatrixType ab;

		decomposition(y,ab);
		typename Vector<RealType>::Type c(steps_);
		try {
			ground(gsEnergy,steps_, ab, c);
		} catch (std::exception &e) {
			std::cerr<<"MatrixRank="<<n<<"\n";
			throw e;
		}

		lanczosVectors_.hookForZ(z,c,ab);

		String str = "LanczosSolver: computeGroundState: ";
		if (norm(z)<1e-6)
			throw RuntimeError(str + " norm is zero\n");

		if (mode_ & WITH_INFO) info(gsEnergy,initialVector,0,std::cout);
	}

	virtual void computeExcitedState(RealType& gsEnergy,VectorType& z,SizeType excited)
	{
		if (excited == 0) return computeGroundState(gsEnergy,z);

		SizeType n =mat_.rank();
		RealType atmp=0.0;
		VectorType y(n);

		for (SizeType i=0;i<n;i++) {
			y[i]=rng_()-0.5;
			atmp += std::real(y[i]*std::conj(y[i]));
		}
		if (mode_ & DEBUG) {
			computeExcitedStateTest(gsEnergy,z,y,0);
			return;
		}
		atmp = 1.0 / sqrt (atmp);
		for (SizeType i = 0; i < n; i++) y[i] *= atmp;
		computeExcitedState(gsEnergy,z,y,excited);
	}

	virtual void computeExcitedState(RealType &gsEnergy,
	                                 VectorType &z,
	                                 const VectorType& initialVector,
	                                 SizeType excited)
	{
		if (excited == 0) return computeGroundState(gsEnergy,z,initialVector);

		if (mode_ & DEBUG) {
			computeExcitedStateTest(gsEnergy,z,initialVector,excited);
			return;
		}

		SizeType n=mat_.rank();
		VectorType y(n);

		RealType atmp=0.0;
		for (SizeType i=0;i<n;i++) {
			y[i]=initialVector[i];
			atmp += std::real(y[i]*std::conj(y[i]));
		}
		atmp = 1.0 / sqrt (atmp);
		for (SizeType i = 0; i < mat_.rank(); i++) y[i] *= atmp;

		TridiagonalMatrixType ab;

		decomposition(y,ab);
		gsEnergy = ab.excited(z,excited);

		if (mode_ & WITH_INFO) info(gsEnergy,initialVector,excited,std::cout);
	}

	void buildDenseMatrix(DenseMatrixType& T,const TridiagonalMatrixType& ab) const
	{
		ab.buildDenseMatrix(T);
	}

	const DenseMatrixRealType& reorthogonalizationMatrix()
	{
		return lanczosVectors_.reorthogonalizationMatrix();
	}

	void push(TridiagonalMatrixType& ab,const RealType& a,const RealType& b) const
	{
		ab.push(a,b);
	}

	/**
	  *     In each step of the Lanczos algorithm the values of a[]
	  *     and b[] are computed.
	  *     then a tridiagonal matrix T[j] is formed from the matrix
	  *     T[j-1] as
	  *
	  *            | a[0]  b[0]                            |
	  *            | b[0]  a[1]  b[1]                      |
	  *     T(j) = |       b[1]    .     .                 |
	  *            |               .     .  a[j-2]  b[j-2] |
	  *            |               .     .  b[j-2]  a[j-1] |
	  */
	void decomposition(const VectorType& initVector,
	                   TridiagonalMatrixType& ab)
	{
		SizeType& max_nstep = steps_;

		if (initVector.size()!=mat_.rank()) {
			String msg("decomposition: vector size ");
			msg += ttos(initVector.size()) + " but matrix size ";
			msg += ttos(mat_.rank()) + "\n";
			throw RuntimeError(msg);
		}

		VectorType x(mat_.rank());
		VectorType y = initVector;
		RealType atmp = 0;
		for (SizeType i = 0; i < mat_.rank(); i++) {
			x[i] = 0;
			atmp += std::real(y[i]*std::conj(y[i]));
		}

		for (SizeType i = 0; i < y.size(); i++) y[i] /= sqrt(atmp);

		if (max_nstep > mat_.rank()) max_nstep = mat_.rank();
		lanczosVectors_.resize(mat_.rank(),max_nstep);
		ab.resize(max_nstep,0);

		if (mode_ & ALLOWS_ZERO && lanczosVectors_.isHyZero(y,ab)) return;

		RealType eold = 100.;
		bool exitFlag=false;
		SizeType j = 0;
		RealType enew = 0;
		lanczosVectors_.saveInitialVector(y);
		typename Vector<RealType>::Type nullVector(0);
		for (; j < max_nstep; j++) {
			for (SizeType i = 0; i < mat_.rank(); i++)
				lanczosVectors_(i,j) = y[i];

			RealType btmp = 0;
			oneStepDecomposition(x,y,atmp,btmp,j==0);
			ab.a(j) = atmp;
			ab.b(j) = btmp;

			if (eps_>0) {
				ground(enew,j+1, ab,nullVector);
				if (fabs (enew - eold) < eps_) exitFlag=true;
				if (exitFlag && mat_.rank()<=4) break;
				if (exitFlag && j>=4) break;
			}

			eold = enew;
		}

		if (j < max_nstep) {
			max_nstep = j + 1;
			lanczosVectors_.reset(mat_.rank(),max_nstep);
			ab.resize(max_nstep);
		}

		OstringStream msg;
		msg<<"Decomposition done for mat.rank="<<mat_.rank();
		msg<<" after "<<j<<" steps";
		if (eps_>0) msg<<", actual eps="<<fabs(enew - eold);

		progress_.printline(msg,std::cout);

		if (j == max_nstep && j != mat_.rank()) {
			OstringStream msg2;
			msg2<<"WARNING: Maximum number of steps used. ";
			msg2<<"Increasing this maximum is recommended.";
			progress_.printline(msg2,std::cout);
		}
	}

	void oneStepDecomposition(
	        VectorType& x,
	        VectorType& y,
	        RealType& atmp,
	        RealType& btmp,
	        bool) const
	{
		lanczosVectors_.oneStepDecomposition(x,y,atmp,btmp);
	}

	SizeType steps() const {return steps_; }

private:

	void setMode(const String& options)
	{
		if (options.find("lanczosdebug")!=String::npos) mode_ |=  DEBUG;
		if (options.find("lanczosAllowsZero")!=String::npos) mode_ |= ALLOWS_ZERO;
	}

	void info(RealType energyTmp,
	          const VectorType& x,
	          SizeType excited,
	          std::ostream& os)
	{
		RealType norma=norm(x);
		SizeType& iter = steps_;

		if (norma<1e-5 || norma>100) {
			std::cerr<<"norma="<<norma<<"\n";
		}

		OstringStream msg;
		msg.precision(8);
		String what = "lowest";
		if (excited > 0) what = ttos(excited) + " excited";
		msg<<"Found "<<what<<" eigenvalue= "<<energyTmp<<" after "<<iter;
		msg<<" iterations, "<<" orig. norm="<<norma;
		if (excited > 0)
			msg<<"\nLanczosSolver: EXPERIMENTAL feature excited > 0 is in use";
		progress_.printline(msg,os);
	}

	void ground(RealType &s,
	            int n,
	            const TridiagonalMatrixType& ab,
	            typename Vector<RealType>::Type& gs)
	{
		int i, k, l, m;
		RealType c, dd, f, g, h, p, r, *d, *e, *v = 0, *vki;
		int long intCounter=0;
		int long maxCounter=stepsForEnergyConvergence_;

		if (gs.size()>0) {
			v  = new RealType[n*n];
			for (k=0;k<n*n;k++) v[k]=0.0;
			for (k = 0, vki = v; k < n; k++, vki += (n + 1))
				(*vki) = 1.0;
		}

		d = new RealType[n];
		e = new RealType[n];

		for (i = 0; i < n; i++) {
			d[i] = ab.a(i);
			e[i] = ab.b(i);
		}

		for (l = 0; l < n; l++) {
			do {
				intCounter++;
				if (intCounter>maxCounter) {
					std::cerr<<"lanczos: ground: premature exit ";
					std::cerr<<"(may indicate an internal error)\n";
					break;
				}
				for (m = l; m < n - 1; m++) {
					dd = fabs (d[m]) + fabs (d[m + 1]);
					if ((fabs (e[m]) + dd) == dd) break;
				}
				if (m != l) {
					g = (d[l + 1] - d[l]) / (2.0 * e[l]);
					r = sqrt (g * g + 1.0);
					g = d[m] - d[l] + e[l] / (g + (g >= 0 ? fabs (r) : -fabs (r)));
					for (i = m - 1, s = c = 1.0, p = 0.0; i >= l; i--) {
						f = s * e[i];
						h = c * e[i];
						e[i + 1] = (r = sqrt (f * f + g * g));
						if (r == 0.0) {
							d[i + 1] -= p;
							e[m] = 0.0;
							break;
						}
						s = f / r;
						c = g / r;
						g = d[i + 1] - p;
						r = (d[i] - g) * s + 2.0 * c * h;
						d[i + 1] = g + (p = s * r);
						g = c * r - h;
						if (gs.size()>0) {
							for (k = 0, vki = v + i; k < n; k++, vki += n) {
								f = vki[1];
								vki[1] = s * vki[0] + c * f;
								vki[0] = c * vki[0] - s * f;
							}
						}
					}
					if (r == 0.0 && i >= l) continue;
					d[l] -= p;
					e[l] = g;
					e[m] = 0.0;
				}
			} while (m != l);
		}

		for (i = 1, s = d[l = 0]; i < n; i++)
			if (d[i] < s) s = d[l = i];

		if (gs.size()>0) {
			for (k = 0, vki = v + l; k < n; k++, vki += n) gs[k] = (*vki);
			delete [] v;
		}

		delete [] d;
		delete [] e;
		if (intCounter>maxCounter)
			throw RuntimeError("LanczosSolver::ground(): internal error\n");

	}

	void getColumn(MatrixType const &mat,VectorType& x,SizeType col)
	{
		SizeType n =x.size();
		VectorType y(n);
		for (SizeType i=0;i<n;i++) {
			x[i]=0;
			y[i]=0;
			if (i==col) y[i]=1.0;
		}
		mat.matrixVectorProduct (x, y);
	}

	//! only for debugging:
	void computeExcitedStateTest(RealType&,
	                             VectorType&,
	                             const VectorType&,
	                             SizeType excited)
	{
		SizeType n =mat_.rank();
		Matrix<VectorElementType> a(n,n);
		for (SizeType i=0;i<n;i++) {
			VectorType x(n);
			getColumn(mat_,x,i);
			for (SizeType j=0;j<n;j++) a(i,j)=x[j];
		}
		bool ih  = isHermitian(a,true);
		if (!ih) throw RuntimeError("computeGroundState: Matrix not hermitian\n");

		std::cerr<<"Matrix hermitian="<<ih<<"\n";
		std::cerr<<a;
		std::cerr<<"----------------\n";

		typename Vector<RealType>::Type eigs(a.n_row());
		diag(a,eigs,'V');
		for (SizeType i=0;i<a.n_row();i++) std::cerr<<a(i,0)<<" ";
		std::cerr<<"\n";
		std::cerr<<"--------------------------------\n";
		std::cerr<<"eigs["<<excited<<"]="<<eigs[excited]<<"\n";

		throw RuntimeError("testing lanczos solver\n");
	}

	ProgressIndicator progress_;
	MatrixType const& mat_;
	SizeType steps_;
	RealType eps_;
	SizeType mode_;
	SizeType stepsForEnergyConvergence_;
	Random48<RealType> rng_;
	LanczosVectorsType lanczosVectors_;
}; // class LanczosSolver

} // namespace PsimagLite

/*@}*/
#endif

