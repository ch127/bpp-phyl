//
// File: DecompositionSubstitutionCount.h
// Created by: Julien Dutheil
// Created on: Thu Mar 17 16:08 2011
//

/*
Copyright or © or Copr. Bio++ Development Team, (November 16, 2004, 2005, 2006)

This software is a computer program whose purpose is to provide classes
for phylogenetic data analysis.

This software is governed by the CeCILL  license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
*/

#include "DecompositionSubstitutionCount.h"

#include "Bpp/Numeric/Matrix/MatrixTools.h"
#include <vector>

using namespace bpp;
using namespace std;

/******************************************************************************/

DecompositionSubstitutionCount::DecompositionSubstitutionCount(const ReversibleSubstitutionModel* model, SubstitutionRegister* reg) :
  AbstractSubstitutionCount(reg), model_(model),
  nbStates_(model->getNumberOfStates()),
  jMat_(nbStates_, nbStates_),
  v_(nbStates_, nbStates_),
  vInv_(nbStates_, nbStates_),
  lambda_(nbStates_, nbStates_),
  bMatrices_(reg->getNumberOfSubstitutionTypes()),
  insideProducts_(reg->getNumberOfSubstitutionTypes()),
  counts_(reg->getNumberOfSubstitutionTypes()),
  currentLength_(-1.)
{
  //Check compatiblity between model and substitution register:
  if (model->getAlphabet()->getAlphabetType() != reg->getAlphabet()->getAlphabetType())
    throw Exception("DecompositionSubstitutionCount (constructor): alphabets do not match between register and model.");

  //Initialize all B matrices according to substitution register. This is done once for all,
  //unless the number of states changes:
  for (unsigned int i = 0; i < reg->getNumberOfSubstitutionTypes(); ++i) {
    bMatrices_[i].resize(nbStates_, nbStates_);
    insideProducts_[i].resize(nbStates_, nbStates_);
    counts_[i].resize(nbStates_, nbStates_);
  }
  for (unsigned int j = 0; j < nbStates_; ++j) {
    for (unsigned int k = 0; k < nbStates_; ++k) {
      unsigned int i = reg->getType(static_cast<int>(j), static_cast<int>(k));
      if (i > 0 && k != j) {
        bMatrices_[i - 1](j, k) = model->Qij(j, k);
      }
    }
  }
 
	v_      = model->getColumnRightEigenVectors();
  vInv_   = model->getRowLeftEigenVectors();
	lambda_ = model->getEigenValues();
	for (unsigned int i = 0; i < register_->getNumberOfSubstitutionTypes(); ++i) {
    //vInv_ %*% bMatrices_[i] %*% v_;
    RowMatrix<double> tmp(nbStates_, nbStates_);
    MatrixTools::mult(vInv_, bMatrices_[i], tmp);
    MatrixTools::mult(tmp, v_, insideProducts_[i]);
  }

}				
    
/******************************************************************************/

void DecompositionSubstitutionCount::jFunction_(const std::vector<double>& lambda, double t, RowMatrix<double>& result) const
{
	vector<double> expLam = VectorTools::exp(lambda * t);
	for (unsigned int i = 0; i < nbStates_; ++i) {
	  for (unsigned int j = 0; j < nbStates_; ++j) {
      double dd = lambda[i] - lambda[j];
      if (dd == 0) {
  		  result(i, j) = t * expLam[i];
      } else {
  		  result(i, j) = (expLam[i] - expLam[j]) / dd;
      }
    }
	}
}

/******************************************************************************/

void DecompositionSubstitutionCount::computeCounts_(double length) const
{
  jFunction_(lambda_, length, jMat_);
  for (unsigned int i = 0; i < register_->getNumberOfSubstitutionTypes(); ++i) {
    RowMatrix<double> tmp1(nbStates_, nbStates_), tmp2(nbStates_, nbStates_);
    MatrixTools::hadamardMult(jMat_, insideProducts_[i], tmp1);
    MatrixTools::mult(v_, tmp1, tmp2);
    MatrixTools::mult(tmp2, vInv_, counts_[i]);
	}
  // Now we must divide by pijt:
  RowMatrix<double> P = model_->getPij_t(length);
  for (unsigned int i = 0; i < register_->getNumberOfSubstitutionTypes(); i++) {
    for (unsigned int j = 0; j < nbStates_; j++) {
      for(unsigned int k = 0; k < nbStates_; k++) {
        counts_[i](j, k) /= P(j, k);
        if (isnan(counts_[i](j, k)) || counts_[i](j, k) < 0.) {
          counts_[i](j, k) = 0.;
        }
      }
    }
  }
}

/******************************************************************************/

Matrix<double>* DecompositionSubstitutionCount::getAllNumbersOfSubstitutions(double length, unsigned int type) const
{
  if (length < 0)
    throw Exception("DecompositionSubstitutionCount::getAllNumbersOfSubstitutions. Negative branch length: " + TextTools::toString(length) + ".");
  if (length != currentLength_)
  {
  //if (length < 0.000001) // Limit case!
  //{ 
  //  unsigned int s = model_->getAlphabet()->getSize();
  //  for (unsigned int i = 0; i < s; i++)
  //  {
  //    for (unsigned int j = 0; j < s; j++)
  //    {
  //      m_(i, j) = i == j ? 0. : 1.;
  //    }
  //  }
  //}
  //else
  //{
  // Else we need to recompute M:
    computeCounts_(length);
  //}

    currentLength_ = length;
  }
  return new RowMatrix<double>(counts_[type - 1]);
}

/******************************************************************************/

double DecompositionSubstitutionCount::getNumberOfSubstitutions(unsigned int initialState, unsigned int finalState, double length, unsigned int type) const
{
  if (length < 0)
    throw Exception("DecompositionSubstitutionCount::getNumbersOfSubstitutions. Negative branch length: " + TextTools::toString(length) + ".");
  if (length != currentLength_)
  {
    computeCounts_(length);
    currentLength_ = length;
  }
  return counts_[type - 1](initialState, finalState);
}

/******************************************************************************/

std::vector<double> DecompositionSubstitutionCount::getNumberOfSubstitutionsForEachType(unsigned int initialState, unsigned int finalState, double length) const
{
  if (length < 0)
    throw Exception("DecompositionSubstitutionCount::getNumbersOfSubstitutions. Negative branch length: " + TextTools::toString(length) + ".");
  if (length != currentLength_)
  {
    computeCounts_(length);
    currentLength_ = length;
  }
  std::vector<double> v(getNumberOfSubstitutionTypes());
  for (unsigned int t = 0; t < getNumberOfSubstitutionTypes(); ++t) {
    v[t] = counts_[t](initialState, finalState);
  }
  return v;
}
    
/******************************************************************************/

void DecompositionSubstitutionCount::setSubstitutionModel(const SubstitutionModel* model)
{
  const ReversibleSubstitutionModel* rModel = dynamic_cast<const ReversibleSubstitutionModel*>(model);
  if (!rModel)
    throw Exception("DecompositionSubstitutionCount::setSubstitutionModel. Only works with reversible models for now.");

  //Check compatiblity between model and substitution register:
  if (model->getAlphabet()->getAlphabetType() != register_->getAlphabet()->getAlphabetType())
    throw Exception("DecompositionSubstitutionCount::setSubstitutionModel: alphabets do not match between register and model.");
  model_ = rModel;
  unsigned int n = model->getAlphabet()->getSize();
  if (n != nbStates_) {
    nbStates_ = n;
    jMat_.resize(n, n);
    v_.resize(n, n);
    vInv_.resize(n, n);
    lambda_.resize(n);
    //Re-initialize all B matrices according to substitution register.
    for (unsigned int i = 0; i < register_->getNumberOfSubstitutionTypes(); ++i) {
      bMatrices_[i].resize(nbStates_, nbStates_);
      insideProducts_[i].resize(nbStates_, nbStates_);
      counts_[i].resize(nbStates_, nbStates_);
    }
  }
  for (unsigned int j = 0; j < nbStates_; ++j) {
    for (unsigned int k = 0; k < nbStates_; ++k) {
      unsigned int i = register_->getType(static_cast<int>(j), static_cast<int>(k));
      if (i > 0) {
        bMatrices_[i - 1](j, k) = model->Qij(j, k);
      }
    }
  }

	v_      = model->getColumnRightEigenVectors();
	vInv_   = model->getRowLeftEigenVectors();
	lambda_ = model->getEigenValues();
	for (unsigned int i = 0; i < register_->getNumberOfSubstitutionTypes(); ++i) {
    //vInv_ %*% bMatrices_[i] %*% v_;
    RowMatrix<double> tmp(nbStates_, nbStates_);
    MatrixTools::mult(vInv_, bMatrices_[i], tmp);
    MatrixTools::mult(tmp, v_, insideProducts_[i]);
  }

  //Recompute counts:
  computeCounts_(currentLength_);
}

/******************************************************************************/
