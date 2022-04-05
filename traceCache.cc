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

    // fields for building a trace in this line
    int insnCount; // current count of insns in trace being built
    int BBCount; // current count of BB's in trace being built
    int branchInsnCount; // count of insns since last branch insn was seen
    int buildingTrace; // 1 = currently building a trace, 0 = otherwise

    // Constructor
    tcLine() {
      // set fields for accessing a tc line
      this->tag = NULL;
      this->valid = 0;
      this->branchFlags = 0;
      // set fields for building a trace
      this->insnCount = 0;
      this->BBCount = 0;
      this->branchInsnCount = 0;
      this->buildingTrace = 0;
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
    int    numInsns; // max number of insns in one cache line
    int    numBBs; // max number of basic blocks in one cache line

    // fields for building a trace in the trace cache
    int buildingTrace; // 1 = currently building a trace, 0 = otherwise
    int buildLineIndex; // hold line # is trace cache that trace
                        // is being built for


    // Constructor
    traceCache(int numSets, int assoc, int numInsns, int numBBs) {
        // create an array of lines
        line = new tcLine[assoc*numSets];
        // set tc parameter
        this->numSets = numSets;
        this->assoc = assoc;
        // set tc fields for describing trace cache
        this->size = assoc*numSets;
        this->numInsns = numInsns;
        this->numBBs = numBBs;
        // set tc fields for building a trace
        this->buildingTrace = 0;
        this->buildLineIndex = 0;
    }

    // to be ran every instruction fetch
    void tcInsnFetch(Addr fetchAddr, int isBranch, int branchPred){
        // get address from fetchAddr object
        int addr = fetchAddr.addrValue;

        if (this->buildingTrace){ // if a trace is currently being built
            // add this instruction to that trace
            this->addToTrace(addr, isBranch);
        }else{ // otherwise...
            // see if this instruction is in the trace cache
            this->accessCache(addr, branchPred);
        }
    }

    // adds the instruction to the trace currently being built
    void addToTrace(int addr, int isBranch){
        // add the instruction
        this->line[this->buildLineIndex].insnCount =
                this->line[this->buildLineIndex].insnCount +1;

        // handle case when current instruction is a branch
        if (isBranch){ // if the current insn is a branch insn

        }

        // check whether trace build is complete

    }

    // check the cache for the current addr
    int accessCache(int addr, int branchPred){
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
