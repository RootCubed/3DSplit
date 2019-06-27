#include <citro2d.h>
#include "sls.h"

#define MAX_SPLITS 512
#define MAX_CHARS 1024

#define MAX_SPLITS_ON_PAGE 14

#define TEXT_DEFAULT C2D_AtBaseline | C2D_WithColor

#define WHITE C2D_Color32(0xFA, 0xFA, 0xFA, 255)
#define BLACK C2D_Color32(0x00, 0x00, 0x00, 255)

struct SplitLiveTimer;

typedef struct SplitLiveTimer {
    int startedRuns;
    int state;
    int scroll;
    u32 timerColor;
    u64 startTime;
    u64 endTime;
    char gameName[256];
    int numSplits;
    int currSplit;
    char splitNames[MAX_CHARS][MAX_SPLITS];
    u64 goldSegments[MAX_SPLITS];
    u64 PBSplits[MAX_SPLITS];
    u64 currSplits[MAX_SPLITS];
} Timer;


void SL_Timer_CreateTimer(Timer *t);

void SL_Timer_Draw(Timer *t, C2D_TextBuf textBuf);

void SL_Timer_LoadFromSplitFile(Timer *t, SLS *s);

void SL_Timer_SaveToSplitFile(Timer *t, SLS *s);

void SL_Timer_StartSplit(Timer *t);

void SL_Timer_Reset(Timer *t);

void SL_Timer_Undo(Timer *t);

void SL_Timer_Skip(Timer *t);

char* SL_Timer_GetMainTimerText(Timer *t);

char* SL_Timer_GetDeltaText(Timer *t, int segment);

char* SL_Timer_GetCurrSplitText(Timer *t, int segment);

char* SL_Timer_GetSOBText(Timer *t);

void u64ToDelta(char *str, u64 time, bool positive);

void u64ToTime(char *str, u64 time);
