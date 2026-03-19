#include "raylib.h"
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ===============================================
// 仮想解像度の定義
// ===============================================
#define VIRTUAL_WIDTH 800
#define VIRTUAL_HEIGHT 450

// ===============================================
// ゲーム状態の定義
// ===============================================
typedef enum {
    GAME_STATE_PLAYING,   // プレイ中
    GAME_STATE_CLEAR,     // クリア
    GAME_STATE_FAILED     // 失敗
} GameState;

// メインゲームの状態
typedef enum {
    MAIN_STATE_TITLE,     // タイトル画面
    MAIN_STATE_PLAYING,   // ゲームプレイ中
    MAIN_STATE_GAMEOVER   // ゲームオーバー
} MainState;

// ===============================================
// 共有リソース（全ミニゲームで使用）
// ===============================================
typedef struct {
    Music bgm;
    Music bgm2;
    Sound fxSuccess;
    Sound fxFail;
    Sound fxBoom;
    Sound fxType;
    Sound fxpush;
    Sound fxCoin;
    Sound fxEnter;
    Sound fxSpace;
    Sound fxCount;
    Texture2D heartTexture;
} GameResources;

GameResources resources;

// ===============================================
// スケーリング用の変数
// ===============================================
RenderTexture2D renderTexture;
float scale = 1.0f;
Vector2 offset = {0, 0};

// マウス座標を仮想座標に変換
Vector2 GetVirtualMousePosition() {
    Vector2 mousePos = GetMousePosition();
    Vector2 virtualPos;
    virtualPos.x = (mousePos.x - offset.x) / scale;
    virtualPos.y = (mousePos.y - offset.y) / scale;
    return virtualPos;
}

// ===============================================
// AUDIO RESET IMPLEMENTATION (START)
// ===============================================
void LoadGameResources() {
    resources.bgm = LoadMusicStream("./resources/music1.wav");
    resources.bgm2 = LoadMusicStream("./resources/music2.wav");
    resources.fxSuccess = LoadSound("./resources/good.mp3");
    resources.fxFail = LoadSound("./resources/se1.mp3");
    resources.fxBoom = LoadSound("./resources/se2.wav");
    resources.fxType = LoadSound("./resources/push.wav");
    resources.fxpush = LoadSound("./resources/push.wav");
    resources.fxCoin = LoadSound("./resources/coin.wav");
    resources.fxEnter = LoadSound("./resources/enter.wav");
    resources.fxSpace = LoadSound("./resources/space.wav");
    resources.fxCount = LoadSound("./resources/count.mp3");
    resources.heartTexture = LoadTexture("./resources/heart1.png");
    PlayMusicStream(resources.bgm);
}

/*
void UnloadGameResources() {
    UnloadMusicStream(resources.bgm);
    UnloadSound(resources.fxSuccess);
    UnloadSound(resources.fxFail);
    UnloadSound(resources.fxType);
    UnloadSound(resources.fxpush);
    UnloadSound(resources.fxCoin);
}
*/
/*
void ResetAudio() {
    UnloadGameResources();
    CloseAudioDevice();
    InitAudioDevice();
    LoadGameResources();
}
*/
// ===============================================
// AUDIO RESET IMPLEMENTATION (END)
// ===============================================

// ===============================================
// ミニゲーム共通データ
// ===============================================
typedef struct {
    int level;            // 難易度レベル
    float timeLimit;      // 制限時間
    float timeLeft;       // 残り時間
    GameState state;      // 現在の状態
    int gameData[300];    // ゲーム固有データ
} MiniGameContext;

// ===============================================
// ミニゲーム関数の型定義
// ===============================================
typedef void (*MiniGameInitFunc)(MiniGameContext* ctx);
typedef void (*MiniGameUpdateFunc)(MiniGameContext* ctx);
typedef void (*MiniGameDrawFunc)(MiniGameContext* ctx);

typedef struct {
    const char* name;
    MiniGameInitFunc init;
    MiniGameUpdateFunc update;
    MiniGameDrawFunc draw;
} MiniGame;

// ===============================================
// ミニゲーム1: タイピングゲーム
// ===============================================
const char* typeWordsLv1[] = {"CAT", "CD", "DIFF", "LS"};
const char* typeWordsLv2[] = {"HELLO", "WORLD", "RAYLIB", "GITHUB", "SOURCE", "BUTTON", "SCREEN", "WINDOW"};
const char* typeWordsLv3[] = {"PROGRAM", "VARIABLE", "INTEGER", "FLOATING", "BOOLEAN", "STRING", "FUNCTION"};
const char* typeWordsLv4[] = {"COMPUTER", "KEYBOARD", "MONITOR", "PROCESSOR", "ALGORITHM", "COMPILER", "DEBUGGER", "NETWORK", "SOFTWARE", "HARDWARE", "DATABASE", "OPERATING", "VIRTUAL", "EMULATOR", "FIREWALL", "ENCRYPTION"};
const char* typeWordsLv5[] = {"FRAMEBUFFER", "MULTITHREADING", "OPTIMIZATION", "DATASTRUCTURE", "RECURSION", "ENCAPSULATION", "INHERITANCE", "POLYMORPHISM", "ABSTRACTION", "INTERFACE", "TEMPLATE", "EXCEPTION", "NAMESPACE", "ITERATOR", "CONCURRENCY"};

const char* GetTargetWord(int level, int index) {
    if (level >= 6) return typeWordsLv5[index % 10];
    if (level >= 5) return typeWordsLv4[index % 8];
    if (level >= 4) return typeWordsLv3[index % 7];
    if (level >= 2) return typeWordsLv2[index % 8];
    return typeWordsLv1[index % 4];
}

void TypingGame_Init(MiniGameContext* ctx) {
    static int lastLevel = -1;
    static int lastWordIndex = -1;

    // レベルに応じて制限時間を変更
    if (ctx->level >= 8) ctx->timeLimit = 3.0f;
    else if (ctx->level >= 7) ctx->timeLimit = 4.0f;
    else if (ctx->level >= 6) ctx->timeLimit = 7.0f;
    else if (ctx->level >= 5) ctx->timeLimit = 5.0f;
    else if (ctx->level >= 3) ctx->timeLimit = 5.5f;
    else if (ctx->level >= 2) ctx->timeLimit = 6.0f;
    else ctx->timeLimit = 8.0f;

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    ctx->gameData[0] = 0;  // 現在の文字位置

    // 単語の選択（前回と同じ単語が出ないようにする）
    int numWords = 4;
    if (ctx->level >= 6) numWords = 10;
    else if (ctx->level == 5) numWords = 8;
    else if (ctx->level == 4) numWords = 7;
    else if (ctx->level >= 2) numWords = 8;
    else numWords = 4;

    int newIndex = rand() % numWords;
    if (ctx->level == lastLevel && newIndex == lastWordIndex) {
        newIndex = (newIndex + 1) % numWords;
    }

    lastLevel = ctx->level;
    lastWordIndex = newIndex;
    ctx->gameData[1] = newIndex;
}

void TypingGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    const char* targetWord = GetTargetWord(ctx->level, ctx->gameData[1]);
    int targetLength = strlen(targetWord);
    int* currentLetter = &ctx->gameData[0];

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // キー入力処理
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (*currentLetter < targetLength)) {
            char pressedChar = (char)toupper(key);
            char targetChar = (char)toupper(targetWord[*currentLetter]);

            if (pressedChar == targetChar) {
                (*currentLetter)++;
                PlaySound(resources.fxType);

                if (*currentLetter >= targetLength) {
                    ctx->state = GAME_STATE_CLEAR;
                    PlaySound(resources.fxSuccess);
                }
            }
        }
        key = GetCharPressed();
    }
}

void TypingGame_Draw(MiniGameContext* ctx) {
    const char* targetWord = GetTargetWord(ctx->level, ctx->gameData[1]);
    int targetLength = strlen(targetWord);
    int currentLetter = ctx->gameData[0];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("TYPE IT!", 350, 100, 30, DARKGRAY);

        // 文字の描画
        int startX = 400 - (targetLength * 20);
        for (int i = 0; i < targetLength; i++) {
            Color color = (i < currentLetter) ? GREEN : LIGHTGRAY;
            DrawText(TextFormat("%c", targetWord[i]), startX + (i * 40), 200, 40, color);
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム2: ボタン連打ゲーム
// ===============================================
void ButtonMashGame_Init(MiniGameContext* ctx) {

    // レベルに応じて制限時間を変更
    if (ctx->level >= 7) ctx->timeLimit = 3.0f;
    else if (ctx->level >= 6) ctx->timeLimit = 3.2f;
    else if (ctx->level >= 5) ctx->timeLimit = 3.5f;
    else if (ctx->level >= 3) ctx->timeLimit = 5.0f;
    else ctx->timeLimit = 7.0f;

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    ctx->gameData[0] = 0;  // 現在のカウント

    // レベルに応じて目標回数を変更
    if (ctx->level >= 8) ctx->gameData[1] = 37;
    else if (ctx->level >= 7) ctx->gameData[1] = 30;
    else if (ctx->level >= 6) ctx->gameData[1] = 25;
    else if (ctx->level >= 5) ctx->gameData[1] = 20;
    else if (ctx->level >= 4) ctx->gameData[1] = 15;
    else if (ctx->level >= 3) ctx->gameData[1] = 10;
    else if (ctx->level >= 2) ctx->gameData[1] = 8;
    else ctx->gameData[1] = 5;
}

void ButtonMashGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    int* count = &ctx->gameData[0];
    int target = ctx->gameData[1];

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // スペースキーで連打
    if (IsKeyPressed(KEY_SPACE)) {
        (*count)++;
        PlaySound(resources.fxpush);

        if (*count >= target) {
            ctx->state = GAME_STATE_CLEAR;
            PlaySound(resources.fxSuccess);
        }
    }
}

void ButtonMashGame_Draw(MiniGameContext* ctx) {
    int count = ctx->gameData[0];
    int target = ctx->gameData[1];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("MASH SPACE!", 300, 100, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d", count, target), 350, 200, 40, BLUE);
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム3: コインクリックゲーム
// ===============================================
void CoinClickGame_Init(MiniGameContext* ctx) {
    // レベルに応じてコイン数と制限時間を設定
    int coinCount = 3;
    if (ctx->level >= 7) { coinCount = 10; ctx->timeLimit = 4.0f; }
    else if (ctx->level >= 5) { coinCount = 8; ctx->timeLimit = 4.4f; }
    else if (ctx->level >= 3) { coinCount = 6; ctx->timeLimit = 4.7f; }
    else if (ctx->level >= 2) { coinCount = 4; ctx->timeLimit = 4.5f; }
    else { coinCount = 3; ctx->timeLimit = 5.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = coinCount; // 総コイン数
    ctx->gameData[1] = 0;         // 獲得数

    // コインの位置をランダムに設定
    for (int i = 0; i < coinCount; i++) {
        int baseIndex = 10 + (i * 3);
        ctx->gameData[baseIndex] = GetRandomValue(50, 750);     // X
        ctx->gameData[baseIndex + 1] = GetRandomValue(50, 350); // Y
        ctx->gameData[baseIndex + 2] = 1;                       // 有効フラグ
    }
}

void CoinClickGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    int coinCount = ctx->gameData[0];
    int* collected = &ctx->gameData[1];

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // クリック判定
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetVirtualMousePosition();
        
        for (int i = 0; i < coinCount; i++) {
            int baseIndex = 10 + (i * 3);
            if (ctx->gameData[baseIndex + 2] == 1) { // 有効なコインのみ
                int coinX = ctx->gameData[baseIndex];
                int coinY = ctx->gameData[baseIndex + 1];
                
                // 円との当たり判定 (半径20)
                float dx = mousePos.x - coinX;
                float dy = mousePos.y - coinY;
                if ((dx*dx + dy*dy) <= (20*20)) {
                    ctx->gameData[baseIndex + 2] = 0; // 無効化
                    (*collected)++;
                    PlaySound(resources.fxCoin); // コイン取得音
                }
            }
        }

        if (*collected >= coinCount) {
            ctx->state = GAME_STATE_CLEAR;
            PlaySound(resources.fxSuccess);
        }
    }
}

void CoinClickGame_Draw(MiniGameContext* ctx) {
    int coinCount = ctx->gameData[0];
    int collected = ctx->gameData[1];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("GET COINS!", 320, 50, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d", collected, coinCount), 350, 380, 30, BLUE);

        // コイン描画
        for (int i = 0; i < coinCount; i++) {
            int baseIndex = 10 + (i * 3);
            if (ctx->gameData[baseIndex + 2] == 1) {
                int coinX = ctx->gameData[baseIndex];
                int coinY = ctx->gameData[baseIndex + 1];
                DrawCircle(coinX, coinY, 20, GOLD);
                DrawCircleLines(coinX, coinY, 20, ORANGE);
                DrawText("$", coinX - 5, coinY - 10, 20, ORANGE);
            }
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム4: 的当てゲーム
// ===============================================
void TargetGame_Init(MiniGameContext* ctx) {
    // レベルに応じて制限時間、的のサイズを設定
    if (ctx->level >= 7) {
        ctx->timeLimit = 4.0f;
        ctx->gameData[4] = 16;  // 的の移動速度
        ctx->gameData[3] = 40;  // 的のサイズ
    } else if (ctx->level >= 5) {
        ctx->timeLimit = 4.5f;
        ctx->gameData[4] = 12;
        ctx->gameData[3] = 60;
    } else if (ctx->level >= 3) {
        ctx->timeLimit = 5.0f;
        ctx->gameData[4] = 8;
        ctx->gameData[3] = 80;
    } else {
        ctx->timeLimit = 5.0f;
        ctx->gameData[4] = 6;
        ctx->gameData[3] = 100;
    }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = GetRandomValue(100, 700); // 的のX位置
    ctx->gameData[1] = GetRandomValue(100, 350); // 的のY位置
    ctx->gameData[2] = (GetRandomValue(0, 1) == 0) ? 1 : -1; // X方向
    ctx->gameData[5] = (GetRandomValue(0, 1) == 0) ? 1 : -1; // Y方向
    ctx->gameData[6] = 0;    // アニメーション状態（0=プレイ中、1=発射中）
    ctx->gameData[7] = 0;    // 弾のX位置
    ctx->gameData[8] = 0;    // 弾のY位置
}

void TargetGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    int* targetX = &ctx->gameData[0];
    int* targetY = &ctx->gameData[1];
    int* dirX = &ctx->gameData[2];
    int targetSize = ctx->gameData[3];
    int speed = ctx->gameData[4];
    int* dirY = &ctx->gameData[5];
    int* animating = &ctx->gameData[6];
    int* bulletX = &ctx->gameData[7];
    int* bulletY = &ctx->gameData[8];

    // アニメーション中でない場合のみタイマー更新
    if (*animating == 0) {
        ctx->timeLeft -= GetFrameTime();
        if (ctx->timeLeft <= 0) {
            ctx->timeLeft = 0;
            ctx->state = GAME_STATE_FAILED;
            PlaySound(resources.fxFail);
            return;
        }

        // 的を移動
        *targetX += (*dirX) * speed;
        *targetY += (*dirY) * speed;
        
        // 画面端で反転
        if (*targetX <= 50 || *targetX >= 750) {
            *dirX = -*dirX;
        }
        if (*targetY <= 100 || *targetY >= 400) {
            *dirY = -*dirY;
        }

        // スペースキーで発射
        if (IsKeyPressed(KEY_ENTER)) {
            *animating = 1;
            *bulletX = 400;
            *bulletY = 420;
            PlaySound(resources.fxSpace);
        }
    } else {
        // アニメーション中：弾を上に移動
        *bulletY -= 12;
        
        // 的との当たり判定
        int dx = (*bulletX - *targetX);
        int dy = (*bulletY - *targetY);
        int distance = dx * dx + dy * dy;
        int hitRange = (targetSize / 2) * (targetSize / 2);
        
        if (distance <= hitRange) {
            ctx->state = GAME_STATE_CLEAR;
            PlaySound(resources.fxSuccess);
            return;
        }
        
        // 画面外に出たら失敗
        if (*bulletY < 0) {
            ctx->state = GAME_STATE_FAILED;
            PlaySound(resources.fxFail);
        }
    }
}

void TargetGame_Draw(MiniGameContext* ctx) {
    int targetX = ctx->gameData[0];
    int targetY = ctx->gameData[1];
    int targetSize = ctx->gameData[3];
    int animating = ctx->gameData[6];
    int bulletX = ctx->gameData[7];
    int bulletY = ctx->gameData[8];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("HIT THE TARGET!", 260, 50, 30, DARKGRAY);
        if (animating == 0) {
            DrawText("Press ENTER to shoot!", 270, 80, 20, GRAY);
        }

        // 的を描画
        DrawCircle(targetX, targetY, targetSize/2, RED);
        DrawCircle(targetX, targetY, targetSize/2 - 8, WHITE);
        DrawCircle(targetX, targetY, targetSize/2 - 16, RED);
        DrawCircle(targetX, targetY, targetSize/2 - 24, WHITE);
        DrawCircle(targetX, targetY, 5, RED);

        // 発射台
        DrawRectangle(380, 420, 40, 30, DARKGRAY);
        DrawTriangle(
            (Vector2){400, 410},
            (Vector2){390, 420},
            (Vector2){410, 420},
            DARKGRAY
        );

        // 弾を描画（発射中のみ）
        if (animating == 1) {
            DrawCircle(bulletX, bulletY, 8, GOLD);
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("HIT!", 340, 180, 60, GOLD);
    } else {
        DrawText("MISSED!", 300, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム5: 計算ゲーム
// ===============================================
// 問題データを指定されたインデックスに生成 (index: 0-2)
void GenerateMathProblemAtIndex(MiniGameContext* ctx, int index) {
    int op = GetRandomValue(0, 3); // 0:+, 1:-, 2:*, 3:/
    int a, b, ans;

    // 1桁と1桁の演算 (1-9)
    switch (op) {
        case 0: // +
            a = GetRandomValue(1, 9);
            b = GetRandomValue(1, 9);
            ans = a + b;
            break;
        case 1: // -
            a = GetRandomValue(1, 9);
            b = GetRandomValue(1, a); // 答えが0以上
            ans = a - b;
            break;
        case 2: // *
            a = GetRandomValue(1, 9);
            b = GetRandomValue(1, 9);
            ans = a * b;
            break;
        case 3: // /
            // 割り切れる数を作る
            while (true) {
                a = GetRandomValue(1, 9);
                b = GetRandomValue(1, 9);
                if (a % b == 0) break;
            }
            ans = a / b;
            break;
    }

    int baseIndex = 10 + (index * 5);
    ctx->gameData[baseIndex + 0] = a;
    ctx->gameData[baseIndex + 1] = b;
    ctx->gameData[baseIndex + 2] = op;
    ctx->gameData[baseIndex + 3] = 0; // 現在の正解桁数
    ctx->gameData[baseIndex + 4] = ans; // 答え
}

void MathGame_Init(MiniGameContext* ctx) {
    // レベル設定
    int problemCount = 3;
    if (ctx->level >= 8) { problemCount = 16; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 7) { problemCount = 14; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 6) { problemCount = 12; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 5) { problemCount = 10; ctx->timeLimit = 10.5f; }
    else if (ctx->level >= 4) { problemCount = 8; ctx->timeLimit = 11.0f; }
    else if (ctx->level >= 3) { problemCount = 6; ctx->timeLimit = 9.0f; }
    else if (ctx->level >= 2) { problemCount = 5; ctx->timeLimit = 10.0f; }
    else { problemCount = 3; ctx->timeLimit = 15.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = 0;            // 現在解答中の問題番号
    ctx->gameData[1] = problemCount; // 目標問題数
    ctx->gameData[6] = 0;            // ペナルティタイマー(ms)
    ctx->gameData[7] = 0;            // スライドアニメーション進行度 (0-100)
    ctx->gameData[8] = 0;            // 生成済み問題数

    // 初期の問題を生成（最大3問まで）
    int initialProblems = (problemCount < 3) ? problemCount : 3;
    for (int i = 0; i < initialProblems; i++) {
        GenerateMathProblemAtIndex(ctx, i);
        ctx->gameData[8]++;
    }
}

void MathGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // スライドアニメーション処理（イージング付き）
    if (ctx->gameData[7] > 0) {
        ctx->gameData[7] += 8; // 進行速度
        if (ctx->gameData[7] >= 100) {
            ctx->gameData[7] = 0;
            // 問題を上にシフト
            for (int i = 0; i < 2; i++) {
                int srcBase = 10 + ((i + 1) * 5);
                int dstBase = 10 + (i * 5);
                for (int j = 0; j < 5; j++) {
                    ctx->gameData[dstBase + j] = ctx->gameData[srcBase + j];
                }
            }
            // 新しい問題を生成（残りがある場合のみ）
            int generatedProblems = ctx->gameData[8];
            int totalProblems = ctx->gameData[1];
            if (generatedProblems < totalProblems) {
                GenerateMathProblemAtIndex(ctx, 2);
                ctx->gameData[8]++;
            } else {
                // 最後の枠を空にする（ダミーデータ）
                int baseIndex = 10 + (2 * 5);
                ctx->gameData[baseIndex + 0] = 0;
                ctx->gameData[baseIndex + 1] = 0;
                ctx->gameData[baseIndex + 2] = 0;
                ctx->gameData[baseIndex + 3] = 0;
                ctx->gameData[baseIndex + 4] = 0;
            }
        }
        return; // アニメーション中は入力を受け付けない
    }

    // ペナルティ処理
    if (ctx->gameData[6] > 0) {
        ctx->gameData[6] -= (int)(GetFrameTime() * 1000);
        if (ctx->gameData[6] < 0) ctx->gameData[6] = 0;
        return; // 操作不能
    }

    // 現在の問題データを取得
    int baseIndex = 10; // 常に先頭の問題を解答
    int a = ctx->gameData[baseIndex + 0];
    int b = ctx->gameData[baseIndex + 1];
    int op = ctx->gameData[baseIndex + 2];
    int* currentDigit = &ctx->gameData[baseIndex + 3];
    int ans = ctx->gameData[baseIndex + 4];

    const char* ansStr = TextFormat("%d", ans);
    int len = strlen(ansStr);

    int key = GetCharPressed();
    while (key > 0) {
        if (key >= '0' && key <= '9') {
            if (key == ansStr[*currentDigit]) {
                // 正解の桁
                (*currentDigit)++;
                PlaySound(resources.fxType);

                if (*currentDigit >= len) {
                    // 1問クリア
                    ctx->gameData[0]++;
                    PlaySound(resources.fxCoin);
                    if (ctx->gameData[0] >= ctx->gameData[1]) {
                        ctx->state = GAME_STATE_CLEAR;
                        PlaySound(resources.fxSuccess);
                    } else {
                        // スライドアニメーション開始
                        ctx->gameData[7] = 1;
                    }
                }
            } else {
                // 不正解
                PlaySound(resources.fxFail);
                ctx->gameData[6] = 1000; // 1秒ペナルティ
            }
        }
        key = GetCharPressed();
    }
}

void MathGame_Draw(MiniGameContext* ctx) {
    int currentProblem = ctx->gameData[0];
    int totalProblems = ctx->gameData[1];
    int penalty = ctx->gameData[6];
    int animProgress = ctx->gameData[7]; // 0-100

    // イージング関数（ease-out quad）
    float t = animProgress / 100.0f;
    float eased = 1.0f - (1.0f - t) * (1.0f - t);
    int slideOffset = (int)(eased * 120.0f);

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("CALCULATE!", 290, 30, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d", currentProblem + 1, totalProblems), 360, 400, 20, BLUE);

        // 3問を縦に並べて表示
        for (int i = 0; i < 3; i++) {
            int baseIndex = 10 + (i * 5);
            int a = ctx->gameData[baseIndex + 0];
            int b = ctx->gameData[baseIndex + 1];
            int op = ctx->gameData[baseIndex + 2];
            int currentDigit = ctx->gameData[baseIndex + 3];
            int ans = ctx->gameData[baseIndex + 4];

            // 空の問題はスキップ
            if (a == 0 && b == 0 && ans == 0) continue;

            char opChar = '+';
            if (op == 1) opChar = '-';
            else if (op == 2) opChar = 'x';
            else if (op == 3) opChar = '/';

            // Y位置計算（スライドオフセットを適用）
            int yPos = 80 + (i * 120) - slideOffset;

            // 問題が画面内にある場合のみ描画
            if (yPos >= 60 && yPos <= 420) {
                // 現在解答中の問題は強調
                Color textColor = (i == 0) ? DARKBLUE : GRAY;
                int fontSize = (i == 0) ? 50 : 35;

                // 数式の描画
                const char* problemText = TextFormat("%d %c %d = ", a, opChar, b);
                int textWidth = MeasureText(problemText, fontSize);
                DrawText(problemText, 400 - textWidth, yPos, fontSize, textColor);

                // 入力済み数字の描画（現在の問題のみ）
                if (i == 0) {
                    const char* ansStr = TextFormat("%d", ans);
                    char inputBuf[16] = {0};
                    strncpy(inputBuf, ansStr, currentDigit);
                    DrawText(inputBuf, 400, yPos, fontSize, GREEN);
                }
            }
        }

        // ペナルティ表示
        if (penalty > 0) {
            DrawText("X", 360, 250, 80, RED);
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム6: マインスイーパー
// ===============================================
void MinesweeperGame_Init(MiniGameContext* ctx) {
    // レベルに応じてグリッドサイズと制限時間を設定
    int gridSize = 5;
    if (ctx->level >= 8) { gridSize = 9; ctx->timeLimit = 7.0f; }
    else if (ctx->level >= 7) { gridSize = 8; ctx->timeLimit = 8.0f; }
    else if (ctx->level >= 6) { gridSize = 7; ctx->timeLimit = 8.0f; }
    else if (ctx->level >= 5) { gridSize = 7; ctx->timeLimit = 9.0f; }
    else if (ctx->level >= 4) { gridSize = 6; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 3) { gridSize = 5; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 2) { gridSize = 4; ctx->timeLimit = 10.0f; }
    else { gridSize = 3; ctx->timeLimit = 10.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = gridSize; // グリッドサイズ
    
    // 爆弾の数を設定（グリッドサイズに応じて）
    int mineCount = (gridSize * gridSize) / 5;
    if (mineCount < 3) mineCount = 3;
    ctx->gameData[1] = mineCount;
    
    // グリッド初期化（最大10x10=100マス）
    // gameData[10-109]: 各マスの状態（0=空、1=爆弾）
    // gameData[110-209]: 各マスの開示状態（0=開示済み、1=未開示）
    for (int i = 0; i < 100; i++) {
        ctx->gameData[10 + i] = 0;
        ctx->gameData[110 + i] = 0; // 初期は全て開示済み
    }
    
    // 爆弾をランダム配置
    for (int i = 0; i < mineCount; i++) {
        int pos;
        do {
            pos = GetRandomValue(0, gridSize * gridSize - 1);
        } while (ctx->gameData[10 + pos] == 1);
        ctx->gameData[10 + pos] = 1;
        ctx->gameData[110 + pos] = 1; // 全ての爆弾マスを未開示にする
    }
    
    // 安全な場所から正解を1つ選ぶ（唯一の未開示安全マス）
    int answerPos;
    do {
        answerPos = GetRandomValue(0, gridSize * gridSize - 1);
    } while (ctx->gameData[10 + answerPos] == 1); // 爆弾でない場所
    
    ctx->gameData[2] = answerPos; // 正解の位置
    ctx->gameData[110 + answerPos] = 1; // 未開示にする
    ctx->gameData[3] = -1; // 開示アニメーション用インデックス（-1=未選択）
    ctx->gameData[4] = 0;  // アニメーション用タイマー
}

void MinesweeperGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    int gridSize = ctx->gameData[0];
    int answerPos = ctx->gameData[2];
    int revealIndex = ctx->gameData[3];

    // 開示アニメーション中の処理
    if (revealIndex >= 0) {
        // タイマーで順番に開いていく
        ctx->gameData[4] += GetFrameTime() * 1000; // ミリ秒単位
        
        if (ctx->gameData[4] >= 160) { // 0.16秒ごとに1マス開く
            ctx->gameData[4] = 0;
            
            int totalCells = gridSize * gridSize;
            
            // 次の未開示マスを探す
            while (revealIndex < totalCells) {
                // 未開示マスのみ処理
                if (ctx->gameData[110 + revealIndex] == 1) {
                    // 現在のマスを開く
                    ctx->gameData[110 + revealIndex] = 0; // 開示済みにする
                    
                    // 爆弾が出たら音を鳴らす
                    if (ctx->gameData[10 + revealIndex] == 1) {
                        PlaySound(resources.fxBoom);
                        
                        // 選択マスが爆弾だった場合のみ失敗
                        if (revealIndex == answerPos) {
                            ctx->state = GAME_STATE_FAILED;
                            PlaySound(resources.fxFail);
                            return;
                        }
                    }
                    
                    ctx->gameData[3]++; // 次のマスへ
                    break; // 1マス開いたので一旦抜ける
                } else {
                    // 既に開示済みのマスはスキップ
                    ctx->gameData[3]++;
                    revealIndex = ctx->gameData[3];
                }
            }
            
            // 全マス確認完了
            if (revealIndex >= totalCells) {
                ctx->state = GAME_STATE_CLEAR;
                PlaySound(resources.fxSuccess);
            }
        }
        return; // アニメーション中は他の入力を受け付けない
    }

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // クリック判定
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetVirtualMousePosition();
        
        // グリッドの描画位置とサイズを計算
        int cellSize = 350 / gridSize;
        if (cellSize > 40) cellSize = 40;
        int gridWidth = cellSize * gridSize;
        int gridHeight = cellSize * gridSize;
        int startX = (800 - gridWidth) / 2;
        int startY = (450 - gridHeight) / 2 + 20;
        
        // どのマスがクリックされたか判定
        if (mousePos.x >= startX && mousePos.x < startX + gridWidth &&
            mousePos.y >= startY && mousePos.y < startY + gridHeight) {
            
            int col = (int)(mousePos.x - startX) / cellSize;
            int row = (int)(mousePos.y - startY) / cellSize;
            int clickedPos = row * gridSize + col;
            
            // 未開示のマスがクリックされたかチェック
            if (ctx->gameData[110 + clickedPos] == 1) {
                // 選択したマスを記録して開示アニメーション開始
                ctx->gameData[2] = clickedPos; // answerPosを選択位置に更新
                ctx->gameData[3] = 0; // 開示アニメーション開始（左上から）
                ctx->gameData[4] = 0; // タイマーリセット
                PlaySound(resources.fxpush);
            }
        }
    }
}

void MinesweeperGame_Draw(MiniGameContext* ctx) {
    int gridSize = ctx->gameData[0];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("FIND THE SAFE CELL!", 240, 30, 30, DARKGRAY);
        
        // グリッドの描画
        int cellSize = 350 / gridSize;
        if (cellSize > 40) cellSize = 40;
        int gridWidth = cellSize * gridSize;
        int gridHeight = cellSize * gridSize;
        int startX = (800 - gridWidth) / 2;
        int startY = (450 - gridHeight) / 2 + 20;
        
        for (int row = 0; row < gridSize; row++) {
            for (int col = 0; col < gridSize; col++) {
                int pos = row * gridSize + col;
                int x = startX + col * cellSize;
                int y = startY + row * cellSize;
                
                if (ctx->gameData[110 + pos] == 1) {
                    // 未開示マス
                    DrawRectangle(x, y, cellSize - 2, cellSize - 2, DARKGRAY);
                    DrawRectangleLines(x, y, cellSize - 2, cellSize - 2, BLACK);
                } else {
                    // 開いているマス
                    int answerPos = ctx->gameData[2]; // 選択されたマスの位置
                    
                    if (ctx->gameData[10 + pos] == 1) {
                        // 爆弾マス
                        DrawRectangle(x, y, cellSize - 2, cellSize - 2, RED);
                        DrawRectangleLines(x, y, cellSize - 2, cellSize - 2, BLACK);
                        
                        // 爆弾アイコン（●）
                        int fontSize = cellSize / 2;
                        if (fontSize > 20) fontSize = 20;
                        const char* bombText = "X";
                        int textWidth = MeasureText(bombText, fontSize);
                        DrawText(bombText, x + (cellSize - textWidth) / 2, y + (cellSize - fontSize) / 2, fontSize, BLACK);
                    } else {
                        // 安全なマス - 選択されたマスは黄色で表示
                        Color bgColor = (pos == answerPos && ctx->gameData[3] >= 0) ? YELLOW : LIGHTGRAY;
                        DrawRectangle(x, y, cellSize - 2, cellSize - 2, bgColor);
                        DrawRectangleLines(x, y, cellSize - 2, cellSize - 2, GRAY);
                        
                        // 周囲の爆弾をカウント
                        int count = 0;
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (dx == 0 && dy == 0) continue;
                                int nr = row + dy;
                                int nc = col + dx;
                                if (nr >= 0 && nr < gridSize && nc >= 0 && nc < gridSize) {
                                    int npos = nr * gridSize + nc;
                                    if (ctx->gameData[10 + npos] == 1) {
                                        count++;
                                    }
                                }
                            }
                        }
                        
                        // 数字を表示（0も含む）
                        Color numColor = BLUE;
                        if (count == 0) numColor = GRAY;
                        else if (count == 2) numColor = GREEN;
                        else if (count == 3) numColor = RED;
                        else if (count >= 4) numColor = DARKPURPLE;
                        
                        int fontSize = cellSize / 2;
                        if (fontSize > 20) fontSize = 20;
                        const char* numText = TextFormat("%d", count);
                        int textWidth = MeasureText(numText, fontSize);
                        DrawText(numText, x + (cellSize - textWidth) / 2, y + (cellSize - fontSize) / 2, fontSize, numColor);
                    }
                }
            }
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("WRONG!", 310, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム7: 音楽演奏ゲーム
// ===============================================
void MusicGame_Init(MiniGameContext* ctx) {
    // レベルに応じて音符数と制限時間を設定
    int noteCount = 4;
    if (ctx->level >= 7) { noteCount = 30; ctx->timeLimit = 6.0f; }
    else if (ctx->level >= 6) { noteCount = 25; ctx->timeLimit = 6.3f; }
    else if (ctx->level >= 5) { noteCount = 22; ctx->timeLimit = 6.4f; }
    else if (ctx->level >= 4) { noteCount = 20; ctx->timeLimit = 6.5f; }
    else if (ctx->level >= 3) { noteCount = 15; ctx->timeLimit = 6.0f; }
    else if (ctx->level >= 2) { noteCount = 12; ctx->timeLimit = 6.0f; }
    else { noteCount = 10; ctx->timeLimit = 7.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = noteCount; // 総音符数
    ctx->gameData[1] = 0;         // 現在の入力位置
    ctx->gameData[2] = 0;         // ペナルティタイマー
    ctx->gameData[3] = 0;         // スライドアニメーション進行度（0-100）
    
    // 譜面をランダムに生成（gameData[10-109]に格納）
    // 0 = Space (・), 1 = Enter (○)
    for (int i = 0; i < noteCount; i++) {
        ctx->gameData[10 + i] = GetRandomValue(0, 1);
    }
}

void MusicGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // スライドアニメーション処理
    if (ctx->gameData[3] > 0) {
        ctx->gameData[3] += 25; // 高速アニメーション
        if (ctx->gameData[3] >= 100) {
            ctx->gameData[3] = 0;
        }
        return; // アニメーション中は入力不可
    }

    // ペナルティ処理
    if (ctx->gameData[2] > 0) {
        ctx->gameData[2] -= (int)(GetFrameTime() * 1000);
        if (ctx->gameData[2] < 0) ctx->gameData[2] = 0;
        return; // 操作不能
    }

    int noteCount = ctx->gameData[0];
    int currentPos = ctx->gameData[1];
    
    if (currentPos >= noteCount) return; // 全て入力済み
    
    int expectedNote = ctx->gameData[10 + currentPos];
    
    // キー入力判定
    bool spacePressed = IsKeyPressed(KEY_SPACE);
    bool enterPressed = IsKeyPressed(KEY_ENTER);
    
    if (spacePressed || enterPressed) {
        int inputNote = spacePressed ? 0 : 1;
        
        if (inputNote == expectedNote) {
            // 正解
            ctx->gameData[1]++;
            if (inputNote == 0) {
                PlaySound(resources.fxSpace); // Space用の音
            } else {
                PlaySound(resources.fxEnter); // Enter用の音
            }
            
            // 10個単位でページが切り替わる時だけアニメーション開始
            if (ctx->gameData[1] < noteCount && (ctx->gameData[1] % 10) == 0) {
                ctx->gameData[3] = 1;
            }
            
            // 全て入力完了
            if (ctx->gameData[1] >= noteCount) {
                ctx->state = GAME_STATE_CLEAR;
                PlaySound(resources.fxSuccess);
            }
        } else {
            // 不正解
            PlaySound(resources.fxFail);
            ctx->gameData[2] = 300; // 0.3秒ペナルティ
        }
    }
}

void MusicGame_Draw(MiniGameContext* ctx) {
    int noteCount = ctx->gameData[0];
    int currentPos = ctx->gameData[1];
    int penalty = ctx->gameData[2];
    int animProgress = ctx->gameData[3];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("PLAY THE MUSIC!", 270, 50, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d", currentPos, noteCount), 360, 90, 20, BLUE);
        
        // 10個ずつページ表示
        int page = currentPos / 10;  // 現在のページ
        int startX = 100;
        int y = 200;
        int spacing = 60;
        
        // スライドオフセット計算（線形、画面全体を移動）
        float t = animProgress / 100.0f;
        int slideOffset = (int)(t * 800);  // 画面幅分スクロール
        
        // 現在のページと次のページを表示
        for (int pageOffset = 0; pageOffset <= 1; pageOffset++) {
            int displayPage = page + pageOffset;
            int pageStartIdx = displayPage * 10;
            int pageEndIdx = pageStartIdx + 10;
            if (pageEndIdx > noteCount) pageEndIdx = noteCount;
            
            int pageX = pageOffset * 800 - slideOffset;  // ページのX座標
            
            for (int i = pageStartIdx; i < pageEndIdx; i++) {
                int indexInPage = i - pageStartIdx;
                int x = startX + pageX + (indexInPage * spacing);
            
                // 画面外は表示しない
                if (x < -50 || x > 850) continue;
                
                int noteType = ctx->gameData[10 + i];
                
                // 入力済みは緑、現在位置は黄色、未入力は灰色
                Color color = GRAY;
                if (i < currentPos) {
                    color = GREEN;
                } else if (i == currentPos) {
                    color = YELLOW;
                }
                
                if (noteType == 0) {
                    // Space
                    DrawCircle(x, y, 8, color);
                    DrawCircleLines(x, y, 8, BLACK);
                    // 下にキー表示
                    if (i == currentPos) {
                        DrawText("SPACE", x - 15, y + 20, 12, DARKGRAY);
                    }
                } else {
                    // Enter
                    DrawRectangle(x - 15, y - 15, 30, 30, color);
                    DrawRectangleLines(x - 15, y - 15, 30, 30, BLACK);
                    // 下にキー表示
                    if (i == currentPos) {
                        DrawText("ENTER", x - 15, y + 25, 12, DARKGRAY);
                    }
                }
            }
        }
        
        // 入力ガイド
        DrawText("Space: .   Enter: O", 310, 320, 20, DARKGRAY);
        
        // ペナルティ表示
        if (penalty > 0) {
            DrawText("X", 380, 250, 60, RED);
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム8: 6キーリズムゲーム (ASDJKL)
// ===============================================
void SixKeyRhythmGame_Init(MiniGameContext* ctx) {
    // レベルに応じてノーツ数と制限時間を設定
    int noteCount = 5;
    if (ctx->level >= 8) { noteCount = 40; ctx->timeLimit = 8.0f; }
    else if (ctx->level >= 7) { noteCount = 35; ctx->timeLimit = 8.0f; }
    else if (ctx->level >= 6) { noteCount = 35; ctx->timeLimit = 9.0f; }
    else if (ctx->level >= 5) { noteCount = 22; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 4) { noteCount = 20; ctx->timeLimit = 10.0f; }
    else if (ctx->level >= 3) { noteCount = 18; ctx->timeLimit = 12.0f; }
    else if (ctx->level >= 2) { noteCount = 15; ctx->timeLimit = 15.0f; }
    else { noteCount = 10; ctx->timeLimit = 20.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = noteCount;  // 総ノーツ数
    ctx->gameData[1] = 0;          // 現在のノーツインデックス
    ctx->gameData[2] = 0;          // ペナルティタイマー
    ctx->gameData[3] = 0;          // スクロールアニメーション進行度（0-100）
    
    // ノーツデータ生成（gameData[10-69]に格納）
    // 各ノーツは6ビットで6つのキー(A,S,D,J,K,L)のON/OFFを表現
    int previousKeyIndex = -1; // 前のノーツのキーインデックス
    for (int i = 0; i < noteCount; i++) {
        // 1つのキーだけを選択（前のノーツと同じキーを避ける）
        int keyIndex;
        do {
            keyIndex = GetRandomValue(0, 5); // 0-5 (A,S,D,J,K,L)
        } while (i > 0 && keyIndex == previousKeyIndex); // 前のノーツと同じキーは避ける
        
        int keys = (1 << keyIndex);
        ctx->gameData[10 + i] = keys;
        previousKeyIndex = keyIndex;
    }
}

void SixKeyRhythmGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    // スクロールアニメーション処理
    if (ctx->gameData[3] > 0) {
        ctx->gameData[3] += 25; // 高速アニメーション
        if (ctx->gameData[3] >= 100) {
            ctx->gameData[3] = 0;
        }
        return; // アニメーション中は入力不可
    }

    // ペナルティ処理
    if (ctx->gameData[2] > 0) {
        ctx->gameData[2] -= (int)(GetFrameTime() * 1000);
        if (ctx->gameData[2] < 0) ctx->gameData[2] = 0;
        return; // 操作不能
    }

    int noteCount = ctx->gameData[0];
    int currentNote = ctx->gameData[1];
    
    if (currentNote >= noteCount) return; // 全て完了
    
    int expectedKeys = ctx->gameData[10 + currentNote];
    
    // 現在押されているキーを検出
    int currentKeys = 0;
    if (IsKeyDown(KEY_A)) currentKeys |= (1 << 0);
    if (IsKeyDown(KEY_S)) currentKeys |= (1 << 1);
    if (IsKeyDown(KEY_D)) currentKeys |= (1 << 2);
    if (IsKeyDown(KEY_J)) currentKeys |= (1 << 3);
    if (IsKeyDown(KEY_K)) currentKeys |= (1 << 4);
    if (IsKeyDown(KEY_L)) currentKeys |= (1 << 5);
    
    // 必要なキーの数を数える
    int expectedCount = 0;
    for (int i = 0; i < 6; i++) {
        if (expectedKeys & (1 << i)) expectedCount++;
    }
    
    // 押されているキーの数を数える
    int currentCount = 0;
    for (int i = 0; i < 6; i++) {
        if (currentKeys & (1 << i)) currentCount++;
    }
    
    // いずれかのキーが新たに押されたか検出
    bool anyKeyPressed = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) ||
                         IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_L);
    
    // キー入力判定：必要な数のキーが押されたら判定
    if (anyKeyPressed && currentCount >= expectedCount) {
        // 必要なキーが全て押されているかチェック
        if ((currentKeys & expectedKeys) == expectedKeys) {
            // 正解（必要なキーが全て押されている）
            ctx->gameData[1]++;
            PlaySound(resources.fxEnter);
            
            // スクロールアニメーション開始
            if (ctx->gameData[1] < noteCount) {
                ctx->gameData[3] = 1;
            }
            
            // 全て完了
            if (ctx->gameData[1] >= noteCount) {
                ctx->state = GAME_STATE_CLEAR;
                PlaySound(resources.fxSuccess);
            }
        } else {
            // 不正解
            PlaySound(resources.fxFail);
            ctx->gameData[2] = 300; // 0.3秒ペナルティ
        }
    }
}

void SixKeyRhythmGame_Draw(MiniGameContext* ctx) {
    int noteCount = ctx->gameData[0];
    int currentNote = ctx->gameData[1];
    int penalty = ctx->gameData[2];
    int animProgress = ctx->gameData[3];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("6-KEY RHYTHM!", 280, 30, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d", currentNote, noteCount), 360, 70, 20, BLUE);
        
        // 6つのレーン（横並び）
        const char* keyLabels[] = {"A", "S", "D", "J", "K", "L"};
        int laneWidth = 80;
        int laneHeight = 350;
        int startX = (800 - laneWidth * 6) / 2;
        int startY = 50;  // もっと上に
        int judgeLineY = startY + laneHeight - 80;
        
        // レーン描画
        for (int i = 0; i < 6; i++) {
            int x = startX + i * laneWidth;
            
            // レーン枠
            DrawRectangleLines(x, startY, laneWidth, laneHeight, GRAY);
            
            // キーラベル
            DrawText(keyLabels[i], x + laneWidth / 2 - 8, startY + laneHeight + 10, 30, DARKGRAY);
        }
        
        // スクロールオフセット計算（線形、上から下へ）
        float t = animProgress / 100.0f;
        int noteSpacing = 70; // ノーツ間の距離
        int slideOffset = (int)(t * noteSpacing); // 上から下へ移動
        
        // 3つ先までのノーツを表示（次、次の次、次の次の次、現在）
        for (int noteOffset = 3; noteOffset >= 0; noteOffset--) {
            int noteIndex = currentNote + noteOffset;
            if (noteIndex >= noteCount) continue;
            
            int keys = ctx->gameData[10 + noteIndex];
            
            // noteOffset=0（現在）は判定ライン、他は上から降りてくる
            int y;
            if (noteOffset == 0) {
                // 現在のノーツは判定ラインに固定
                y = judgeLineY;
            } else {
                // 次以降のノーツは上に並ぶ
                y = judgeLineY - (noteSpacing * noteOffset) + slideOffset;
            }
            
            // 画面外は表示しない
            if (y < startY - 50 || y > startY + laneHeight + 50) continue;
            
            // 各レーンのノーツを描画
            for (int i = 0; i < 6; i++) {
                if (keys & (1 << i)) {
                    int x = startX + i * laneWidth;
                    
                    Color color = (noteIndex == currentNote) ? YELLOW : GRAY;
                    
                    // ノーツ（横長の矩形）
                    DrawRectangle(x + 10, y, laneWidth - 20, 40, color);
                    DrawRectangleLines(x + 10, y, laneWidth - 20, 40, BLACK);
                }
            }
        }
        
        // 判定ライン（下の方に表示）
        DrawRectangle(startX, judgeLineY, laneWidth * 6, 3, RED);
        
        // ペナルティ表示
        if (penalty > 0) {
            DrawText("MISS!", 350, 250, 40, RED);
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム9: 神経衰弱
// ===============================================
void MemoryCardGame_Init(MiniGameContext* ctx) {
    // レベルに応じてカードのペア数と制限時間を設定
    int pairCount = 3;
    if (ctx->level >= 8) { pairCount = 6; ctx->timeLimit = 9.0f; }
    else if (ctx->level >= 7) { pairCount = 6; ctx->timeLimit = 11.0f; }
    else if (ctx->level >= 6) { pairCount = 6; ctx->timeLimit = 13.0f; }
    else if (ctx->level >= 5) { pairCount = 5; ctx->timeLimit = 13.0f; }
    else if (ctx->level >= 4) { pairCount = 4; ctx->timeLimit = 13.0f; }
    else if (ctx->level >= 3) { pairCount = 3; ctx->timeLimit = 13.0f; }
    else if (ctx->level >= 2) { pairCount = 3; ctx->timeLimit = 15.0f; }
    else { pairCount = 2; ctx->timeLimit = 10.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    int totalCards = pairCount * 2;
    
    ctx->gameData[0] = totalCards;      // 総カード数
    ctx->gameData[1] = 0;               // 見つかったペア数
    ctx->gameData[2] = -1;              // 1枚目のカードインデックス（-1=なし）
    ctx->gameData[3] = -1;              // 2枚目のカードインデックス（-1=なし）
    ctx->gameData[4] = 0;               // アニメーションタイマー（ミリ秒）
    
    // カードの値を生成（gameData[10-25]に格納、最大8ペア=16枚）
    for (int i = 0; i < pairCount; i++) {
        ctx->gameData[10 + i * 2] = i;      // ペアの1枚目
        ctx->gameData[10 + i * 2 + 1] = i;  // ペアの2枚目
    }
    
    // カードをシャッフル
    for (int i = 0; i < totalCards; i++) {
        int j = GetRandomValue(0, totalCards - 1);
        int temp = ctx->gameData[10 + i];
        ctx->gameData[10 + i] = ctx->gameData[10 + j];
        ctx->gameData[10 + j] = temp;
    }
    
    // カードの状態（gameData[30-45]に格納、0=裏、1=表、2=見つかった）
    for (int i = 0; i < totalCards; i++) {
        ctx->gameData[30 + i] = 0;  // 全て裏向き
    }
}

void MemoryCardGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    int totalCards = ctx->gameData[0];
    int foundPairs = ctx->gameData[1];
    int firstCard = ctx->gameData[2];
    int secondCard = ctx->gameData[3];
    
    // アニメーション中
    if (ctx->gameData[4] > 0) {
        ctx->gameData[4] -= (int)(GetFrameTime() * 1000);
        if (ctx->gameData[4] <= 0) {
            ctx->gameData[4] = 0;
            
            // 2枚めくった後の処理
            if (firstCard >= 0 && secondCard >= 0) {
                int value1 = ctx->gameData[10 + firstCard];
                int value2 = ctx->gameData[10 + secondCard];
                
                if (value1 == value2) {
                    // ペア成立
                    ctx->gameData[30 + firstCard] = 2;  // 見つかった状態
                    ctx->gameData[30 + secondCard] = 2;
                    ctx->gameData[1]++;  // ペア数を増やす
                    PlaySound(resources.fxSuccess);
                    
                    // 全ペア見つかったらクリア
                    if (ctx->gameData[1] >= totalCards / 2) {
                        ctx->state = GAME_STATE_CLEAR;
                    }
                } else {
                    // ペア不成立、裏に戻す
                    ctx->gameData[30 + firstCard] = 0;
                    ctx->gameData[30 + secondCard] = 0;
                    PlaySound(resources.fxFail);
                }
                
                // 選択をリセット
                ctx->gameData[2] = -1;
                ctx->gameData[3] = -1;
            }
        }
        return;
    }
    
    // クリック判定
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetVirtualMousePosition();
        
        // カードのレイアウト計算
        int cols = (totalCards <= 6) ? 3 : 4;
        int rows = (totalCards + cols - 1) / cols;
        int cardWidth = 60;
        int cardHeight = 80;
        int spacing = 10;
        int gridWidth = cols * (cardWidth + spacing) - spacing;
        int gridHeight = rows * (cardHeight + spacing) - spacing;
        int startX = (800 - gridWidth) / 2;
        int startY = (450 - gridHeight) / 2;
        
        // どのカードがクリックされたか判定
        for (int i = 0; i < totalCards; i++) {
            if (ctx->gameData[30 + i] != 0) continue;  // 既に表または見つかっている
            
            int col = i % cols;
            int row = i / cols;
            int x = startX + col * (cardWidth + spacing);
            int y = startY + row * (cardHeight + spacing);
            
            if (mousePos.x >= x && mousePos.x < x + cardWidth &&
                mousePos.y >= y && mousePos.y < y + cardHeight) {
                
                // カードをめくる
                ctx->gameData[30 + i] = 1;  // 表向き
                PlaySound(resources.fxType);
                
                if (firstCard == -1) {
                    // 1枚目
                    ctx->gameData[2] = i;
                } else if (secondCard == -1) {
                    // 2枚目
                    ctx->gameData[3] = i;
                    ctx->gameData[4] = 300;  // 0.3秒待つ（短め）
                }
                break;
            }
        }
    }
}

void MemoryCardGame_Draw(MiniGameContext* ctx) {
    int totalCards = ctx->gameData[0];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("MEMORY CARD!", 290, 30, 30, DARKGRAY);
        DrawText(TextFormat("%d / %d pairs", ctx->gameData[1], totalCards / 2), 340, 70, 20, BLUE);
        
        // カードのレイアウト計算
        int cols = (totalCards <= 6) ? 3 : 4;
        int rows = (totalCards + cols - 1) / cols;
        int cardWidth = 60;
        int cardHeight = 80;
        int spacing = 10;
        int gridWidth = cols * (cardWidth + spacing) - spacing;
        int gridHeight = rows * (cardHeight + spacing) - spacing;
        int startX = (800 - gridWidth) / 2;
        int startY = (450 - gridHeight) / 2;
        
        // カード描画
        for (int i = 0; i < totalCards; i++) {
            int col = i % cols;
            int row = i / cols;
            int x = startX + col * (cardWidth + spacing);
            int y = startY + row * (cardHeight + spacing);
            
            int state = ctx->gameData[30 + i];
            
            if (state == 0) {
                // 裏向き
                DrawRectangle(x, y, cardWidth, cardHeight, DARKGRAY);
                DrawRectangleLines(x, y, cardWidth, cardHeight, BLACK);
                DrawText("?", x + cardWidth / 2 - 8, y + cardHeight / 2 - 10, 30, WHITE);
            } else {
                // 表向き（または見つかった）
                int value = ctx->gameData[10 + i];
                Color color = (state == 2) ? GREEN : YELLOW;
                
                DrawRectangle(x, y, cardWidth, cardHeight, color);
                DrawRectangleLines(x, y, cardWidth, cardHeight, BLACK);
                DrawText(TextFormat("%d", value), x + cardWidth / 2 - 8, y + cardHeight / 2 - 10, 30, BLACK);
            }
        }
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ミニゲーム10: 巻物展開ゲーム
// ===============================================
void ScrollUnrollGame_Init(MiniGameContext* ctx) {
    // レベルに応じて巻物の長さと制限時間を設定
    int scrollLength = 300;
    if (ctx->level >= 8) { scrollLength = 10000; ctx->timeLimit = 5.0f; }
    else if (ctx->level >= 7) { scrollLength = 8000; ctx->timeLimit = 5.0f; }
    else if (ctx->level >= 6) { scrollLength = 7000; ctx->timeLimit = 5.5f; }
    else if (ctx->level >= 5) { scrollLength = 6000; ctx->timeLimit = 5.5f; }
    else if (ctx->level >= 4) { scrollLength = 5000; ctx->timeLimit = 5.5f; }
    else if (ctx->level >= 3) { scrollLength = 4500; ctx->timeLimit = 5.5f; }
    else if (ctx->level >= 2) { scrollLength = 4000; ctx->timeLimit = 5.0f; }
    else { scrollLength = 3000; ctx->timeLimit = 5.0f; }

    ctx->timeLeft = ctx->timeLimit;
    ctx->state = GAME_STATE_PLAYING;
    
    ctx->gameData[0] = scrollLength;  // 巻物の総長さ
    ctx->gameData[1] = 50;            // 現在開いた長さ（最初に少し出しておく）
    ctx->gameData[2] = 0;             // ドラッグ中フラグ
    ctx->gameData[3] = 0;             // 前フレームのマウスX
}

void ScrollUnrollGame_Update(MiniGameContext* ctx) {
    if (ctx->state != GAME_STATE_PLAYING) return;

    // タイマー更新
    ctx->timeLeft -= GetFrameTime();
    if (ctx->timeLeft <= 0) {
        ctx->timeLeft = 0;
        ctx->state = GAME_STATE_FAILED;
        PlaySound(resources.fxFail);
        return;
    }

    int scrollLength = ctx->gameData[0];
    int unrolledLength = ctx->gameData[1];
    
    // マウスドラッグで開く
    Vector2 mousePos = GetVirtualMousePosition();
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // ドラッグ開始
        ctx->gameData[2] = 1;
        ctx->gameData[3] = (int)mousePos.x;
    }
    
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && ctx->gameData[2] == 1) {
        // ドラッグ中
        int prevX = ctx->gameData[3];
        int currentX = (int)mousePos.x;
        int deltaX = currentX - prevX;
        
        if (deltaX > 0) {  // 右方向のみ
            ctx->gameData[1] += deltaX;
            PlaySound(resources.fxpush);
            
            if (ctx->gameData[1] >= scrollLength) {
                ctx->gameData[1] = scrollLength;
                ctx->state = GAME_STATE_CLEAR;
                PlaySound(resources.fxSuccess);
            }
        }
        
        ctx->gameData[3] = currentX;
    }
    
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        // ドラッグ終了
        ctx->gameData[2] = 0;
    }
}

void ScrollUnrollGame_Draw(MiniGameContext* ctx) {
    int scrollLength = ctx->gameData[0];
    int unrolledLength = ctx->gameData[1];

    // タイマーバー
    DrawRectangle(0, 0, (int)((ctx->timeLeft / ctx->timeLimit) * 800), 20, RED);

    if (ctx->state == GAME_STATE_PLAYING) {
        DrawText("UNROLL THE SCROLL!", 250, 30, 30, DARKGRAY);
        DrawText("Drag Right to Unroll!", 280, 70, 20, GRAY);
        
        // 進行度バー
        int progressBarWidth = 600;
        int progressBarHeight = 30;
        int progressBarX = (800 - progressBarWidth) / 2;
        int progressBarY = 350;
        
        DrawRectangleLines(progressBarX, progressBarY, progressBarWidth, progressBarHeight, BLACK);
        float progress = (float)unrolledLength / scrollLength;
        DrawRectangle(progressBarX, progressBarY, (int)(progressBarWidth * progress), progressBarHeight, GREEN);
        
        DrawText(TextFormat("%d%%", (int)(progress * 100)), progressBarX + progressBarWidth / 2 - 20, progressBarY + 5, 20, BLACK);
        
        // 巻物のビジュアル（左側に円筒、右側に開いた部分）
        int scrollX = 50;
        int scrollY = 120;
        int scrollRollRadius = 40;  // 巻物の円筒半径
        int paperHeight = 180;  // 紙の高さ（縦長に）
        
        // 矢印を先に描画（巻物の下になるように）
        int arrowY = scrollY + paperHeight / 2;
        int arrowStartX = 150;
        int arrowEndX = 300;
        int arrowSize = 15;
        
        // 矢印の線
        DrawLineEx((Vector2){arrowStartX, arrowY}, (Vector2){arrowEndX, arrowY}, 4, ORANGE);
        // 矢印の先端（三角形）
        DrawTriangle(
            (Vector2){arrowEndX, arrowY},
            (Vector2){arrowEndX - arrowSize, arrowY - arrowSize},
            (Vector2){arrowEndX - arrowSize, arrowY + arrowSize},
            ORANGE
        );
        DrawText("Drag!", arrowStartX + 50, arrowY - 30, 20, ORANGE);
        
        // 左側の巻物（円筒）- 残りの長さに応じて太さが変わる
        int remainingLength = scrollLength - unrolledLength;
        int rollRadius = scrollRollRadius * remainingLength / scrollLength;
        if (rollRadius < 10) rollRadius = 10;
        
        // 円筒を縦長に描画（長方形+上下に半円）
        DrawRectangle(scrollX - rollRadius, scrollY, rollRadius * 2, paperHeight, (Color){139, 69, 19, 255});  // 茶色
        DrawCircle(scrollX, scrollY, rollRadius, (Color){139, 69, 19, 255});
        DrawCircle(scrollX, scrollY + paperHeight, rollRadius, (Color){139, 69, 19, 255});
        DrawRectangleLines(scrollX - rollRadius, scrollY, rollRadius * 2, paperHeight, BLACK);
        DrawCircleLines(scrollX, scrollY, rollRadius, BLACK);
        DrawCircleLines(scrollX, scrollY + paperHeight, rollRadius, BLACK);
        
        // 開いた紙の部分
        int paperWidth = (unrolledLength * 600) / scrollLength;  // 最大600ピクセル
        if (paperWidth > 0) {
            DrawRectangle(scrollX, scrollY, paperWidth, paperHeight, (Color){255, 248, 220, 255});  // 古紙の色
            DrawRectangleLines(scrollX, scrollY, paperWidth, paperHeight, BLACK);
            
            // 紙に線を描く（テキスト風）
            for (int i = 0; i < 5; i++) {
                int lineY = scrollY + 20 + i * 20;
                DrawLine(scrollX + 10, lineY, scrollX + paperWidth - 10, lineY, GRAY);
            }
        }
        
    } else if (ctx->state == GAME_STATE_CLEAR) {
        DrawText("CLEAR!", 320, 180, 60, GOLD);
    } else {
        DrawText("TIME UP...", 280, 180, 60, RED);
    }
}

// ===============================================
// ゲームリスト（ここに新しいゲームを追加）
// ===============================================
MiniGame games[] = {
    {"Typing", TypingGame_Init, TypingGame_Update, TypingGame_Draw},
    {"Button Mash", ButtonMashGame_Init, ButtonMashGame_Update, ButtonMashGame_Draw},
    {"Coin Click", CoinClickGame_Init, CoinClickGame_Update, CoinClickGame_Draw},
    {"Hit The Target", TargetGame_Init, TargetGame_Update, TargetGame_Draw},
    {"Math Game", MathGame_Init, MathGame_Update, MathGame_Draw},
    {"Minesweeper", MinesweeperGame_Init, MinesweeperGame_Update, MinesweeperGame_Draw},
    {"Music Game", MusicGame_Init, MusicGame_Update, MusicGame_Draw},
    {"6-Key Rhythm", SixKeyRhythmGame_Init, SixKeyRhythmGame_Update, SixKeyRhythmGame_Draw},
    {"Memory Card", MemoryCardGame_Init, MemoryCardGame_Update, MemoryCardGame_Draw},
    {"Unroll Scroll", ScrollUnrollGame_Init, ScrollUnrollGame_Update, ScrollUnrollGame_Draw},
};

int gameCount = sizeof(games) / sizeof(games[0]);

// ===============================================
// メイン処理
// ===============================================
int main() {
    InitWindow(800, 450, "Made in Raylib");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitAudioDevice();
    SetMasterVolume(1.0f);
    SetTargetFPS(60);

    // RenderTextureの作成（仮想解像度）
    renderTexture = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(renderTexture.texture, TEXTURE_FILTER_BILINEAR);

    // リソース読み込み
    LoadGameResources();

    // ゲーム管理変数
    srand(time(NULL));
    MainState mainState = MAIN_STATE_TITLE;
    int currentGameIndex = 0;
    int score = 0;
    int lives = 3;
    bool showResult = false;
    float resultTimer = 0;
    bool showLevelUp = false;
    float levelUpTimer = 0;
    int previousLevel = 1;
    float musicPitch = 1.0f;
    bool isPaused = false;
    bool countdownPlayed[3] = {false, false, false}; // 3秒, 2秒, 1秒のカウント音再生済みフラグ

    MiniGameContext ctx;

    while (!WindowShouldClose()) {
        UpdateMusicStream(resources.bgm);
        
        // F11キーでフルスクリーントグル
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        
        // BGMがループした場合は再生を継続
        if (!IsMusicStreamPlaying(resources.bgm)) {
            PlayMusicStream(resources.bgm);
        }

        // タイトル画面
        if (mainState == MAIN_STATE_TITLE) {
            if (IsKeyPressed(KEY_SPACE)) {
                mainState = MAIN_STATE_PLAYING;
                score = 0;
                lives = 3;
                showResult = false;
                showLevelUp = false;
                currentGameIndex = rand() % gameCount;
                previousLevel = 1;
                musicPitch = 1.0f;
                SetMusicPitch(resources.bgm, musicPitch);
                
                ctx.level = 1;
                games[currentGameIndex].init(&ctx);
            }
        }
        // ゲームオーバー画面
        else if (mainState == MAIN_STATE_GAMEOVER) {
            if (IsKeyPressed(KEY_R)) {
                mainState = MAIN_STATE_TITLE;
            }
        }
        // ゲームプレイ中
        else if (mainState == MAIN_STATE_PLAYING) {
            // Pキーでポーズ切り替え（タイピングゲーム以外）
            if (IsKeyPressed(KEY_P) && currentGameIndex != 0) { // currentGameIndex 0がタイピングゲーム
                isPaused = !isPaused;
            }
            
            // ポーズ中は更新しない
            if (!isPaused) {
                // ライフがなくなったらゲームオーバー
                if (lives <= 0) {
                    mainState = MAIN_STATE_GAMEOVER;
                }
                // 結果表示中
                else if (showResult) {
                resultTimer -= GetFrameTime();
                if (resultTimer <= 0) {
                    showResult = false;
                    showLevelUp = false;

                    // スコアに基づいてレベル設定
                    if (score >= 70) ctx.level = 8;
                    else if (score >= 50) ctx.level = 7;
                    else if (score >= 30) ctx.level = 6;
                    else if (score >= 20) ctx.level = 5;
                    else if (score >= 10) ctx.level = 4;
                    else if (score >= 5) ctx.level = 3;
                    else if (score >= 3) ctx.level = 2;
                    else ctx.level = 1;

                    // カウントダウン音のフラグをリセット
                    countdownPlayed[0] = false;
                    countdownPlayed[1] = false;
                    countdownPlayed[2] = false;

                    games[currentGameIndex].init(&ctx);
                }
            } else {
                // ゲーム更新
                games[currentGameIndex].update(&ctx);

                // カウントダウン音の再生（3秒、2秒、1秒）
                if (ctx.state == GAME_STATE_PLAYING) {
                    if (ctx.timeLeft <= 3.0f && ctx.timeLeft > 2.9f && !countdownPlayed[0]) {
                        PlaySound(resources.fxCount);
                        countdownPlayed[0] = true;
                    } else if (ctx.timeLeft <= 2.0f && ctx.timeLeft > 1.9f && !countdownPlayed[1]) {
                        PlaySound(resources.fxCount);
                        countdownPlayed[1] = true;
                    } else if (ctx.timeLeft <= 1.0f && ctx.timeLeft > 0.9f && !countdownPlayed[2]) {
                        PlaySound(resources.fxCount);
                        countdownPlayed[2] = true;
                    }
                }

                // ゲーム終了判定
                if (ctx.state != GAME_STATE_PLAYING) {
                    showResult = true;
                    // ResetAudio(); // 音ズレ対策：デバイスリセット
                    resultTimer = 2.0f; // 2秒間結果を表示

                    if (ctx.state == GAME_STATE_CLEAR) {
                        score++;
                        
                        // レベルアップ判定（次のレベルを計算）
                        int newLevel = 1;
                        if (score >= 70) newLevel = 8;
                        else if (score >= 50) newLevel = 7;
                        else if (score >= 30) newLevel = 6;
                        else if (score >= 20) newLevel = 5;
                        else if (score >= 12) newLevel = 4;
                        else if (score >= 7) newLevel = 3;
                        else if (score >= 3) newLevel = 2;
                        else newLevel = 1;
                        
                        if (newLevel > previousLevel) {
                            showLevelUp = true;
                            previousLevel = newLevel;
                            // レベルアップごとにBGM速度を上げる
                            if (newLevel > 4 && newLevel < 7) musicPitch = 1.0f + (newLevel - 3) * 0.10f;
                            if (newLevel == 7) musicPitch = 1.5f;
                            if (newLevel == 8) musicPitch = 2.0f;
                            SetMusicPitch(resources.bgm, musicPitch);
                        }
                    } else {
                        lives--;
                    }

                    // 次のゲームを先に選択しておく
                    int nextGameIndex = rand() % gameCount;
                    if (gameCount > 1) {
                        while (nextGameIndex == currentGameIndex) {
                            nextGameIndex = rand() % gameCount;
                        }
                    }
                    currentGameIndex = nextGameIndex;
                }
            }
            } // isPaused check end
        }

        // 描画
        // スケーリング計算
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        float scaleX = (float)screenWidth / VIRTUAL_WIDTH;
        float scaleY = (float)screenHeight / VIRTUAL_HEIGHT;
        scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        offset.x = (screenWidth - (VIRTUAL_WIDTH * scale)) * 0.5f;
        offset.y = (screenHeight - (VIRTUAL_HEIGHT * scale)) * 0.5f;

        // 仮想解像度に描画
        BeginTextureMode(renderTexture);
            ClearBackground(RAYWHITE);

            // タイトル画面
            if (mainState == MAIN_STATE_TITLE) {
                DrawText("Made in Raylib", 200, 120, 60, DARKBLUE);
                DrawText("Press SPACE to Start", 260, 280, 25, GREEN);
                 DrawText("Music by OtoLogic", 200, 350, 20, GRAY);
            }
            // ゲームオーバー画面
            else if (mainState == MAIN_STATE_GAMEOVER) {
                DrawText("GAME OVER", 225, 150, 60, RED);
                DrawText(TextFormat("Final Score: %d", score), 285, 240, 30, DARKGRAY);
                DrawText("Press R to Title", 300, 300, 20, DARKGRAY);
            }
            // ゲームプレイ中
            else if (mainState == MAIN_STATE_PLAYING) {
                // ポーズ中の表示
                if (isPaused) {
                    DrawText("PAUSED", 300, 200, 60, DARKGRAY);
                    DrawText("Press P to Resume", 280, 280, 20, GRAY);
                } else {
                /*
                // リズムを刻むビートマーカー（画面上下）
                float beatTime = fmod(GetTime() * musicPitch * 1.35f, 1.0f);
                float beatIntensity = (beatTime < 0.15f) ? (1.0f - beatTime / 0.15f) : 0.0f;
                
                // ビートマーカーの高さ
                int barHeight = (int)(beatIntensity * 30);
                Color barColor = Fade(ORANGE, beatIntensity * 0.9f);
                
                // 上側のビートマーカー
                DrawRectangle(0, 0, 800, barHeight, barColor);
                
                // 下側のビートマーカー
                DrawRectangle(0, 450 - barHeight, 800, barHeight, barColor);
                
                // 左右にも薄く線を追加（オプション）
                Color sideColor = Fade(ORANGE, beatIntensity * 0.5f);
                DrawRectangle(0, 0, 3, 450, sideColor);
                DrawRectangle(797, 0, 3, 450, sideColor);
                */
                
                if (!showResult) {
                    // ゲーム名表示
                    DrawText(games[currentGameIndex].name, 10, 30, 20, DARKGRAY);
                    
                    // ゲーム描画
                    games[currentGameIndex].draw(&ctx);
                } else {
                    // 結果表示
                    if (ctx.state == GAME_STATE_CLEAR) {
                        DrawText("SUCCESS!", 260, 140, 60, GOLD);
                        
                        // レベルアップ時は追加表示
                        if (showLevelUp) {
                            DrawText("LEVEL UP!", 280, 210, 40, ORANGE);
                            
                            // 次のレベルを計算して表示
                            int nextLevel = 1;
                            if (score >= 70) nextLevel = 8;
                            else if (score >= 50) nextLevel = 7;
                            else if (score >= 30) nextLevel = 6;
                            else if (score >= 20) nextLevel = 5;
                            else if (score >= 10) nextLevel = 4;
                            else if (score >= 5) nextLevel = 3;
                            else if (score >= 3) nextLevel = 2;
                            
                            DrawText(TextFormat("Level %d", nextLevel), 340, 255, 35, GOLD);
                            
                            // キラキラエフェクト
                            for (int i = 0; i < 15; i++) {
                                int x = 250 + (i * 25) + (int)(sin(GetTime() * 5 + i) * 10);
                                int y = 180 + (int)(cos(GetTime() * 3 + i) * 20);
                                DrawCircle(x, y, 3, GOLD);
                            }
                        }
                        
                        DrawText(TextFormat("Next: %s", games[currentGameIndex].name), 290, 310, 25, DARKGRAY);
                    } else {
                        DrawText("FAILED!", 300, 180, 60, RED);
                        DrawText(TextFormat("Next: %s", games[currentGameIndex].name), 290, 310, 25, DARKGRAY);
                    }
                }
                
                // 残り時間表示（右上）
                Color timeColor = DARKGRAY;
                if (ctx.timeLeft < 1.0f) {
                    timeColor = RED;
                } else if (ctx.timeLeft < 3.0f) {
                    timeColor = ORANGE;
                }
                DrawText(TextFormat("%.2f", ctx.timeLeft), 725, 20, 30, timeColor);

                // スコアとライフ表示
                DrawText(TextFormat("Score: %d", score), 10, 420, 20, DARKGRAY);
                DrawText(TextFormat("Level: %d", ctx.level), 350, 420, 20, BLUE);
                
                // ビート計算（リズムに合わせた鼓動）
                float beatTime = fmod(GetTime() * musicPitch * 1.35f, 1.0f);
                float beatIntensity = (beatTime < 0.15f) ? (1.0f - beatTime / 0.15f) : 0.0f;
                float heartScale = 1.0f + beatIntensity * 0.15f; // 最大15%拡大
                
                // ライフをハートで表示（リズムに合わせて鼓動）
                for (int i = 0; i < 3; i++) {
                    int heartX = 640 + i * 50;
                    int heartY = 380;
                    
                    // ハートのサイズを取得
                    int width = resources.heartTexture.width;
                    int height = resources.heartTexture.height;
                    
                    // スケール適用後のサイズ
                    int scaledWidth = (int)(width * heartScale);
                    int scaledHeight = (int)(height * heartScale);
                    
                    // 中心を維持するためのオフセット
                    int offsetX = (scaledWidth - width) / 2;
                    int offsetY = (scaledHeight - height) / 2;
                    
                    Rectangle source = {0, 0, (float)width, (float)height};
                    Rectangle dest = {(float)(heartX - offsetX), (float)(heartY - offsetY), (float)scaledWidth, (float)scaledHeight};
                    Vector2 origin = {0, 0};
                    
                    if (i >= (3 - lives)) {
                        DrawTexturePro(resources.heartTexture, source, dest, origin, 0.0f, WHITE);
                    } else {
                        DrawTexturePro(resources.heartTexture, source, dest, origin, 0.0f, (Color){100, 100, 100, 100});
                    }
                }
                } // isPaused else block end
            }
        EndTextureMode();

        // 実画面に描画
        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle sourceRec = {0, 0, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT};
            Rectangle destRec = {offset.x, offset.y, VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale};
            Vector2 origin = {0, 0};
            DrawTexturePro(renderTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
        EndDrawing();
    }

    // 終了処理
    UnloadRenderTexture(renderTexture);
    // UnloadGameResources();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}