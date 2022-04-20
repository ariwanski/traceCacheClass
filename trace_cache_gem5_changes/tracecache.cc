#include "tracecache.hh"
using namespace gem5;
traceCache trace;

// represents one line in the trace cache


// represents the trace cache as an array of tcLine objects
// This trace cache is currently built to support one branch instruction per
// trace cache line.
 

// to be ran every instruction fetch
void traceCache::tcInsnFetch(int fetchAddr, int isCondBranch, int branchPred){
	traceCache trace;

        int traceHit = 0;
        // conditional branch instructions mark the beginning of a new trace
        if(isCondBranch){
            // complete the trace currently being built
            if(trace.buildingTrace){
                trace.completeTrace();
            }

            // look for the current address and prediction in the trace cache
            traceHit = trace.searchTraceCache(fetchAddr, branchPred);

        }else{
            if(trace.buildingTrace){
                // if there is a trace currently being built, add this instruction
                trace.buildTrace(fetchAddr, branchPred);
            } // otherwise, do nothing
        }
    }

// returns 1 if hit, 0 if miss
    int traceCache::searchTraceCache(int fetchAddr, int branchPred){
        // get the index bits from the fetch address
        unsigned int mask = 0;
        int index = 0;
        int lowerSearchBound = 0;
        int upperSearchBound = 0;
        int numIndexBits = log2(trace.numSets);
        int hit = 0;
        mask = (1 << numIndexBits) - 1;
        index = fetchAddr & mask;

        // find bounds of tc lines to access
        lowerSearchBound = index * trace.assoc;
        upperSearchBound = lowerSearchBound + trace.assoc;

        // search all lines in appropriate set for hit
        for(int i = lowerSearchBound; i < upperSearchBound; i++ ){
            hit = searchTraceLine(fetchAddr, branchPred, i);
            if(hit){ // if there is a hit, exit early
                // log the hit statistics
                trace.logHitStats(fetchAddr, branchPred, i);
                return 1;
            }
        }

        // if there is no hit - we have a trace cache miss
        trace.logMissStats(fetchAddr, branchPred);

        // on a miss, we begin building a new trace
        // first, we figure out in which line the new trace should reside
        trace.buildLineIndex = trace.selectBuildLineIndex(lowerSearchBound, upperSearchBound);
        // then, we begin building the trace
        trace.buildTrace(fetchAddr, branchPred);
        return 0;
    }

    void traceCache::logHitStats(int fetchAddr, int branchPred, int i){
        trace.globalHitCount++;
    }

    void traceCache::logMissStats(int fetchAddr, int branchPred){
        trace.globalMissCount++;
    }

    void traceCache::buildTrace(int fetchAddr, int branchPred){
        // if we are not currently building a trace, then this is the first
        // instruction of the trace
        if(trace.buildingTrace == 0){
            // set up trace
            trace.buildLine->tagAddr = fetchAddr; // save the PC
            trace.buildLine->branchFlags = branchPred;
            trace.buildLine->insnCount = 1;
            trace.buildLine->BBCount = 1;
            // tell system we are now building a trace
            trace.buildingTrace = 1;
        }
        // if we are currently building a trace, then we need to add this insn to the trace
        else{
            // increment the instruction count of the line
            trace.buildLine->insnCount++;
        }

        // check whether the trace has reached its max capacity
        if(trace.buildLine->insnCount >= trace.maxNumInsns){
            completeTrace();
        }
    }

    int traceCache::searchTraceLine(int fetchAddr, int branchPred, int lineIndex){
        // check for hit conditions
        if((fetchAddr == trace.line[lineIndex].tagAddr)&
           (branchPred == trace.line[lineIndex].branchFlags)&
           (trace.line[lineIndex].valid == 1)){
               return 1; // line hit
        }else{
            return 0; // line miss
        }
    }

    int traceCache::selectBuildLineIndex(int lowerSearchBound, int upperSearchBound){
        // first, see if there are any lines which are invalid
        for(int i = lowerSearchBound; i < upperSearchBound; i++){
            if(trace.line[i].valid == 0){ // found an invalid line
                return i;
            }
        }
        // if there are no invalid cache lines, randomly pick a line to evict
        return lowerSearchBound + (rand() % upperSearchBound);
    }

    void traceCache::completeTrace(){
        // copy all buildLine values to the correct line in the tc
        trace.line[trace.buildLineIndex].tagAddr = trace.buildLine->tagAddr;
        trace.line[trace.buildLineIndex].branchFlags = trace.buildLine->branchFlags;
        trace.line[trace.buildLineIndex].insnCount = trace.buildLine->insnCount;
        trace.line[trace.buildLineIndex].BBCount = trace.buildLine->BBCount;
        trace.line[trace.buildLineIndex].valid = 1;

        // clear out all buildLine values
        trace.buildLine->tagAddr = 9;
        trace.buildLine->branchFlags = 0;
        trace.buildLine->insnCount = 0;
        trace.buildLine->BBCount = 0;

        // no longer building a trace
        trace.buildingTrace = 0;
        trace.buildLineIndex = 0;
    }



