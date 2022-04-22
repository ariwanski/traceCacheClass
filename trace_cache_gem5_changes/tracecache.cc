#include "tracecache.hh"
using namespace gem5;

traceCache::traceCache(const traceCacheParams &params, int numSets, int assoc, int numInsns, int numBBs) :
SimObject(params)
{
    // create an array of lines
    line = new tcLine[assoc*numSets];
    // set tc parameter
    numSets = numSets;
    assoc = assoc;
    // set tc fields for describing trace cache
    size = assoc*numSets;
    maxNumInsns = numInsns;
    maxNumBBs = numBBs;
    // set tc fields for building a trace
    buildingTrace = 0;
    buildLineIndex = 0;
    buildLine = new tcLine();
    // set tc fields for stats tracking
    globalHitCount = 0;
    globalMissCount = 0;
}

traceCache::traceCache(const traceCacheParams &params) :
SimObject(params)
{
    // create an array of lines
    line = new tcLine[1*64];
    // set tc parameter
    numSets = 64;
    assoc = 1;
    // set tc fields for describing trace cache
    size = 1*64;
    maxNumInsns = 16;
    maxNumBBs = 1;
    // set tc fields for building a trace
    buildingTrace = 0;
    buildLineIndex = 0;
    buildLine = new tcLine();
    // set tc fields for stats tracking
    globalHitCount = 0;
    globalMissCount = 0;
}

// to be ran every instruction fetch
void traceCache::tcInsnFetch(int fetchAddr, int isCondBranch, int branchPred){
int traceHit = 0;
    // conditional branch instructions mark the beginning of a new trace
    if(isCondBranch){
        // complete the trace currently being built
        if(buildingTrace){
            completeTrace();
        }
        // look for the current address and prediction in the trace cache
        traceHit = searchTraceCache(fetchAddr, branchPred);

    }else{
        if(buildingTrace){
            // if there is a trace currently being built, add this instruction
            buildTrace(fetchAddr, branchPred);
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
    int numIndexBits = log2(numSets);
    int hit = 0;
    mask = (1 << numIndexBits) - 1;
    index = fetchAddr & mask;

    // find bounds of tc lines to access
    lowerSearchBound = index * assoc;
    upperSearchBound = lowerSearchBound + assoc;

    // search all lines in appropriate set for hit
    for(int i = lowerSearchBound; i < upperSearchBound; i++ ){
        hit = searchTraceLine(fetchAddr, branchPred, i);
        if(hit){ // if there is a hit, exit early
            // log the hit statistics
            logHitStats(fetchAddr, branchPred, i);
            return 1;
        }
    }

    // if there is no hit - we have a trace cache miss
    logMissStats(fetchAddr, branchPred);

    // on a miss, we begin building a new trace
    // first, we figure out in which line the new trace should reside
    buildLineIndex = selectBuildLineIndex(lowerSearchBound, upperSearchBound);
    // then, we begin building the trace
    buildTrace(fetchAddr, branchPred);
    return 0;
}


void traceCache::logHitStats(int fetchAddr, int branchPred, int i){
    globalHitCount++;
}

void traceCache::logMissStats(int fetchAddr, int branchPred){
    globalMissCount++;
}


void traceCache::buildTrace(int fetchAddr, int branchPred){
    // if we are not currently building a trace, then this is the first
    // instruction of the trace
    if(buildingTrace == 0){
        // set up trace
        buildLine->tagAddr = fetchAddr; // save the PC
        buildLine->branchFlags = branchPred;
        buildLine->insnCount = 1;
        buildLine->BBCount = 1;
        // tell system we are now building a trace
        buildingTrace = 1;
    }
    // if we are currently building a trace, then we need to add this insn to the trace
    else{
        // increment the instruction count of the line
        buildLine->insnCount++;
    }

    // check whether the trace has reached its max capacity
    if(buildLine->insnCount >= maxNumInsns){
        completeTrace();
    }
}

int traceCache::searchTraceLine(int fetchAddr, int branchPred, int lineIndex){
    // check for hit conditions
    if((fetchAddr == line[lineIndex].tagAddr)&
      (branchPred == line[lineIndex].branchFlags)&
      (line[lineIndex].valid == 1)){
        return 1; // line hit
    }else{
        return 0; // line miss
    }
}


int traceCache::selectBuildLineIndex(int lowerSearchBound, int upperSearchBound){
    // first, see if there are any lines which are invalid
    for(int i = lowerSearchBound; i < upperSearchBound; i++){
        if(line[i].valid == 0){ // found an invalid line
            return i;
        }
    }
    // if there are no invalid cache lines, randomly pick a line to evict
    return lowerSearchBound + (rand() % upperSearchBound);
}

void traceCache::completeTrace(){
    // copy all buildLine values to the correct line in the tc
    line[buildLineIndex].tagAddr = buildLine->tagAddr;
    line[buildLineIndex].branchFlags = buildLine->branchFlags;
    line[buildLineIndex].insnCount = buildLine->insnCount;
    line[buildLineIndex].BBCount = buildLine->BBCount;
    line[buildLineIndex].valid = 1;

    // clear out all buildLine values
    buildLine->tagAddr = 9;
    buildLine->branchFlags = 0;
    buildLine->insnCount = 0;
    buildLine->BBCount = 0;

    // no longer building a trace
    buildingTrace = 0;
    buildLineIndex = 0;
}
