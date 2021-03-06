/*
Copyright (c) 2009-2012, UT-Battelle, LLC
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

/*! \file DavidsonSolver.h
 *
 *  A class to represent a generic Davidson Solver
 *  reference: Ernest R. Davidson, J. Comp. Phys. 17, 87-94 (1975).
 *
 *  http://web.eecs.utk.edu/~dongarra/etemplates/node138.html
 */

#ifndef DAVIDSON_SOLVER_H
#define DAVIDSON_SOLVER_H
#include "LanczosVectors.h"
#include "ProgressIndicator.h"
#include "TridiagonalMatrix.h"
#include <cassert>
#include "Vector.h"
#include "Matrix.h"
#include "Random48.h"

namespace PsimagLite {

template<typename SolverParametersType,typename MatrixType,typename VectorType>
class DavidsonSolver : public LanczosOrDavidsonBase<SolverParametersType,MatrixType,VectorType> {

	typedef typename SolverParametersType::RealType RealType;
	typedef LanczosOrDavidsonBase<SolverParametersType,MatrixType,VectorType> ParentType;
	typedef typename VectorType::value_type ComplexOrRealType;

public:

	DavidsonSolver(MatrixType const &mat,
	               const SolverParametersType& params)
	    : progress_("DavidsonSolver",params.threadId),
	      mat_(mat),
	      steps_(params.steps),
	      eps_(params.tolerance),
	      mode_(ParentType::WITH_INFO),
	      rng_(343311)
	{
		setMode(params.options);
		OstringStream msg;
		msg<<"Constructing... mat.rank="<<mat_.rows();
		msg<<" steps="<<steps_<<" eps="<<eps_;
		progress_.printline(msg,std::cout);
	}

	virtual void computeGroundState(RealType& gsEnergy,VectorType& z)
	{
		SizeType n =mat_.rows();
		RealType atmp=0.0;
		VectorType y(n);

		for (SizeType i=0;i<n;i++) {
			y[i]=rng_()-0.5;
			atmp += PsimagLite::real(y[i]*PsimagLite::conj(y[i]));
		}
		if (mode_ & ParentType::DEBUG) {
			computeGroundStateTest(gsEnergy,z,y);
			return;
		}
		atmp = 1.0 / sqrt (atmp);
		for (SizeType i = 0; i < n; i++) y[i] *= atmp;
		computeGroundState(gsEnergy,z,y);
	}

	virtual void computeGroundState(RealType&,
	                                VectorType&,
	                                const VectorType&)
	{
		String s(__FILE__);
		s += " Unimplemented\n";
		throw RuntimeError(s.c_str());
	}

	virtual void computeExcitedState(RealType&,
	                                 VectorType&,
	                                 SizeType)
	{
		String s(__FILE__);
		s += " computeExcitedState Unimplemented\n";
		throw RuntimeError(s.c_str());
	}

	virtual void computeExcitedState(RealType&,
	                                 VectorType&,
	                                 const VectorType&,
	                                 SizeType)
	{
		String s(__FILE__);
		s += " computeExcitedState Unimplemented\n";
		throw RuntimeError(s.c_str());
	}

private:

	void setMode(const String& options)
	{
		if (options.find("lanczosdebug")!=String::npos)
			mode_ |=  ParentType::DEBUG;
		if (options.find("lanczosAllowsZero")!=String::npos)
			mode_ |= ParentType::ALLOWS_ZERO;
	}

	//! only for debugging:
	void computeGroundStateTest(RealType&,
	                            VectorType&,
	                            const VectorType&)
	{
		String s(__FILE__);
		s += " Unimplemented\n";
		throw RuntimeError(s.c_str());
	}

	void algorithm4_14(VectorType& t,const typename Vector<VectorType>::Type& v)
	{
		SizeType m = v.size();
		if (m==0) return;
		// select a value for k less than 1
		RealType k = 0.25;
		RealType tauin= PsimagLite::real(t*t);
		for (SizeType i=0;i<m;i++) {
			ComplexOrRealType tmp = scalarProduct(v[i],t);
			t=t-tmp*t;
		}
		if (PsimagLite::real(t*t)/tauin>k) return;
		for (SizeType i=0;i<m;i++) {
			ComplexOrRealType tmp = scalarProduct(v[i],t);
			t=t-tmp*v[i];
		}
	}

	void callMinRes()
	{
		String s(__FILE__);
		s += " Unimplemented\n";
		throw RuntimeError(s.c_str());
	}

	void largestEigenpair(RealType& theta,
	                      VectorType& s,
	                      const Matrix<ComplexOrRealType>& M)
	{
		String st(__FILE__);
		st += " Unimplemented\n";
		throw RuntimeError(st.c_str());
	}

	ProgressIndicator progress_;
	MatrixType const& mat_;
	SizeType steps_;
	RealType eps_;
	SizeType mode_;
	Random48<RealType> rng_;
}; // class DavidsonSolver
} // namespace PsimagLite

/*@}*/
#endif // DAVIDSON_SOLVER_H

