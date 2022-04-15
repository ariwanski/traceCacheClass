#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;

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
    int    maxNumBBs = 1; // max number of basic blocks in one cache line

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
    }

    // to be ran every instruction fetch
    void tcInsnFetch(int fetchAddr, int isCondBranch, int branchPred){
        int traceHit = 0;
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
    int searchTraceCache(int fetchAddr, int branchPred){
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

    void logHitStats(int fetchAddr, int branchPred, int i){
        this->globalHitCount++;
    }

    void logMissStats(int fetchAddr, int branchPred){
        this->globalMissCount++;
    }

    void buildTrace(int fetchAddr, int branchPred){
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

    int searchTraceLine(int fetchAddr, int branchPred, int lineIndex){
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

};

// FOR TESTING PURPOSES

// dummy instruction class for testing
class insn{
    public:
        int addr;
        int isCondBranch;
        int branchPred;

    insn(){
        this->addr = 0;
        this->isCondBranch = 0;
        this->branchPred = 0;
    }
};
int nxtPC = 0;

void createInsnStream(insn* insnStream, int size){
    for(int i = 0; i < size; i++){
        insnStream[i].addr=nxtPC;
        if(i % 5 == 0){
            insnStream[i].isCondBranch = 1;
            insnStream[i].branchPred = 1;
            nxtPC = nxtPC + 2048;
        }else{
            nxtPC = nxtPC + 1;
        }
    }
}

void printInsnStream(insn* insnStream, int size){
    for(int i = 0; i < size; i++){
        printf("Insn#: %d, Addr: %d, CondBranch: %d, Pred: %d\n", i, insnStream[i].addr, insnStream[i].isCondBranch, insnStream[i].branchPred);
    }
}

void simulateInsnStream(insn* insnStream, int size, traceCache *tc){
    for(int i = 0; i < size; i++){
        tc->tcInsnFetch(insnStream[i].addr,insnStream[i].isCondBranch, insnStream[i].branchPred);
    }
}

// just for basic sanity checks
int main()
{
    // initialize trace cache
    int numSets = 64;
    int assoc = 1;
    int numInsns = 16;
    int numBBs = 1;
    traceCache *tc = new traceCache(numSets,assoc, numInsns, numBBs);

    // create instruction stream
    int size = 30;
    insn* insnStream  = new insn[size];
    createInsnStream(insnStream, size);
    printInsnStream(insnStream, size);

    // simulate insn stream
    simulateInsnStream(insnStream, size, tc);
    simulateInsnStream(insnStream, size, tc);

    printf("trace cache size: %d\n", tc->size);

    for (int i = 0; i < tc->size; i++){
        printf("tc at %d: %d\n", i, tc->line[i].insnCount);
    }

    printf("trace miss count: %d\n", tc->globalMissCount);
    printf("trace hit count: %d\n", tc->globalHitCount);

    return 0;
}
