 //
// File: WordReversibleSubstitutionModel.h
// Created by: Laurent Gueguen
// Created on: Jan 2009
//

/*
Copyright or � or Copr. CNRS, (November 16, 2004)

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

#ifndef _WORDREVERSIBLESUBSTITUTIONMODEL_H_
#define _WORDREVERSIBLESUBSTITUTIONMODEL_H_

#include "AbstractWordReversibleSubstitutionModel.h"

// From Utils
#include <Utils/BppVector.h>

namespace bpp
{

/**
 * @brief Basal class for words of reversible substitution models.
 *
 * Only substitutions with one letter changed are accepted. Hence the
 * equilibrium frequency of each word is the product of the
 * equilibrium frequencies of the letters.</p>
 *
 * The rates are defined by relative rates parameters $r_i$ with:
 * @f[
 * i < n-1, \rho_i = (1-r_0).(1-r_1)...(1-r_{i-1}).r_i
 * \rho_{n-1} = (1-r_0).(1-r_1)...(1-r_{n-2})
 * @f]
 * and
 * @f[
 * \forall i< n-1, r_i = \frac{\rho_i}{1-(\rho_0+...\rho_{i-1})}
 * @f]
 * where $\rho_i$ stands for the rate of position $i$.
 */
  
class WordReversibleSubstitutionModel :
  public AbstractWordReversibleSubstitutionModel
{
public:

  /**
   *@brief Build a new WordReversibleSubstitutionModel object from a
   *Vector of pointers to SubstitutionModels.
   *
   *@param modelVector the Vector of substitution models to use, in
   *   the order of the positions in the words from left to right. All
   *   the models must be different objects to avoid parameters
   *   redondancy, otherwise only the first model is used.
   */
  
  WordReversibleSubstitutionModel(const Vector<SubstitutionModel*>& modelVector, const std::string& = "");

  /**
   *@brief Build a new WordReversibleSubstitutionModel object from a
   *pointer to an SubstitutionModel and a number of
   *desired models.
   *
   * @param A pointer to the substitution model to use in all the
   *positions.
   * @param The number of models involved.
   */

  WordReversibleSubstitutionModel(SubstitutionModel*, unsigned int, const std::string& = "");

  WordReversibleSubstitutionModel(const WordReversibleSubstitutionModel&);

  virtual ~WordReversibleSubstitutionModel(){};
  
#ifndef NO_VIRTUAL_COV
  WordReversibleSubstitutionModel*
#else
  Clonable*
#endif
  clone() const { return new WordReversibleSubstitutionModel(*this);}

protected:

  /**
   *@brief Constructor for the derived classes only
   */

  WordReversibleSubstitutionModel(const Alphabet* alph, const std::string& = "");

  virtual void updateMatrices();
  virtual void completeMatrices();
  
public:
  
  virtual double Pij_t(int i, int j, double d) const;

  virtual const RowMatrix<double>& getPij_t(double d) const;

  virtual double dPij_dt(int i, int j, double d) const;

  virtual const RowMatrix<double>& getdPij_dt(double d) const;

  virtual double d2Pij_dt2(int i, int j, double d) const;

  virtual const RowMatrix<double>& getd2Pij_dt2(double d) const;

  virtual string getName() const;
};

} //end of namespace bpp.

#endif	//_WORDREVERSIBLESUBSTITUTIONMODEL
