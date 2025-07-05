#include <jni.h>
#include <memory>
#include "Game.h"
#include "Renderer.h"
#include "AndroidOut.h"

// Using a static pointer to the game and renderer instances.
// This is okay for a simple app where there's only one game instance.
static std::unique_ptr<Game> g_game;
static std::unique_ptr<Renderer> g_renderer;

void checkAndPlayLockSound(JNIEnv* env, jobject thiz) {
    if (g_game && g_game->wasPieceLocked()) {
        jclass mainActivityClass = env->GetObjectClass(thiz);
        jmethodID playLockSoundMethod = env->GetMethodID(mainActivityClass, "playLockSound", "()V");
        if (playLockSoundMethod != nullptr) {
            env->CallVoidMethod(thiz, playLockSoundMethod);
        }
        g_game->clearPieceLockedFlag();
    }
}

void checkAndPlayLineClearSound(JNIEnv* env, jobject thiz) {
    if (g_game && g_game->wereLinesCleared()) {
        jclass mainActivityClass = env->GetObjectClass(thiz);
        jmethodID playLineClearSoundMethod = env->GetMethodID(mainActivityClass, "playLineClearSound", "()V");
        if (playLineClearSoundMethod != nullptr) {
            env->CallVoidMethod(thiz, playLineClearSoundMethod);
        }
        g_game->clearLinesClearedFlag();
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeOnCreate(JNIEnv *env, jobject thiz) {
    aout << "nativeOnCreate" << std::endl;
    g_game = std::make_unique<Game>();
    g_renderer = std::make_unique<Renderer>();
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeOnSurfaceCreated(JNIEnv *env, jobject thiz) {
    aout << "nativeOnSurfaceCreated" << std::endl;
    if (g_renderer) {
        g_renderer->initRenderer();
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeOnSurfaceChanged(JNIEnv *env, jobject thiz, jint width, jint height) {
    aout << "nativeOnSurfaceChanged" << std::endl;
    if (g_renderer) {
        g_renderer->updateRenderArea(width, height);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeOnDrawFrame(JNIEnv *env, jobject thiz) {
    if (g_renderer && g_game) {
        // For now, game logic is not tied to a timer, so we don't call g_game->update() here.
        // It will be driven by user input.
        g_renderer->render(*g_game);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeOnDestroy(JNIEnv *env, jobject thiz) {
    aout << "nativeOnDestroy" << std::endl;
    g_renderer.reset();
    g_game.reset();
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeUpdate(JNIEnv *env, jobject thiz) {
    if (g_game) {
        g_game->update(); // Call game update for automatic dropping
        checkAndPlayLockSound(env, thiz);
        checkAndPlayLineClearSound(env, thiz);
    }
}

// --- Input Functions ---

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeMove(JNIEnv *env, jobject thiz, jint direction) {
    if (g_game) {
        g_game->move(direction);
        checkAndPlayLockSound(env, thiz);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeRotate(JNIEnv *env, jobject thiz) {
    if (g_game) {
        g_game->rotate();
        checkAndPlayLockSound(env, thiz);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeRotateLeft(JNIEnv *env, jobject thiz) {
    if (g_game) {
        g_game->rotateLeft();
        checkAndPlayLockSound(env, thiz);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeSoftDrop(JNIEnv *env, jobject thiz) {
     if (g_game) {
        g_game->softDrop();
        checkAndPlayLockSound(env, thiz);
        checkAndPlayLineClearSound(env, thiz);
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeHardDrop(JNIEnv *env, jobject thiz) {
     if (g_game) {
        g_game->hardDrop();
        checkAndPlayLockSound(env, thiz);
        checkAndPlayLineClearSound(env, thiz);

        // Vibrate on hard drop
        jclass mainActivityClass = env->GetObjectClass(thiz);
        jmethodID vibrateMethod = env->GetMethodID(mainActivityClass, "vibrate", "(J)V");
        if (vibrateMethod != nullptr) {
            env->CallVoidMethod(thiz, vibrateMethod, 50L); // 50ms vibration
        }
    }
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeHold(JNIEnv *env, jobject thiz) {
     if (g_game) {
        g_game->hold();
    }
}

// Game state functions
JNIEXPORT jint JNICALL
Java_com_example_palibrix_MainActivity_nativeGetScore(JNIEnv *env, jobject thiz) {
    if (g_game) {
        return g_game->getScore();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_example_palibrix_MainActivity_nativeGetLines(JNIEnv *env, jobject thiz) {
    if (g_game) {
        return g_game->getLines();
    }
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_example_palibrix_MainActivity_nativeIsGameOver(JNIEnv *env, jobject thiz) {
    if (g_game) {
        return g_game->isGameOver();
    }
    return false;
}

JNIEXPORT void JNICALL
Java_com_example_palibrix_MainActivity_nativeReset(JNIEnv *env, jobject thiz) {
    if (g_game) {
        g_game->reset();
    }
}

// TODO: Add a function to get game state (score, lines, etc.) to display on the UI

} // extern "C" 