#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <3ds.h>
#include <citro2d.h>

#include "timer.h"

#define START_SPLIT_ICON 0
#define RESET_ICON 1

bool isSelecting = true;
bool foundSplits = false;
char splitFiles[4096][4096];
int numFoundSplits = 0;
int selectedFile = 0;

C2D_SpriteSheet spriteSheet;
C2D_TextBuf textBuf;

C2D_Sprite startSplitButton;
C2D_Sprite resetButton;

Timer mainTimer;

u8 *loadedSplits;
SLS splitFile;

static void sceneInit()
{
    SL_Timer_CreateTimer(&mainTimer);

    textBuf = C2D_TextBufNew(256);

    //start/split
    C2D_SpriteFromSheet(&startSplitButton, spriteSheet, START_SPLIT_ICON);
    C2D_SpriteSetCenter(&startSplitButton, 0, 0);
    C2D_SpriteSetPos(&startSplitButton, 60, 20);

    //reset
    C2D_SpriteFromSheet(&resetButton, spriteSheet, RESET_ICON);
    C2D_SpriteSetCenter(&resetButton, 0, 0);
    C2D_SpriteSetPos(&resetButton, 240, 0);

    // look for splits
    DIR *d;
    struct dirent *dir;
    d = opendir("/splits");
    if (d) {
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".spl") != NULL) {
                strcpy(splitFiles[i++], dir->d_name);
                foundSplits = true;
            }
        }
        numFoundSplits = i;
        closedir(d);
    }
}

void drawError(char *text, C2D_TextBuf textBuf) {
    C2D_TextBufClear(textBuf);
    C2D_Text errorMsg;
    C2D_TextParse(&errorMsg, textBuf, text);
    C2D_TextOptimize(&errorMsg);
    C2D_DrawText(&errorMsg, TEXT_DEFAULT, 160 - errorMsg.width / 4, 50.0f, 0.0f, 0.5f, 0.5f, WHITE);
    C2D_DrawRectSolid(0, 200, 0.0f, 320, 40, WHITE);

    C2D_TextBufClear(textBuf);
    C2D_Text OKBtn;
    C2D_TextParse(&OKBtn, textBuf, "OK");
    C2D_TextOptimize(&OKBtn);
    C2D_DrawText(&OKBtn, TEXT_DEFAULT, 160 - OKBtn.width / 2, 230.0f, 0.0f, 1.0f, 1.0f, BLACK);
}

void drawRectOutline(int x1, int y1, int w, int h, int thickness) {
    C2D_DrawRectSolid(x1, y1, 0, w, thickness, WHITE);
    C2D_DrawRectSolid(x1, y1, 0, thickness, h, WHITE);
    C2D_DrawRectSolid(x1 + w, y1, 0, thickness, h + thickness, WHITE);
    C2D_DrawRectSolid(x1, y1 + h, 0, w + thickness, thickness, WHITE);
}

void drawFiles(int numFiles, char text[4096][4096], C2D_TextBuf textBuf) {
    for (int i = 0; i < numFiles; i++) {
        C2D_TextBufClear(textBuf);
        C2D_Text fileText;
        C2D_TextParse(&fileText, textBuf, text[i]);
        C2D_TextOptimize(&fileText);
        C2D_DrawText(&fileText, TEXT_DEFAULT, 10, i * 30 + 15, 0.0f, 0.5f, 0.5f, WHITE);

        if (i == selectedFile) {
            drawRectOutline(2, i * 30, fileText.width / 2 + 12, 20, 2);
        }
    }
}

int getBufferLine(int start, char (*result)[1024], u8 **buffer) {
    int i = 0;
    char currChar = (char) (*buffer)[start];
    while (currChar != '\n') {
        (*result)[i++] = currChar;
        currChar = (char) (*buffer)[++start];
    }
    (*result)[i] = '\0';
    printf("line at %d: %s\n", start - i, *result);
    return ++start;
}

void parseSplits() {
    int pos = 0;
    pos = getBufferLine(pos, &splitFile.gameName, &loadedSplits);
    pos = getBufferLine(pos, &splitFile.category, &loadedSplits);
    char temp[1024];
    pos = getBufferLine(pos, &temp, &loadedSplits);
    splitFile.startedRuns = atoi(temp);
    pos = getBufferLine(pos, &temp, &loadedSplits);
    splitFile.numSplits = atoi(temp);
    for (int i = 0; i < splitFile.numSplits; i++) {
        pos = getBufferLine(pos, &(splitFile.splitNames[i]), &loadedSplits);
    }
    for (int i = 0; i < splitFile.numSplits; i++) {
        pos = getBufferLine(pos, &temp, &loadedSplits);
        splitFile.PBSplits[i] = atoi(temp);
    }
    for (int i = 0; i < splitFile.numSplits; i++) {
        pos = getBufferLine(pos, &temp, &loadedSplits);
        splitFile.goldSegments[i] = atoi(temp);
    }
    SL_Timer_LoadFromSplitFile(&mainTimer, &splitFile);
    // free file
    free(loadedSplits);
}

void loadSplits(const char *path) {
    char realPath[256];
    strcpy(realPath, "/splits/");
    strcat(realPath, path);
    FILE *file = fopen(realPath, "rb");
	if (file != NULL) {
    	// seek to end of file
    	fseek(file, 0, SEEK_END);
    	// file pointer tells us the size
    	off_t size = ftell(file);

    	// seek back to start
    	fseek(file,0,SEEK_SET);

    	//allocate a buffer
    	loadedSplits = malloc(size);
        fread(loadedSplits, 1, size, file);

        fclose(file);

        parseSplits();
    }
}

void saveSplits(const char *path) {
    SL_Timer_Reset(&mainTimer); // save current run
    SL_Timer_SaveToSplitFile(&mainTimer, &splitFile);
    char realPath[256];
    strcpy(realPath, "/splits/");
    strcat(realPath, path);
    FILE *file = fopen(realPath, "w");
	if (file != NULL) {
    	fprintf(file, "%s\n", splitFile.gameName);
        fprintf(file, "%s\n", splitFile.category);
        fprintf(file, "%d\n", splitFile.startedRuns);
        fprintf(file, "%d\n", splitFile.numSplits);
        for (int i = 0; i < splitFile.numSplits; i++) {
            fprintf(file, "%s\n", splitFile.splitNames[i]);
        }
        for (int i = 0; i < splitFile.numSplits; i++) {
            fprintf(file, "%lld\n", splitFile.PBSplits[i]);
        }
        for (int i = 0; i < splitFile.numSplits; i++) {
            fprintf(file, "%lld\n", splitFile.goldSegments[i]);
        }
        fclose(file);
    }
}

static void sceneRender()
{
    C2D_TextBufClear(textBuf);
    SL_Timer_Draw(&mainTimer, textBuf);
}

static void sceneExit()
{
    // Delete the text buffers
    C2D_TextBufDelete(textBuf);
}

int main(int argc, char* argv[])
{
    // Initialize the libs
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create screen
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // load sprites
    spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

    // Initialize the scene
    sceneInit();

    // Main loop
    while (aptMainLoop())
    {
        hidScanInput();

        // Respond to user input
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            saveSplits(splitFiles[selectedFile]);
            break;
        }

        if (kDown & KEY_A) {
            if (isSelecting && foundSplits) {
                loadSplits(splitFiles[selectedFile]);
                isSelecting = false;
            } else if (!isSelecting) {
                SL_Timer_StartSplit(&mainTimer);
            }
        }

        if (kDown & (KEY_L | KEY_R)) {
            SL_Timer_Reset(&mainTimer);
        }

        if (kDown & KEY_B) {
            SL_Timer_Undo(&mainTimer);
        }

        if (kDown & KEY_X) {
            SL_Timer_Skip(&mainTimer);
        }


        if (kDown & KEY_DUP) {
            if (isSelecting && foundSplits) {
                selectedFile--;
                if (selectedFile < 0) {
                    selectedFile = 0;
                }
            } else if (!isSelecting) {
                if (mainTimer.scroll > 0) {
                    mainTimer.scroll--;
                }
            }
        }

        if (kDown & KEY_DDOWN) {
            if (isSelecting && foundSplits) {
                selectedFile++;
                if (selectedFile >= numFoundSplits) {
                    selectedFile = numFoundSplits - 1;
                }
            } else if (!isSelecting) {
                if (mainTimer.scroll < mainTimer.numSplits - MAX_SPLITS_ON_PAGE - 1) {
                    mainTimer.scroll++;
                }
            }
        }

        if (kDown & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);

            if (!isSelecting) {
                // start/select
                int adjX = touch.px - 200;
                int adjY = touch.py - 200;
                if (sqrt(adjX * adjX + adjY * adjY) < 100) {
                    SL_Timer_StartSplit(&mainTimer);
                }

                // reset
                if (touch.px > 240 && touch.py < 31) {
                    SL_Timer_Reset(&mainTimer);
                }
            } else {
                if (!foundSplits) {
                    if (touch.py > 200) {
                        isSelecting = false;
                    }
                } else {

                }
            }
        }

        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(bot, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(bot);
        if (isSelecting) {
            if (!foundSplits) {
                drawError("No splits found.\nMake sure you placed them in /splits!\n3DSplits has created a temporary file\nwhich will not be saved after quitting!\n\nConverter for splits:\nletsplentendo-ch.github.io", textBuf);
            } else {
                drawFiles(numFoundSplits, splitFiles, textBuf);
            }
        } else {
            C2D_DrawSprite(&startSplitButton);
            C2D_DrawSprite(&resetButton);
        }
        if (!isSelecting) {
            C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
            C2D_SceneBegin(top);
            sceneRender();
        }
        C3D_FrameEnd(0);
    }

    C2D_SpriteSheetFree(spriteSheet);

    // Deinitialize the scene
    sceneExit();

    // Deinitialize the libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
