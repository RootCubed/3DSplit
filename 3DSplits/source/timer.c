#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"

#define STATE_RESET 0
#define STATE_RUNNING 1
#define STATE_END 2
#define STATE_PAUSED 3

#define GREEN_BETTER C2D_Color32(0x00, 0x80, 0x30, 255)
#define GREEN_WORSE C2D_Color32(0x51, 0xCF, 0x74, 255)
#define RED_BETTER C2D_Color32(0xAD, 0x68, 0x5A, 255)
#define RED_WORSE C2D_Color32(0xD7, 0x23, 0x0D, 255)
#define GOLD C2D_Color32(0xD6, 0xAC, 0x1C, 255)

void SL_Timer_CreateTimer(Timer *t) {
    t->state = STATE_RESET;
    t->currSplit = -1;
    t->numSplits = 0;
    t->scroll = 0;
}

void SL_Timer_Draw(Timer *t, C2D_TextBuf textBuf) {
    // draw big timer
    C2D_Text timerText;
    char *timerTextStr = SL_Timer_GetMainTimerText(t);
    C2D_TextParse(&timerText, textBuf, timerTextStr);
    free(timerTextStr);
    C2D_TextOptimize(&timerText);
    C2D_DrawText(&timerText, TEXT_DEFAULT, 400 - timerText.width, 235.0f, 0.0f, 1.0f, 1.0f, WHITE);

    // draw splits
    int y = 15;
    for (int i = t->scroll; i < t->numSplits; i++) {
        if (i == t->currSplit + 1 && (t->state == STATE_RUNNING || t->state == STATE_PAUSED)) {
            C2D_DrawRectSolid(0, y - 12, 0, 400, 14, C2D_Color32(0x1A, 0x4B, 0x9B, 255));
        }
        // split names
        C2D_TextBufClear(textBuf);
        C2D_Text splitText;
        C2D_TextParse(&splitText, textBuf, t->splitNames[i]);
        C2D_TextOptimize(&splitText);
        C2D_DrawText(&splitText, TEXT_DEFAULT, 0, y, 0.0f, 0.5f, 0.5f, WHITE);

        // deltas
        C2D_TextBufClear(textBuf);
        C2D_Text deltaText;
        char *deltaTextStr = SL_Timer_GetDeltaText(t, i);
        C2D_TextParse(&deltaText, textBuf, deltaTextStr);
        free(deltaTextStr);
        C2D_TextOptimize(&deltaText);
        // find out color
        u32 color = WHITE;
        long long currDelta = t->currSplits[i] - t->PBSplits[i];
        long long lastDelta = t->currSplits[i - 1] - t->PBSplits[i - 1];
        long long currSegment = t->currSplits[i] - t->currSplits[i - 1];
        if (i < t->currSplit + 1) {
            bool gotWorse = (currDelta > lastDelta);
            if (t->currSplits[i] >= t->PBSplits[i]) {
                if (gotWorse)
                    color = RED_WORSE;
                else
                    color = RED_BETTER;
            } else {
                if (gotWorse)
                    color = GREEN_WORSE;
                else
                    color = GREEN_BETTER;
            }
            if (currSegment < t->goldSegments[i]) {
                color = GOLD;
            }
        }

        C2D_DrawText(&deltaText, TEXT_DEFAULT, 400 / 2 - deltaText.width / 4, y, 0.0f, 0.5f, 0.5f, color);

        // time
        C2D_TextBufClear(textBuf);
        C2D_Text timeText;
        char *timeTextStr = SL_Timer_GetCurrSplitText(t, i);
        C2D_TextParse(&timeText, textBuf, timeTextStr);
        free(timeTextStr);
        C2D_TextOptimize(&timeText);
        C2D_DrawText(&timeText, TEXT_DEFAULT, 400 - timeText.width / 2, y, 0.0f, 0.5f, 0.5f, WHITE);

        // move y down
        y += 14;
        if (i < t->numSplits - 2 && (y - 15) / 14 == MAX_SPLITS_ON_PAGE) {
            // draw seperator
            C2D_DrawRectSolid(0, y - 12, 0.0f, 400, 1, WHITE);
            i = t->numSplits - 2;
        }
    }

}

void SL_Timer_LoadFromSplitFile(Timer *t, SLS *s) {
    t->numSplits = s->numSplits;
    for(int i = 0; i < t->numSplits; i++) {
        strcpy(t->splitNames[i], s->splitNames[i]);
        t->PBSplits[i] = s->PBSplits[i];
        t->goldSegments[i] = s->goldSegments[i];
    }
}

void SL_Timer_StartSplit(Timer *t) {
    switch (t->state) {
        case STATE_RESET:
            t->state = STATE_RUNNING;
            t->startTime = osGetTime();
            t->scroll = 0;
            break;
        case STATE_RUNNING:
            t->currSplit++;
            if (t->scroll < t->numSplits - MAX_SPLITS_ON_PAGE - 1) { // can still scroll
                if (t->currSplit - t->scroll > MAX_SPLITS_ON_PAGE * 3 / 4) t->scroll++; // scroll after 3/4 of screen
            }
            t->currSplits[t->currSplit] = osGetTime() - t->startTime;
            if (t->currSplit == t->numSplits - 1) {
                t->state = STATE_END;
                t->endTime = osGetTime();
            }
        case STATE_END:
            break;
        case STATE_PAUSED:
            t->state = STATE_RUNNING;
            break;
    }
}

void SL_Timer_Reset(Timer *t) {
    if (t->currSplit == t->numSplits - 1 && (t->endTime - t->startTime < t->PBSplits[t->numSplits - 1] || t->PBSplits[t->numSplits - 1] == 0)) {
        for (int i = 0; i < t->numSplits; i++) {
            t->PBSplits[i] = t->currSplits[i];
        }
    }
    t->currSplit = -1;
    t->state = STATE_RESET;
}

char* SL_Timer_GetMainTimerText(Timer *t) {
    u64 currTime = osGetTime();
    char *res = malloc(sizeof(char) * 256);
    u64 diff = 0;

    switch (t->state) {
        case STATE_RESET:
            diff = 0;
            break;
        case STATE_RUNNING:
            diff = currTime - t->startTime;
            break;
        case STATE_END:
            diff = t->endTime - t->startTime;
            break;
    }

    u64ToTime(res, diff);
    return res;
}

char* SL_Timer_GetDeltaText(Timer *t, int segment) {
    char *res = malloc(sizeof(char) * 256);

    if (segment == t->currSplit + 1) { // if current segment has passed gold, show it
        if (t->state == STATE_RESET || osGetTime() - t->startTime - t->currSplits[segment - 1] < t->goldSegments[segment] || t->goldSegments[segment] == 0) {
            strcpy(res, "");
            return res;
        }
        u64 currTime = osGetTime() - t->startTime;
        u64 segSplit;
        if (currTime <= t->PBSplits[segment]) {
            segSplit = t->PBSplits[segment] - currTime;
            u64ToDelta(res, segSplit, false);
        } else {
            segSplit = currTime - t->PBSplits[segment];
            u64ToDelta(res, segSplit, true);
        }
    } else if (segment <= t->currSplit + 1) {
        if (t->PBSplits[segment] == 0) {
            strcpy(res, "- ");
        } else {
            u64 segSplit;
            if (t->currSplits[segment] <= t->PBSplits[segment]) {
                segSplit = t->PBSplits[segment] - t->currSplits[segment];
                u64ToDelta(res, segSplit, false);
            } else {
                segSplit = t->currSplits[segment] - t->PBSplits[segment];
                u64ToDelta(res, segSplit, true);
            }
        }
    } else {
        strcpy(res, "");
    }

    return res;
}

char* SL_Timer_GetCurrSplitText(Timer *t, int segment) {
    char *res = malloc(sizeof(char) * 256);

    if (segment <= t->currSplit) {
        u64 segSplit = t->currSplits[segment];
        u64ToTime(res, segSplit);
    } else {
        if (t->PBSplits[segment] > 0) {
            u64 currPBSplit = t->PBSplits[segment];
            u64ToTime(res, currPBSplit);
        } else {
            strcpy(res, "- ");
        }
    }
    return res;
}

void u64ToDelta(char *str, u64 time, bool positive) {
    time = time / 10; // display tenths
    if (time / 100 / 60 / 60 > 0) {
        sprintf(str, (positive) ? "+%lld:%02lld:%02lld.%02lld" : "-%lld:%02lld:%02lld.%02lld", (time / 100 / 60 / 60), (time / 100 / 60 % 60), (time / 100 % 60), (time % 100));
        return;
    }
    if (time / 100 / 60 > 0) {
        sprintf(str, (positive) ? "+%lld:%02lld.%02lld" : "-%lld:%02lld.%02lld", (time / 100 / 60 % 60), (time / 100 % 60), (time % 100));
        return;
    }
    sprintf(str, (positive) ? "+%lld.%02lld" : "-%lld.%02lld", (time / 100 % 60), (time % 100));
}

void u64ToTime(char *str, u64 time) {
    time = time / 10; // display tenths
    if (time / 100 / 60 / 60 > 0) {
        sprintf(str, "%lld:%02lld:%02lld.%02lld", (time / 100 / 60 / 60), (time / 100 / 60 % 60), (time / 100 % 60), (time % 100));
        return;
    }
    sprintf(str, "%lld:%02lld.%02lld", (time / 100 / 60 % 60), (time / 100 % 60), (time % 100));
}
