#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;

// represents one line in the trace cache
class tcLine
{
  public:
    // fields for accessing the tc line
    uint64_t   tagAddr;
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
class traceCache
{
  public:
    // parameters
    int numSets; // number of sets in the cache
    int assoc; // associativity of the cache

    // fields describing the trace cache
    tcLine* line;
    int    size; // size of the cache in terms of lines
    int    maxNumInsns; // max number of insns in one cache line
    int    fetchInsnCount; // counter to count all the fetched instructions
    int    maxNumBBs = 1; // max number of basic blocks in one cache line
//    float MissRate;
    // fields for building a trace in the trace cache
    int buildingTrace; // 1 = currently building a trace, 0 = otherwise
    int buildLineIndex; // hold line # is trace cache that trace
                        // is being built for
    tcLine* buildLine; // used to hold stats on tc line currently being built

    // Stats tracking
    int globalHitCount;
    int globalMissCount;

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
        this->fetchInsnCount = 0;
//        this->MissRate = 0;
    }

    // to be ran every instruction fetch
    void tcInsnFetch(uint64_t fetchAddr, int isCondBranch, int branchPred){
        int traceHit = 0;
	this->fetchInsnCount++;
        // conditional branch instructions mark the beginning of a new trace
        if(isCondBranch){
            // complete the trace currently being built
            if(this->buildingTrace){
                this->completeTrace();
            }

            // look for the current address and prediction in the trace cache
            traceHit = this->searchTraceCache(fetchAddr, branchPred);

        }else{
            if(this->buildingTrace){
                // if there is a trace currently being built, add this instruction
                this->buildTrace(fetchAddr, branchPred);
            } // otherwise, do nothing
        }
    }

    // returns 1 if hit, 0 if miss
    int searchTraceCache(uint64_t fetchAddr, int branchPred){
        // get the index bits from the fetch address
        unsigned int mask = 0;
        int index = 0;
        int lowerSearchBound = 0;
        int upperSearchBound = 0;
        int numIndexBits = log2(this->numSets);
        int hit = 0;
        mask = (1 << numIndexBits) - 1;
        index = fetchAddr & mask;

        // find bounds of tc lines to access
        lowerSearchBound = index * this->assoc;
        upperSearchBound = lowerSearchBound + this->assoc;

        // search all lines in appropriate set for hit
        for(int i = lowerSearchBound; i < upperSearchBound; i++ ){
            hit = searchTraceLine(fetchAddr, branchPred, i);
            if(hit){ // if there is a hit, exit early
                // log the hit statistics
                this->logHitStats(fetchAddr, branchPred, i);
                return 1;
            }
        }

        // if there is no hit - we have a trace cache miss
        this->logMissStats(fetchAddr, branchPred);

        // on a miss, we begin building a new trace
        // first, we figure out in which line the new trace should reside
        this->buildLineIndex = this->selectBuildLineIndex(lowerSearchBound, upperSearchBound);
        // then, we begin building the trace
        this->buildTrace(fetchAddr, branchPred);
        return 0;
    }

    void logHitStats(uint64_t fetchAddr, int branchPred, int i){
        this->globalHitCount++;
    }

    void logMissStats(uint64_t fetchAddr, int branchPred){
        this->globalMissCount++;
    }

    void buildTrace(uint64_t fetchAddr, int branchPred){
        // if we are not currently building a trace, then this is the first
        // instruction of the trace
        if(this->buildingTrace == 0){
            // set up trace
            this->buildLine->tagAddr = fetchAddr; // save the PC
            this->buildLine->branchFlags = branchPred;
            this->buildLine->insnCount = 1;
            this->buildLine->BBCount = 1;
            // tell system we are now building a trace
            this->buildingTrace = 1;
        }
        // if we are currently building a trace, then we need to add this insn to the trace
        else{
            // increment the instruction count of the line
            this->buildLine->insnCount++;
        }

        // check whether the trace has reached its max capacity
        if(this->buildLine->insnCount >= this->maxNumInsns){
            completeTrace();
        }
    }

    int searchTraceLine(uint64_t fetchAddr, int branchPred, int lineIndex){
        // check for hit conditions
        if((fetchAddr == this->line[lineIndex].tagAddr)&
           (branchPred == this->line[lineIndex].branchFlags)&
           (this->line[lineIndex].valid == 1)){
               return 1; // line hit
        }else{
            return 0; // line miss
        }
    }

    int selectBuildLineIndex(int lowerSearchBound, int upperSearchBound){
        // first, see if there are any lines which are invalid
        for(int i = lowerSearchBound; i < upperSearchBound; i++){
            if(this->line[i].valid == 0){ // found an invalid line
                return i;
            }
        }
        // if there are no invalid cache lines, randomly pick a line to evict
        return lowerSearchBound + (rand() % upperSearchBound);
    }

    void completeTrace(){
        // copy all buildLine values to the correct line in the tc
        this->line[this->buildLineIndex].tagAddr = this->buildLine->tagAddr;
        this->line[this->buildLineIndex].branchFlags = this->buildLine->branchFlags;
        this->line[this->buildLineIndex].insnCount = this->buildLine->insnCount;
        this->line[this->buildLineIndex].BBCount = this->buildLine->BBCount;
        this->line[this->buildLineIndex].valid = 1;

        // clear out all buildLine values
        this->buildLine->tagAddr = 9;
        this->buildLine->branchFlags = 0;
        this->buildLine->insnCount = 0;
        this->buildLine->BBCount = 0;

        // no longer building a trace
        this->buildingTrace = 0;
        this->buildLineIndex = 0;
    }
    
    void printCacheState(){
     if(this->fetchInsnCount>=100000000){
	printf("No. of instructions fetched:%d\n",this->fetchInsnCount);
      	printf("******TRACE CACHE STATE******\n");
    	printf("Current Miss Count: %d\n", this->globalMissCount);
    	printf("Current Hit Count: %d\n\n", this->globalHitCount);
//	this->MissRate = (this->globalMissCount/(this->globalMissCount+this->globalHitCount))*100;
//	printf("Current Miss Rate: %f",MissRate);
    }
    }
    
    void printCacheParameters(){
      if(this->fetchInsnCount>=100000000){
      printf("******TRACE CACHE PARAMS******\n");
      printf("Size: %d\n", this->size);
      printf("Number of Sets: %d\n", this->numSets);
      printf("Associativity: %d\n", this->assoc);
      printf("Max # of Insns Per Line: %d\n", this->maxNumInsns);
    }
    }
    void testCache(){
        printf("THIS IS FROM TRACE CACHE!!!\n");
    }

};


