//
// File: RNonHomogeneousTreeLikelihood.cpp
// Created by: Julien Dutheil
// Created on: Tue Oct 09 16:07 2007
// From file: RHomogeneousTreeLikelihood.cpp
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

#include "RNonHomogeneousTreeLikelihood.h"
#include "PatternTools.h"

// From Utils:
#include <Utils/TextTools.h>
#include <Utils/ApplicationTools.h>

// From the STL:
#include <iostream>
using namespace std;

/******************************************************************************/

RNonHomogeneousTreeLikelihood::RNonHomogeneousTreeLikelihood(
  const Tree & tree,
  SubstitutionModelSet * modelSet,
  DiscreteDistribution * rDist,
  bool checkRooted,
  bool verbose,
  bool usePatterns)
throw (Exception):
  AbstractNonHomogeneousTreeLikelihood(tree, modelSet, rDist, checkRooted, verbose),
  _likelihoodData(NULL)
{
  if(!modelSet->isFullySetUpFor(tree))
    throw Exception("RNonHomogeneousTreeLikelihood(constructor). Model set is not fully specified.");
   _init(usePatterns);
}

/******************************************************************************/

RNonHomogeneousTreeLikelihood::RNonHomogeneousTreeLikelihood(
  const Tree & tree,
  const SiteContainer & data,
  SubstitutionModelSet * modelSet,
  DiscreteDistribution * rDist,
  bool checkUnrooted,
  bool verbose,
  bool usePatterns)
throw (Exception):
  AbstractNonHomogeneousTreeLikelihood(tree, modelSet, rDist, checkUnrooted, verbose),
  _likelihoodData(NULL)
{
  if(!modelSet->isFullySetUpFor(tree))
    throw Exception("RNonHomogeneousTreeLikelihood(constructor). Model set is not fully specified.");
  _init(usePatterns);
  setData(data);
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::_init(bool usePatterns) throw (Exception)
{
  _likelihoodData = new DRASRTreeLikelihoodData(*_tree, _rateDistribution->getNumberOfCategories(), usePatterns);
}

/******************************************************************************/

RNonHomogeneousTreeLikelihood::RNonHomogeneousTreeLikelihood(
    const RNonHomogeneousTreeLikelihood & lik):
  AbstractNonHomogeneousTreeLikelihood(lik),
  _likelihoodData(NULL)
{
  _likelihoodData = lik._likelihoodData->clone();
  _likelihoodData->setTree(*_tree);
}

/******************************************************************************/

RNonHomogeneousTreeLikelihood & RNonHomogeneousTreeLikelihood::operator=(
    const RNonHomogeneousTreeLikelihood & lik)
{
  AbstractNonHomogeneousTreeLikelihood::operator=(lik);
  _likelihoodData = lik._likelihoodData->clone();
  _likelihoodData->setTree(*_tree);
  return *this;
}

/******************************************************************************/

RNonHomogeneousTreeLikelihood::~RNonHomogeneousTreeLikelihood()
{ 
  delete _likelihoodData;
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::setData(const SiteContainer & sites) throw (Exception)
{
  if(_data) delete _data;
  _data = PatternTools::getSequenceSubset(sites, * _tree->getRootNode());
  if(_verbose) ApplicationTools::displayTask("Initializing data structure");
  _likelihoodData->initLikelihoods(* _data, *_modelSet->getModel(0)); //We assume here that all models have the same number of states, and that they have the same 'init' method,
                                                                      //Which is a reasonable assumption as long as they share the same alphabet.
  if(_verbose) ApplicationTools::displayTaskDone();

  _nbSites = _likelihoodData->getNumberOfSites();
  _nbDistinctSites = _likelihoodData->getNumberOfDistinctSites();
  _nbStates = _likelihoodData->getNumberOfStates();
  
  if(_verbose) ApplicationTools::displayResult("Number of distinct sites",
      TextTools::toString(_nbDistinctSites));
  _initialized = false;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLikelihood() const
{
  double l = 1.;
  for(unsigned int i = 0; i < _nbSites; i++)
  {
    l *= getLikelihoodForASite(i);
  }
  return l;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLogLikelihood() const
{
  double ll = 0;
  for(unsigned int i = 0; i < _nbSites; i++)
  {
    ll += getLogLikelihoodForASite(i);
  }
  return ll;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLikelihoodForASite(unsigned int site) const
{
  double l = 0;
  for(unsigned int i = 0; i < _nbClasses; i++)
  {
    l += getLikelihoodForASiteForARateClass(site, i) * _rateDistribution->getProbability(i);
  }
  return l;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLogLikelihoodForASite(unsigned int site) const
{
  double l = 0;
  for(unsigned int i = 0; i < _nbClasses; i++)
  {
    l += getLikelihoodForASiteForARateClass(site, i) * _rateDistribution->getProbability(i);
  }
  //if(l <= 0.) cerr << "WARNING!!! Negative likelihood." << endl;
  if(l<0) l=0; //May happen because of numerical errors.
  return log(l);
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
  double l = 0;
  Vdouble * la = & _likelihoodData->getLikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass];
  for(unsigned int i = 0; i < _nbStates; i++)
  {
    l += (* la)[i] * _rootFreqs[i];
  }
  return l;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLogLikelihoodForASiteForARateClass(unsigned int site, unsigned int rateClass) const
{
  double l = 0;
  Vdouble * la = & _likelihoodData->getLikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass];
  for(unsigned int i = 0; i < _nbStates; i++) {
    l += (* la)[i] * _rootFreqs[i];
  }
  //if(l <= 0.) cerr << "WARNING!!! Negative likelihood." << endl;
  return log(l);
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getLikelihoodForASiteForARateClassForAState(unsigned int site, unsigned int rateClass, int state) const
{
  return _likelihoodData->getLikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass][state];
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getLogLikelihoodForASiteForARateClassForAState(unsigned int site, unsigned int rateClass, int state) const
{
  return log(_likelihoodData->getLikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass][state]);
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::setParameters(const ParameterList & parameters)
  throw (ParameterNotFoundException, ConstraintException)
{
  setParametersValues(parameters);
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::fireParameterChanged(const ParameterList & params)
{
  applyParameters();

  if(params.getCommonParametersWith(_rateDistribution->getParameters()).size() > 0)
  {
    computeAllTransitionProbabilities();
  }
  else
  {
    vector<int> ids;
    vector<string> tmp = params.getCommonParametersWith(_modelSet->getParameters()).getParameterNames();
    for(unsigned int i = 0; i < tmp.size(); i++)
    {
      vector<int> tmpv = _modelSet->getNodesWithParameter(tmp[i]);
      ids = VectorTools::vectorUnion(ids, tmpv);
    }
    tmp = params.getCommonParametersWith(_brLenParameters).getParameterNames();
    vector<int> tmpv;
    bool test = false;
    for(unsigned int i = 0; i < tmp.size(); i++)
    {
      if(!test && (tmp[i] == "BrLenRoot" || tmp[i] == "RootPosition"))
      {
        tmpv.push_back(_root1);
        tmpv.push_back(_root2);
        test = true; //Add only once.
      }
      else
        tmpv.push_back(TextTools::toInt(tmp[i].substr(5)));
    }
    ids = VectorTools::vectorUnion(ids, tmpv);

    for(unsigned int i = 0; i < ids.size(); i++)
    {
      computeTransitionProbabilitiesForNode(_tree->getNode(ids[i]));
    }
    _rootFreqs = _modelSet->getRootFrequencies();
  }
  computeTreeLikelihood();
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getValue() const
throw (Exception)
{
  //double f = - getLogLikelihood(); // For minimization.
  //if(isnan(f)) f = -log(0.); // (+inf if unlikely!)
  //return f;
  return - getLogLikelihood();
}

/******************************************************************************
 *                           First Order Derivatives                          *
 ******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getDLikelihoodForASiteForARateClass(
  unsigned int site,
  unsigned int rateClass) const
{
  double dl = 0;
  Vdouble * dla = & _likelihoodData->getDLikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass];
  for(unsigned int i = 0; i < _nbStates; i++)
  {
    dl += (* dla)[i] * _rootFreqs[i];
  }
  return dl;
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getDLikelihoodForASite(unsigned int site) const
{
  // Derivative of the sum is the sum of derivatives:
  double dl = 0;
  for(unsigned int i = 0; i < _nbClasses; i++)
    dl += getDLikelihoodForASiteForARateClass(site, i) * _rateDistribution->getProbability(i);
  return dl;
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getDLogLikelihoodForASite(unsigned int site) const
{
  // d(f(g(x)))/dx = dg(x)/dx . df(g(x))/dg :
  return getDLikelihoodForASite(site) / getLikelihoodForASite(site);
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getDLogLikelihood() const
{
  // Derivative of the sum is the sum of derivatives:
  double dl = 0;
  for(unsigned int i = 0; i < _nbSites; i++)
    dl += getDLogLikelihoodForASite(i);
  return dl;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getFirstOrderDerivative(const string & variable) const
throw (Exception)
{ 
  Parameter * p = _parameters.getParameter(variable);
  if(p == NULL) throw ParameterNotFoundException("HomogeneousTreeLikelihood::df", variable);
  if(getRateDistributionParameters().getParameter(variable) != NULL)
  {
    throw Exception("Derivatives respective to rate distribution parameter are not implemented.");
  }
  if(getSubstitutionModelParameters().getParameter(variable) != NULL)
  {
    throw Exception("Derivatives respective to substitution model parameters are not implemented.");
  }
 
  if(variable == "BrLenRoot")
  {
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root1));
    double d1 = - getDLogLikelihood();
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root2));
    double d2 = - getDLogLikelihood();
    double pos = _brLenParameters.getParameter("RootPosition")->getValue();
    return pos * d1 + (1. - pos) * d2;
  }
  else if(variable == "RootPosition")
  {
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root1));
    double d1 = - getDLogLikelihood();
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root2));
    double d2 = - getDLogLikelihood();
    double len = _brLenParameters.getParameter("BrLenRoot")->getValue();
    return len * (d1 - d2);
  }
  else
  {
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood(variable);
    return - getDLogLikelihood();
  }
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::computeTreeDLikelihood(const string & variable)
{  
  // Get the node with the branch whose length must be derivated:
  int brI = TextTools::toInt(variable.substr(5));
  const Node * branch = _nodes[brI];
  const Node * father = branch->getFather();
  VVVdouble * _dLikelihoods_father = & _likelihoodData->getDLikelihoodArray(father);
  
  // Compute dLikelihoods array for the father node.
  // Fist initialize to 1:
  unsigned int nbSites  = _dLikelihoods_father->size();
  for(unsigned int i = 0; i < nbSites; i++)
  {
    VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
    for(unsigned int c = 0; c < _nbClasses; c++)
    {
      Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
      for(unsigned int s = 0; s < _nbStates; s++)
      {
        (* _dLikelihoods_father_i_c)[s] = 1.;  
      }
    }
  }

  unsigned int nbNodes = father->getNumberOfSons();
  for(unsigned int l = 0; l < nbNodes; l++)
  {
    const Node * son = father->getSon(l);

    vector <unsigned int> * _patternLinks_father_son = & _likelihoodData->getArrayPositions(father, son);
    VVVdouble * _likelihoods_son = & _likelihoodData->getLikelihoodArray(son);

    if(son == branch)
    {
      VVVdouble * _dpxy_son = & _dpxy[son->getId()];
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
          VVdouble * _dpxy_son_c = & (* _dpxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double dl = 0;
            Vdouble * _dpxy_son_c_x = & (* _dpxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              dl += (* _dpxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _dLikelihoods_father_i_c)[x] *= dl;
          }
        }
      }
    }
    else
    {
      VVVdouble * _pxy_son = & _pxy[son->getId()];
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double dl = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              dl += (* _pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _dLikelihoods_father_i_c)[x] *= dl;
          }
        }
      }
    }
  }

  // Now we go down the tree toward the root node:
  computeDownSubtreeDLikelihood(father);
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::computeDownSubtreeDLikelihood(const Node * node)
{
  const Node * father = node->getFather();
  // We assume that the _dLikelihoods array has been filled for the current node 'node'.
  // We will evaluate the array for the father node.
  if(father == NULL) return; // We reached the root!
    
  // Compute dLikelihoods array for the father node.
  // Fist initialize to 1:
  VVVdouble * _dLikelihoods_father = & _likelihoodData->getDLikelihoodArray(father);
  unsigned int nbSites  = _dLikelihoods_father->size();
  for(unsigned int i = 0; i < nbSites; i++)
  {
    VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
    for(unsigned int c = 0; c < _nbClasses; c++)
    {
      Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
      for(unsigned int s = 0; s < _nbStates; s++)
      {
        (* _dLikelihoods_father_i_c)[s] = 1.;  
      }
    }
  }

  unsigned int nbNodes = father->getNumberOfSons();
  for(unsigned int l = 0; l < nbNodes; l++)
  {
    const Node * son = father->getSon(l);

    VVVdouble * _pxy_son = & _pxy[son->getId()];
    vector <unsigned int> * _patternLinks_father_son = & _likelihoodData->getArrayPositions(father,son);
  
    if(son == node)
    {
      VVVdouble * _dLikelihoods_son = & _likelihoodData->getDLikelihoodArray(son);
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _dLikelihoods_son_i = & (* _dLikelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _dLikelihoods_son_i_c = & (* _dLikelihoods_son_i)[c];
          Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double dl = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              dl +=(* _pxy_son_c_x)[y] * (* _dLikelihoods_son_i_c)[y];
            }
            (* _dLikelihoods_father_i_c)[x] *= dl;
          }
        }
      }
    }
    else
    {
      VVVdouble * _likelihoods_son = & _likelihoodData->getLikelihoodArray(son);
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _dLikelihoods_father_i = & (* _dLikelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _dLikelihoods_father_i_c = & (* _dLikelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double dl = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              dl += (* _pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _dLikelihoods_father_i_c)[x] *= dl;
          }
        }
      }
    }
  }

  //Next step: move toward grand father...
  computeDownSubtreeDLikelihood(father);
}
  
/******************************************************************************
 *                           Second Order Derivatives                         *
 ******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getD2LikelihoodForASiteForARateClass(
  unsigned int site,
  unsigned int rateClass) const
{
  double d2l = 0;
  Vdouble * d2la = & _likelihoodData->getD2LikelihoodArray(_tree->getRootNode())[_likelihoodData->getRootArrayPosition(site)][rateClass];
  for(unsigned int i = 0; i < _nbStates; i++)
  {
    d2l += (* d2la)[i] * _rootFreqs[i];
  }
  return d2l;
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getD2LikelihoodForASite(unsigned int site) const
{
  // Derivative of the sum is the sum of derivatives:
  double d2l = 0;
  for(unsigned int i = 0; i < _nbClasses; i++)
    d2l += getD2LikelihoodForASiteForARateClass(site, i) * _rateDistribution->getProbability(i);
  return d2l;
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getD2LogLikelihoodForASite(unsigned int site) const
{
  return getD2LikelihoodForASite(site) / getLikelihoodForASite(site)
  - pow( getDLikelihoodForASite(site) / getLikelihoodForASite(site), 2);
}

/******************************************************************************/  

double RNonHomogeneousTreeLikelihood::getD2LogLikelihood() const
{
  // Derivative of the sum is the sum of derivatives:
  double dl = 0;
  for(unsigned int i = 0; i < _nbSites; i++)
    dl += getD2LogLikelihoodForASite(i);
  return dl;
}

/******************************************************************************/

double RNonHomogeneousTreeLikelihood::getSecondOrderDerivative(const string & variable) const
throw (Exception)
{
  Parameter * p = _parameters.getParameter(variable);
  if(p == NULL) throw ParameterNotFoundException("HomogeneousTreeLikelihood::df", variable);
  if(getRateDistributionParameters().getParameter(variable) != NULL)
  {
    throw Exception("Derivatives respective to rate distribution parameter are not implemented.");
  }
  if(getSubstitutionModelParameters().getParameter(variable) != NULL)
  {
    throw Exception("Derivatives respective to substitution model parameters are not implemented.");
  }
  
  if(variable == "BrLenRoot")
  {
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root1));
    double d1 = - getDLogLikelihood();
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeD2Likelihood("BrLen" + TextTools::toString(_root1));
    double d21 = - getD2LogLikelihood();
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root2));
    double d2 = - getDLogLikelihood();
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeD2Likelihood("BrLen" + TextTools::toString(_root2));
    double d22 = - getD2LogLikelihood();
    double pos = _brLenParameters.getParameter("RootPosition")->getValue();
    return pos * pos * d21 + (1. - pos) * (1. - pos) * d22 - 2 * pos * (1. - pos) * d1 * d2;
  }
  else if(variable == "RootPosition")
  {
    
    vector<double> d(_nbSites);
    for(unsigned int i = 0; i < _nbSites; i++)
      d[i] = - getLikelihoodForASite(i);
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root1));
    vector<double> d1(_nbSites);
    for(unsigned int i = 0; i < _nbSites; i++)
      d1[i] = - getDLikelihoodForASite(i);
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeD2Likelihood("BrLen" + TextTools::toString(_root1));
    vector<double> d21(_nbSites);
    for(unsigned int i = 0; i < _nbSites; i++)
      d21[i] = - getD2LikelihoodForASite(i);
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeDLikelihood("BrLen" + TextTools::toString(_root2));
    vector<double> d2(_nbSites);
    for(unsigned int i = 0; i < _nbSites; i++)
      d2[i] = - getDLikelihoodForASite(i);
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeD2Likelihood("BrLen" + TextTools::toString(_root2));
    vector<double> d22(_nbSites);
    for(unsigned int i = 0; i < _nbSites; i++)
      d22[i] = - getD2LikelihoodForASite(i);
    double len = _brLenParameters.getParameter("BrLenRoot")->getValue();
    double dl = 0;
    for(unsigned int i = 0; i < _nbSites; i++)
      dl += len * len * (d21[i] + d22[i]) / d[i]  - std::pow(len * (d1[i] - d2[i]) / d[i], 2.);
    return dl;
  }
  else
  {
    const_cast<RNonHomogeneousTreeLikelihood *>(this)->computeTreeD2Likelihood(variable);
    return - getD2LogLikelihood();
  }
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::computeTreeD2Likelihood(const string & variable)
{
  // Get the node with the branch whose length must be derivated:
  int brI = TextTools::toInt(variable.substr(5));
  Node * branch = _nodes[brI];
  Node * father = branch->getFather();
  
  // Compute dLikelihoods array for the father node.
  // Fist initialize to 1:
  VVVdouble * _d2Likelihoods_father = & _likelihoodData->getD2LikelihoodArray(father); 
  unsigned int nbSites  = _d2Likelihoods_father->size();
  for(unsigned int i = 0; i < nbSites; i++)
  {
    VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
    for(unsigned int c = 0; c < _nbClasses; c++)
    {
      Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
      for(unsigned int s = 0; s < _nbStates; s++)
      {
        (* _d2Likelihoods_father_i_c)[s] = 1.;  
      }
    }
  }

  unsigned int nbNodes = father->getNumberOfSons();
  for(unsigned int l = 0; l < nbNodes; l++)
  {
    const Node * son = father->getSon(l);
    
    vector <unsigned int> * _patternLinks_father_son = & _likelihoodData->getArrayPositions(father,son);
    VVVdouble * _likelihoods_son = & _likelihoodData->getLikelihoodArray(son);

    if(son == branch)
    {
      VVVdouble * _d2pxy_son = & _d2pxy[son->getId()];
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
          VVdouble * _d2pxy_son_c = & (* _d2pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double d2l = 0;
            Vdouble * _d2pxy_son_c_x = & (* _d2pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              d2l += (* _d2pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _d2Likelihoods_father_i_c)[x] *= d2l;
          }
        }
      }
    }
    else
    {
      VVVdouble * _pxy_son = & _pxy[son->getId()];
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double d2l = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              d2l += (* _pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _d2Likelihoods_father_i_c)[x] *= d2l;
          }
        }
      }
    }  
  }

  // Now we go down the tree toward the root node:
  computeDownSubtreeD2Likelihood(father);
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::computeDownSubtreeD2Likelihood(const Node * node)
{
  const Node * father = node->getFather();
  // We assume that the _dLikelihoods array has been filled for the current node 'node'.
  // We will evaluate the array for the father node.
  if(father == NULL) return; // We reached the root!
    
  // Compute dLikelihoods array for the father node.
  // Fist initialize to 1:
  VVVdouble * _d2Likelihoods_father = & _likelihoodData->getD2LikelihoodArray(father);
  unsigned int nbSites  = _d2Likelihoods_father->size();
  for(unsigned int i = 0; i < nbSites; i++)
  {
    VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
    for(unsigned int c = 0; c < _nbClasses; c++)
    {
      Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
      for(unsigned int s = 0; s < _nbStates; s++)
      {
        (* _d2Likelihoods_father_i_c)[s] = 1.;  
      }
    }
  }

  unsigned int nbNodes = father->getNumberOfSons();
  for(unsigned int l = 0; l < nbNodes; l++)
  {
    const Node * son = father->getSon(l);

    VVVdouble * _pxy_son = & _pxy[son->getId()];
    vector <unsigned int> * _patternLinks_father_son = & _likelihoodData->getArrayPositions(father,son);
  
    if(son == node)
    {
      VVVdouble * _d2Likelihoods_son = & _likelihoodData->getD2LikelihoodArray(son);
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _d2Likelihoods_son_i = & (* _d2Likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _d2Likelihoods_son_i_c = & (* _d2Likelihoods_son_i)[c];
          Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double d2l = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              d2l += (* _pxy_son_c_x)[y] * (* _d2Likelihoods_son_i_c)[y];
            }
            (* _d2Likelihoods_father_i_c)[x] *= d2l;
          }
        }
      }
    }
    else
    {
      VVVdouble * _likelihoods_son = & _likelihoodData->getLikelihoodArray(son);
      for(unsigned int i = 0; i < nbSites; i++)
      {
        VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_father_son)[i]];
        VVdouble * _d2Likelihoods_father_i = & (* _d2Likelihoods_father)[i];
        for(unsigned int c = 0; c < _nbClasses; c++)
        {
          Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
          Vdouble * _d2Likelihoods_father_i_c = & (* _d2Likelihoods_father_i)[c];
          VVdouble * _pxy_son_c = & (* _pxy_son)[c];
          for(unsigned int x = 0; x < _nbStates; x++)
          {
            double dl = 0;
            Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
            for(unsigned int y = 0; y < _nbStates; y++)
            {
              dl += (* _pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
            }
            (* _d2Likelihoods_father_i_c)[x] *= dl;
          }
        }
      }
    }
  }

  //Next step: move toward grand father...
  computeDownSubtreeD2Likelihood(father);
}

/******************************************************************************/
  
void RNonHomogeneousTreeLikelihood::computeTreeLikelihood()
{
  computeSubtreeLikelihood(_tree->getRootNode());
}

/******************************************************************************/  

void RNonHomogeneousTreeLikelihood::computeSubtreeLikelihood(const Node * node)
{
  if(node->isLeaf()) return;

  unsigned int nbSites  = _likelihoodData->getLikelihoodArray(node).size();
  unsigned int nbNodes  = node->getNumberOfSons();
    
  // Must reset the likelihood array first (i.e. set all of them to 1):
  VVVdouble * _likelihoods_node = & _likelihoodData->getLikelihoodArray(node);
  for(unsigned int i = 0; i < nbSites; i++)
  {
    //For each site in the sequence,
    VVdouble * _likelihoods_node_i = & (* _likelihoods_node)[i];
    for(unsigned int c = 0; c < _nbClasses; c++)
    {
      //For each rate classe,
      Vdouble * _likelihoods_node_i_c = & (* _likelihoods_node_i)[c];
      for(unsigned int x = 0; x < _nbStates; x++)
      {
        //For each initial state,
        (* _likelihoods_node_i_c)[x] = 1.;
      }
    }
  }

  for(unsigned int l = 0; l < nbNodes; l++)
  {
    //For each son node,  

    const Node * son = node->getSon(l);
    
    computeSubtreeLikelihood(son); //Recursive method:
    
    VVVdouble * _pxy_son = & _pxy[son->getId()];
    vector <unsigned int> * _patternLinks_node_son = & _likelihoodData->getArrayPositions(node,son);
    VVVdouble * _likelihoods_son = & _likelihoodData->getLikelihoodArray(son);

    for(unsigned int i = 0; i < nbSites; i++)
    {
      //For each site in the sequence,
      VVdouble * _likelihoods_son_i = & (* _likelihoods_son)[(* _patternLinks_node_son)[i]];
      VVdouble * _likelihoods_node_i = & (* _likelihoods_node)[i];
      for(unsigned int c = 0; c < _nbClasses; c++)
      {
        //For each rate classe,
        Vdouble * _likelihoods_son_i_c = & (* _likelihoods_son_i)[c];
        Vdouble * _likelihoods_node_i_c = & (* _likelihoods_node_i)[c];
        VVdouble * _pxy_son_c = & (* _pxy_son)[c];
        for(unsigned int x = 0; x < _nbStates; x++)
        {
          //For each initial state,
          Vdouble * _pxy_son_c_x = & (* _pxy_son_c)[x];
          double likelihood = 0;
          for(unsigned int y = 0; y < _nbStates; y++)
          {
            likelihood += (* _pxy_son_c_x)[y] * (* _likelihoods_son_i_c)[y];
          }
          (* _likelihoods_node_i_c)[x] *= likelihood;
        }
      }
    }
  }
}

/******************************************************************************/

void RNonHomogeneousTreeLikelihood::displayLikelihood(const Node * node)
{
  cout << "Likelihoods at node " << node->getName() << ": " << endl;
  displayLikelihoodArray(_likelihoodData->getLikelihoodArray(node));
  cout << "                                         ***" << endl;
}

/*******************************************************************************/
