#include <iostream>

using namespace std;

// dummy class representing the Addr class in Gem5.
// putting this here to work on trace cache code outside of the gem5 system
// for faster building and debugging
class Addr
{
  public:
    int addrValue;
    Addr(){
        this->addrValue = 5;
    }
};

// represents one line in the trace cache
class tcLine
{
  public:
    // fields for accessing the tc line
    Addr* tag;
    int   valid;
    int   branchFlags;

    int insnCount; // number of instructions in this line
    int BBCount; // number of basic blocks in this line

    // Constructor
    tcLine() {
      // set fields for accessing a tc line
      this->tag = NULL;
      this->valid = 0;
      this->branchFlags = 0;
      // set fields for building a trace
      this->insnCount = 0;
      this->BBCount = 0;
    }
};

// represents the trace cache as an array of tcLine objects
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
    int    maxNumBBs; // max number of basic blocks in one cache line

    // fields for building a trace in the trace cache
    int buildingTrace; // 1 = currently building a trace, 0 = otherwise
    int buildLineIndex; // hold line # is trace cache that trace
                        // is being built for
    tcLine* buildLine; // used to hold stats on tc line currently being built


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
    }

    // to be ran every instruction retire
    void tcInsnRetire(Addr retireAddr, int isBranch, int branchResult){
        // get address from fetchAddr object
        int addr = retireAddr.addrValue;

        if (this->buildingTrace){ // if a trace is currently being built
            // add this instruction to that trace
            this->addToTrace(addr, isBranch, branchResult);
        }else{ // otherwise...
            // see if this instruction is in the trace cache
            this->accessCache(addr, branchResult);
        }
    }

    // adds the instruction to the trace currently being built
    void addToTrace(int addr, int isBranch, int branchResult){
        // add the instruction
        this->buildLine->insnCount++;

        // handle case when current instruction is a branch
        if (isBranch){ // if the current insn is a branch insn
            // add branch result to the build line's branch flags
            branchResult = branchResult << this->buildLine->BBCount;
            this->buildLine->branchFlags = this->buildLine->branchFlags | branchResult;
            this->buildLine->BBCount++; // basic blocks end with branches
        }

        // check whether trace build is complete
        // check if we have reached the max number of instructions
        if((this->maxNumInsns == this->buildLine->insnCount) |
           (this->maxNumBBs == this->buildLine->BBCount)){
            this->completeTrace();
        }
    }

    void completeTrace(){
        // copy all buildLine values to the correct line in the tc
        this->line[this->buildLineIndex].tag = this->buildLine->tag;
        this->line[this->buildLineIndex].branchFlags = this->buildLine->branchFlags;
        this->line[this->buildLineIndex].insnCount = this->buildLine->insnCount;
        this->line[this->buildLineIndex].BBCount = this->buildLine->BBCount;
        this->line[this->buildLineIndex].valid = 1;

        // clear out all buildLine values
        this->buildLine->tag = NULL;
        this->buildLine->branchFlags = 0;
        this->buildLine->insnCount = 0;
        this->buildLine->BBCount = 0;

        // no longer building a trace
        this->buildingTrace = 0;
    }

    // check the cache for the current addr
    int accessCache(int addr, int branchResult){
        return 0;
    }

};

// just for basic sanity checks
int main()
{
    int numSets = 16;
    int assoc = 2;
    int numInsns = 16;
    int numBBs = 3;
    traceCache *tc = new traceCache(numSets,assoc, numInsns, numBBs);

    printf("trace cache size: %d\n", tc->size);

    for (int i = 0; i < tc->size; i++){
        printf("tc at %d: %d\n", i, tc->line[i].insnCount);
    }

    return 0;
}
