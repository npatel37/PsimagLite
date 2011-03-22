// BEGIN LICENSE BLOCK
/*
Copyright (c) 2009 , UT-Battelle, LLC
All rights reserved

[PsimagLite, Version 1.0.0]

*********************************************************
THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. 

Please see full open source license included in file LICENSE.
*********************************************************

*/
// END LICENSE BLOCK

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "IoSimple.h"
#include "TridiagonalMatrix.h"
#include "ContinuedFraction.h"
#include "TwoContinuedFraction.h"

using namespace PsimagLite;
typedef double RealType;
typedef TridiagonalMatrix<RealType> TridiagonalMatrixType;
typedef ContinuedFraction<RealType,TridiagonalMatrixType> ContinuedFractionType;
typedef TwoContinuedFraction<ContinuedFractionType> TwoContinuedFractionType;

void usage(const char *progName)
{
	std::cerr<<"Usage: "<<progName<<" file1 file2";
}

int main(int argc,char *argv[])
{
	if (argc<2) {
		usage(argv[0]);
		return 1;
	}

	IoSimple::In io1(argv[1]);
	ContinuedFractionType cfPlus(io1);
	ContinuedFractionType *cfMinus;
	
	if (argc==3) {
		IoSimple::In io2(argv[2]);
		cfMinus = new ContinuedFractionType(io2);
	} else {
		TridiagonalMatrixType ab;
		cfMinus = new ContinuedFractionType(ab,0,0);
	}

	TwoContinuedFractionType twoCf(cfPlus,*cfMinus);
	IoSimple::Out ioOut(std::cout);
	twoCf.save(ioOut);

}
