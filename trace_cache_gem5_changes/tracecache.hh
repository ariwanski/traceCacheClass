
#ifndef __TRACE_CACHE_HH__
#define __TRACE_CACHE_HH__


#include <iostream>
#include <cmath>
#include <cstdlib>
#include "params/tracecache.hh"
#include "sim/sim_object.hh"

namespace gem5{

// represents one line in the trace cache
class tcLine
{
  public:
    // fields for accessing the tc line
    int   tagAddr;
    int   valid;
    int   branchFlags;

    int insnCount; // number of instructions in this line
    int BBCount; // number of basic blocks in this line

    // Constructor
    tcLine() {
      // set fields for accessing a tc line
      this->tagAddr = 0;
      this->valid = 0;
      this->branchFlags = 0;
      // set fields for building a trace
      this->insnCount = 0;
      this->BBCount = 0;
    }
};

// represents the trace cache as an array of tcLine objects
// This trace cache is currently built to support one branch instruction per
// trace cache line.
class traceCache : public SimObject
{
  public:
    // parameters
    int numSets; // number of sets in the cache
    int assoc; // associativity of the cache

    // fields describing the trace cache
    tcLine* line;
    int    size; // size of the cache in terms of lines
    int    maxNumInsns; // max number of insns in one cache line
    int    maxNumBBs = 1; // max number of basic blocks in one cache line

    // fields for building a trace in the trace cache
    int buildingTrace; // 1 = currently building a trace, 0 = otherwise
    int buildLineIndex; // hold line # is trace cache that trace
                        // is being built for
    tcLine* buildLine; // used to hold stats on tc line currently being built

    // Stats tracking
    int globalHitCount;
    int globalMissCount;


    traceCache() {
        // create an array of lines
        line = new tcLine[1*64];
        // set tc parameter
        this->numSets = 64;
        this->assoc = 1;
        // set tc fields for describing trace cache
        this->size = 1*64;
        this->maxNumInsns = 16;
        this->maxNumBBs = 1;
        // set tc fields for building a trace
        this->buildingTrace = 0;
        this->buildLineIndex = 0;
        this->buildLine = new tcLine();
        // set tc fields for stats tracking
        this->globalHitCount = 0;
        this->globalMissCount = 0;
    }


    // Constructor
    traceCache(int numSets, int assoc, int numInsns, int numBBs) {
        // create an array of lines
        line = new tcLine[assoc*numSets];
        // set tc parameter
        this->numSets = numSets;
        this->assoc = assoc;
        // set tc fields for describing trace cache
        this->size = assoc*numSets;
        this->maxNumInsns = numInsns;
        this->maxNumBBs = numBBs;
        // set tc fields for building a trace
        this->buildingTrace = 0;
        this->buildLineIndex = 0;
        this->buildLine = new tcLine();
        // set tc fields for stats tracking
        this->globalHitCount = 0;
        this->globalMissCount = 0;
    }

    // to be ran every instruction fetch
    void tcInsnFetch(int fetchAddr, int isCondBranch, int branchPred);


    // returns 1 if hit, 0 if miss
    int searchTraceCache(int fetchAddr, int branchPred);

    void logHitStats(int fetchAddr, int branchPred, int i);

    void logMissStats(int fetchAddr, int branchPred);

    void buildTrace(int fetchAddr, int branchPred);

    int searchTraceLine(int fetchAddr, int branchPred, int lineIndex);

    int selectBuildLineIndex(int lowerSearchBound, int upperSearchBound);

    void completeTrace();

};
} // for the namespace gem5

#endif //_TRACE_CACHE_HH__

