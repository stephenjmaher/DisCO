/*===========================================================================*
 * This file is part of the BiCePS Linear Integer Solver (BLIS).             *
 *                                                                           *
 * BLIS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *                                                                           *
 * Conceptual Design:                                                        *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           *
 *                                                                           *
 * Copyright (C) 2001-2015, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * All Rights Reserved.                                                      *
 *===========================================================================*/

#include "DcoMessage.hpp"
#include <cstring>

//#############################################################################
typedef struct {
    DISCO_Message internalNumber;
    int externalNumber;              // or continuation
    char detail;
    const char * message;
} Dco_message;

//#############################################################################

// From CoinMessages documentation
// <3000 are informational ('I')
// <6000 warnings ('W')
// <9000 non-fatal errors ('E')
// >=9000 aborts the program (after printing the message) ('S')

// From CoinMessageHandler documentation
// log (detail) levels explained.
//
// If the log level is equal or greater than the detail level of a message, the
// message will be printed. A rough convention for the amount of output
// expected is
//
// 0 - none
// 1 - minimal
// 2 - normal low
// 3 - normal high
// 4 - verbose
//
// Please assign log levels to messages accordingly. Log levels of 8 and above
// (8,16,32, etc.) are intended for selective debugging. The logical AND of the
// log level specified in the message and the current log level is used to
// determine if the message is printed. (In other words, you're using
// individual bits to determine which messages are printed.)

// Log messages external numbers
//
// 100-199 DcoModel::readInstance information messages
// 200-299 DcoTreeNode information messages
// 300-399 Constraint generation information messages
// 400-499 Relaxation solver information messages
// 6200-6299 DcoTreeNode warnings
// 6300-6399 Constraint generation warnings
// 6400-6499 Relaxation solver warnings
// 9200-9299 DcoTreeNode error messages
// 9900-9999 general error messages
// 9300-9399 Constraint generation errors
// 6400-6499 Relaxation solver errors
//
//

// Debug levels, assume 32 bit integers, first 3 bits are already in use
// by log levels, we will skip 32th bit, this leaves us with 28 bits.
// We can decide 28 different debug issues, branching, cutting, etc.
//
// Bit number   use
// 0,1,2        reserved for log levels
// 3            debug branch
// 4            debug cut generation
// 5            debug node process
//
//
//

static Dco_message us_english[]=
{
    {DISCO_CUTOFF_INC, 43, 1, "Objective coefficients are multiples of %g"},
    {DISCO_CUT_STAT_FINAL, 53, 1, "Called %s cut generator %d times, generated %d cuts, CPU time %.4f seconds, current strategy %d"},
    {DISCO_CUT_STAT_NODE, 55, 1, "Node %d, called %s cut generator %d times, generated %d cuts, CPU time %.4f seconds, current strategy %d"},
    {DISCO_GAP_NO, 57, 1, "Relative optimality gap is infinity because no solution was found"},
    {DISCO_GAP_YES, 58, 1, "Relative optimality gap is %.2f%%"},
    {DISCO_HEUR_BEFORE_ROOT, 60, 1, "%s heuristic found a solution; quality is %g"},
    {DISCO_HEUR_STAT_FINAL, 63, 1, "Called %s heuristic %d times, found %d solutions, CPU time %.4f seconds, current strategy %d"},
    {DISCO_HEUR_STAT_NODE, 65, 1, "Node %d, called %s heuristic %d times, found %d solutions, CPU time %.4f seconds, current strategy %d"},
    {DISCO_ROOT_PROCESS, 30, 1, "Processing the root node (%d rows, %d columns)"},
    {DISCO_ROOT_TIME, 35, 1, "Processing the first root relaxation took CPU time %.4f seconds"},
    // reading mps files
    {DISCO_READ_NOINTS, 20, 1, "Problem does not have integer variables"},
    {DISCO_READ_NOCONES, 21, 1, "Problem does not have conic constraints."},
    {DISCO_READ_MPSERROR, 9001, 1, "Reading conic mps file failed with code %d." },
    {DISCO_READ_MPSFILEONLY,9002, 1, "Mosek style conic mps files only."},
    {DISCO_READ_CONEERROR, 9002, 1, "Invalid cone type."},
    {DISCO_READ_ROTATEDCONESIZE, 9002, 1, "Rotated cones should have at least 3 members."},
    {DISCO_READ_CONESTATS1, 101, 1, "Problem has %d cones."},
    {DISCO_READ_CONESTATS2, 102, 1, "Cone %d has %d entries (type %d)"},
    // tree node
    {DISCO_NODE_BRANCHONINT, 9201, 1, "Branched on integer variable. Variable index %d."},
    {DISCO_NODE_UNEXPECTEDSTATUS,9202,1, "Unexpected node status %d"},
    // constraint generation
    {DISCO_INVALID_CUT_FREQUENCY,9301,1, "%d is not a valid cut frequency, changed it to %d."},
    // relaxation solver messages
    {DISCO_SOLVER_UNKNOWN_STATUS,9401,1, "Unknown relaxation solver status."},
    {DISCO_SOLVER_FAILED,9402,1, "Relaxation solver failed to solve the subproblem."},
    // general messages
    {DISCO_OUT_OF_MEMORY,9901,1, "Out of memory, file: %s, line: %d."},
    {DISCO_NOT_IMPLEMENTED,9902,1, "Not implemented yet, file: %s, line: %d."},
    {DISCO_UNKNOWN_CONETYPE,9903,1, "Unknown cone type %d"},
    {DISCO_UNKNOWN_BRANCHSTRATEGY,9904,1, "Unknown branch strategy %d"},
    {DISCO_UNKNOWN_CUTSTRATEGY,9905,1, "Unknown cut strategy %d"},
    {DISCO_DUMMY_END, 9999, 0, ""}
};

//#############################################################################

/* Constructor */
DcoMessage::DcoMessage(Language language):
  CoinMessages(sizeof(us_english) / sizeof(Dco_message)) {
  language_ = language;
  strcpy(source_, "Dco");
  Dco_message * message = us_english;
  while (message->internalNumber != DISCO_DUMMY_END) {
    CoinOneMessage oneMessage(message->externalNumber, message->detail,
                              message->message);
    addMessage(message->internalNumber, oneMessage);
    message++;
  }
  // now override any language ones
  switch (language) {
  default:
    message = NULL;
    break;
  }
  // replace if any found
  if (message) {
    while (message->internalNumber != DISCO_DUMMY_END) {
      replaceMessage(message->internalNumber, message->message);
      message++;
    }
  }
}
