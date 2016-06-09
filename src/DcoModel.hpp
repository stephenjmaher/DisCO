#ifndef DcoModel_hpp_
#define DcoModel_hpp_

#include <BcpsModel.h>
#include <OsiConicSolverInterface.hpp>
#include <OsiLorentzCone.hpp>
#include <BcpsBranchStrategy.h>


#include "DcoParams.hpp"
#include "DcoConstraint.hpp"

class DcoConGenerator;
class DcoSolution;
class DcoHeuristic;

class CglCutGenerator;
class CglConicCutGenerator;

/**
   Represents a discrete conic optimization problem (master problem).
   Some set of rows/columns will be relaxed in this problem to get subproblems
   represented by the branch and bound tree nodes.

   # Fields of DcoModel
   relaxedCols_ keeps the set of relaxed columns. relaxedRows_ keeps the set
   of relaxed rows. We relax integer columns only, their integrality
   constraint is relaxed. When OA algorithm is used we relax rows corresponding
   to conic constraints.

   In DcoModel columns are stored in cols_ inherited from BcpsModel. We keep
   number of integer variables at numIntegerCols_ and their indices at (int
   * intColIndices_). intColIndices_[0] gives the index of the first integer
   column in the cols_ array.

   DcoModel objects are kept at constraints_ and variables_ inherited from
   BcpsModel.

   In Blis (MILP solver built on top of Bcps), integer variables have their own
   class, BlisObjectInt.  BlisObjectInt inherits BcpsObject class.

   # Cut generation

   Pointers to cut generators are stored in conGenerators_. Type of generators
   are DcoConGenerator. DcoConGenerator is an abstract base class (ABC) for
   constraint generators.  Two different classes implements this ABC,
   DcoLinearConGenerator and DcoConicConGenerator. DcoConGenerator has a single
   pure virtual function, generateConstraints(conPool). This function generates
   constraints and add them to the given pool.

   DcoLinearConGenerator implements linear cuts. CglCutGenerator is given as an
   input to constructor.

   DcoConicConGenerator implements generating supports for conic
   constraints. Constructor takes a CglConicCutGenerator as an input.

   # Heuristics

   # Bcps ideas for future

   We keep relaxed cols/rows (the whole object or integrality of the object) in
   BcpsModel in list BcpsObject ** relaxed_. This way Bcps can check the
   feasibility of its relaxed objects through virtual functions that will be
   impelemnted in Disco level.

   Can subproblems have different relaxed objects? Yes they can. But make sure
   all relaxed objects are in the relaxed_. If subproblems have different
   relaxed objects we will get a more depocposition like algorithm. Moreover if
   not all integrality is relaxed we might need a discrete solver (a DcoModel
   instance?) to solve the problem.

   Subproblems might have different solvers? A subprobllem might be an LP, SOCO,
   MILP or even MISOCO. How will DcoTreeNode::bound know about the solver to be
   used?

   When subproblem is solved with the solver we go back and check the
   feasibility of the objects that are relaxed in the subproblem (objects in
   relaxed_). This is done through infeasible() virtual function defined in
   BcpsObject.
   After this check we have a list of objects that are infeasible. At this
   point we need to decide what to do with them. Options are (1) generate
   cuts (using generateConstraints()), (2) lift the subproblem (using
   generateVariables()), (3) branch.
   (1) We can generate cuts when infeasible objects are continuous or
   integer. Generate a separating hyperplane that cuts the subproblem solution
   from feasible region.
   (2) Branching when the object is continuous. This is similar to branch
   on variables. Branching procedure should create new subproblems where
   infeasible object has new upper and lower bounds.

   feasibility checking should be implemented in Bcps level. Itertate over
   cols/rows and check their feasiblity. Store infeasible cols (BcpsVariable)
   and rows (BcpsConstraint). This function checks the feasibility of the
   solution stored in the solver at the time of the call.

   BcpsModel::feasibleSolution() should be able to check cols or rows only.
   In a typical branch and bound we need to check feasibility of cols only.
   In DisCO we may want to check both or check cols only.

   # DisCO ideas

   # Style guides

   <ul>

   <li> If a function does not fit in the same line of its decleration in the
   header file then it is not supposed to be there. Move it to .cpp file.

   <li> All functions defined in .hpp are already inline since bla.bla version
   of gcc. This makes inline key-words in the header files redundants.  I will
   just remove them, since I can not bear redundant stuff floating around.

   <li> Define pointers as "Type * name". * is not next to Type or variable
   name. * is separate since it is neither part of the Type nor variable
   name. This is the way recommended by Bjarne Stroustrup and it makes sense.

   <il> We use const specifiers as suggested by Bjarne Stroustrup in his book.
   Check the code to see how I declare const objects, check the book to see
   why.

   </ul>

*/

class DcoModel: public BcpsModel {
  /// Subproblem solver.
#if defined(__OA__)
  OsiSolverInterface * solver_;
#else
  OsiConicSolverInterface * solver_;
#endif
  ///@name Variable and constraint bounds.
  //@{
  /// Column lower bound.
  double * colLB_;
  /// Column upper bound.
  double * colUB_;
  /// Row lower bound.
  double * rowLB_;
  /// Row upper bound.
  double * rowUB_;
  //@}

  ///@name Number of columns and rows
  //@{
  /// Number of columns.
  int numCols_;
  /// Number of rows (constraints), linear + conic.
  int numRows_;
  /// Number of linear rows.
  int numLinearRows_;
  /// Number of conic rows.
  int numConicRows_;
  //@}

  /// Problem matrix (linear constraints only).
  CoinPackedMatrix * matrix_;

  ///@name Objective function
  //@{
  double objSense_;
  double * objCoef_;
  //@}

  ///@name Column types
  //@{
  /// Number of integer columns in the problem.
  int numIntegerCols_;
  /// Indices of integer columns. Columns are stored in cols_ inherited from
  /// BcpsModel. Size of numIntegerCols_.
  int * integerCols_;
  //@}

  ///@name Solution related
  //@{
  /// Current relative optimal gap.
  double currRelGap_;
  /// Current absolute optimal gap.
  double currAbsGap_;
  /// quality of best solution so far
  double bestQuality_;
  //@}

  ///@name Variable selection function.
  //@{
  /// Branchs strategy.
  BcpsBranchStrategy * branchStrategy_;
  /// Ramp up branch strategy.
  BcpsBranchStrategy * rampUpBranchStrategy_;
  //@}

  /// Active node.
  AlpsTreeNode * activeNode_;

  ///@name Dco parameters.
  //@{
  /// DisCO parameter.
  DcoParams * dcoPar_;
  //@}

  ///@name Statistics
  //@{
  /// Number of processed nodes.
  int numNodes_;
  /// Number of lp(Simplex) iterations.
  int numIterations_;
  /// Average number of lp iterations to solve a subproblem.
  int aveIterations_;
  //@}

  ///@name Relaxed objects data.
  //@{
  /// Number of relaxed columns
  int numRelaxedCols_;
  /// Array of indices to relaxed columns.
  int * relaxedCols_;
  /// Number of relaxed rows
  int numRelaxedRows_;
  /// Array of indices to relaxed rows.
  int * relaxedRows_;
  //@}

  ///@name Heuristics
  //@{
  DcoHeurStrategy heurStrategy_;
  int heurFrequency_;
  std::vector<DcoHeuristic*> heuristics_;
  // get a specific heuristic
  DcoHeuristic * heuristics(int i) { return heuristics_[i]; }
  //@}

  ///@name Cut generator related.
  //@{
  /// global cut strategy, it will be set with respect to specific cut
  /// strategies. It will be set to the most allowing one, ie. if we have
  /// strategies with root and periodic calls, it will be set to periodic.
  DcoCutStrategy cutStrategy_;
  /// Cut generation frequency, it will be set with respect to specific cut
  /// strategies. It will be set to the most frequent one, ie. if we have
  /// strategies with frequencies 10 and 20, it will be set to 10.
  int cutGenerationFrequency_;
  /// Constraint generators.
  std::vector<DcoConGenerator*> conGenerators_;
  //@}


  // Private Functions
  ///@name Read Helpers
  //@{
  /// Add variables to the model. Helps readInstance function.
  void readAddVariables(CoinMpsIO * reader);
  /// Add linear constraints to the model. Helps readInstance function.
  void readAddLinearConstraints(CoinMpsIO * reader);
  /// Add conic constraints to the model. Helps readInstance function.
  void readAddConicConstraints(CoinMpsIO * reader);
  //@}

  ///@name Setup Helpers
  //@{
  /// Set log levels, Alps, Bcps and Disco
  void setMessageLevel();
  /// Set branching strategy from parameters.
  void setBranchingStrategy();
  /// Add constraint generators with respect to parameters.
  void addConstraintGenerators();
  /// Add heuristics
  void addHeuristics();
  //@}

  /// write parameters to oustream
  void writeParameters(std::ostream& outstream) const;
public:
  ///@name Message printing
  //@{
  /// DisCO message handler.
  CoinMessageHandler * dcoMessageHandler_;
  /// DisCO messages.
  CoinMessages * dcoMessages_;
  //@}
public:
  ///@name Constructors and Destructors
  //@{
  /// Default constructor.
  DcoModel();
  /// Destructor.
  virtual ~DcoModel();
  //@}

  ///@name Solver related
  //@{
  /// Set solver
#if defined(__OA__)
  void setSolver(OsiSolverInterface * solver);
#else
  void setSolver(OsiConicSolverInterface * solver);
#endif
  /// Get solver
#if defined(__OA__)
  OsiSolverInterface * solver() {return solver_;}
#else
  OsiConicSolverInterface * solver() {return solver_;}
#endif
  //@}

  ///@name Other functions
  //@{
  /// Approximate cones.
  void approximateCones();
  /// return to branch strategy.
  BcpsBranchStrategy * branchStrategy() {return branchStrategy_;}
  /// return Dco Parameter
  DcoParams const * dcoPar() const {return dcoPar_;}
  /// store solution
  int storeSolution(DcoSolution * sol);
  /// get upper bound of the objective value for minimization
  double bestQuality();
  //@}

  ///@name Querry problem data
  //@{
  /// Get number of core variables.
  int getNumCoreVariables() const {return numCols_;}
  /// Get number of core linear constraints.
  int getNumCoreLinearConstraints() const {return numLinearRows_;}
  /// Get number of core conic constraints.
  int getNumCoreConicConstraints() const {return numConicRows_;}
  /// Get column lower bounds.
  double * colLB() {return colLB_;}
  /// Get column upper bounds.
  double * colUB() {return colUB_;}
  /// Get row lower bounds.
  double * rowLB() {return rowLB_;}
  /// Get row upper bounds.
  double * rowUB() {return rowUB_;}
  /// Get objective sense, 1 for min, -1 for max
  double objSense() const { return objSense_; }
  //@}

  ///@name Querry relaxed problem objects
  //@{
  /// Get number of relaxed columns.
  int numRelaxedCols() const {return numRelaxedCols_;}
  /// Get array of indices to relaxed columns.
  int const * relaxedCols() const {return relaxedCols_;}
  /// Get number of relaxed rows
  int numRelaxedRows() const {return numRelaxedRows_;}
  /// Get array of indices to relaxed rows.
  int const * relaxedRows() const {return relaxedRows_;}
  //@}

  ///@name Constraint Generation related.
  //@{
  /// Add constraint generator using linear Cgl.
  void addConGenerator(CglCutGenerator * cgl_gen, char const * name,
                       DcoCutStrategy dco_strategy, int frequency);
  /// Add constraint generator using conic Cgl.
  void addConGenerator(CglConicCutGenerator * cgl_gen, char const * name,
                       DcoCutStrategy dco_strategy, int frequency);
  /// Add constraint generator.
  void addConGenerator(DcoConGenerator * dco_gen);
  /// Get the number of constraint generators.
  int numConGenerators() const { return conGenerators_.size(); }
  /// Get a specific constraint generator.
  DcoConGenerator * conGenerators(int i) const { return conGenerators_[i]; }
  /// Get global cut strategy. It will be set using specific cut strategies, to
  /// the most allowing one. If we have strategies with root and periodic
  /// calls, it will be set to periodic.
  DcoCutStrategy cutStrategy() const {return cutStrategy_;}
  /// Set global cut strategy. It will be set using specific cut strategies, to
  /// the most allowing one. If we have strategies with root and periodic
  /// calls, it will be set to periodic.
  void setCutStrategy(DcoCutStrategy strategy) {cutStrategy_ = strategy;}
  //@}

  /// Check feasiblity of subproblem solution, store number of infeasible
  /// columns and rows.
  virtual DcoSolution * feasibleSolution(int & numInfColumns,
                                         int & numInfRows);

  ///@name Virtual functions from AlpsModel
  //@{
  /// Read in the problem instance. Currently linear Mps files and Mosek
  /// style conic mps files.
  virtual void readInstance(char const * dataFile);
  /// Reads in parameters.
  /// This function is called from AlpsKnowledgeBrokerSerial::initializeSearch
  /// It reads and stores the parameters in alpsPar_ inherited from AlpsModel.
  virtual void readParameters(int const argnum, char const * const * arglist);
  /// Do necessary work to make model ready for use, such as classify
  /// variable and constraint types.
  /// Called from AlpsKnowledgeBrokerSerial::initializeSearch. Called
  /// after readParameters and preprocess.
  virtual bool setupSelf();
  /// Preprocessing the model. Default does nothing. We do not have any
  /// preprocessing for now.
  /// Called from AlpsKnowledgeBrokerSerial::initializeSearch. Called
  /// after readParameters and before setupSelf.
  virtual void preprocess();
  /// Postprocessing the model. Default does nothing. We do not have any
  /// postprocessing for now.
  virtual void postprocess();
  /// Create the root node.
  virtual AlpsTreeNode * createRoot();
  /// This is called at the end of the AlpsKnowledgeBroker::rootSearch
  /// Prints solution statistics
  virtual void modelLog();
  //@}

  /// report feasibility of the best solution
  void reportFeasibility();

};

#endif
