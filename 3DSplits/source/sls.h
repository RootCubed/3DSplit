struct SLS;

typedef struct SLS {
    char gameName[1024];
    char category[1024];
    int startedRuns;
    int numSplits;
    char splitNames[512][1024];
    unsigned long long PBSplits[512];
    unsigned long long goldSegments[512];
} SLS;
