#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>
#include <string.h>

#include "../../../game/cgame.h"

int   gAppAlive   = 1;

static long lastClockTime = 0;
static long gameTime = 0;
#define MAX_TIME_DELTA 100

static void* game = 0;

enum { UFO_MAX_PATH = 200 };

int androidResourceOffset;
int androidResourceLen;
char androidResourcePath[UFO_MAX_PATH];
char androidSavePath[UFO_MAX_PATH];

extern void GrinlizSetReleaseAssertPath( const char* path );

static long _getTime(void)
{
    struct timeval  now;

    gettimeofday(&now, NULL);
    return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

// May get called before anything else is initialized. Not safe to call into the game object.
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeResource( JNIEnv* env, jobject thiz, jstring _path, jlong offset, jlong len  )
{
	const char* path = (*env)->GetStringUTFChars( env, _path, 0 );

	androidResourcePath[0] = 0;
	if ( strlen( path ) < (UFO_MAX_PATH-1) ) {
		strcpy( androidResourcePath, path );
	}
	androidResourceOffset = (int)offset;
	androidResourceLen = (int)len;

    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "Resource path=%s offset=%d len=%d", androidResourcePath, (int)offset, (int)len );
	(*env)->ReleaseStringUTFChars( env, _path, path );
}


// May get called before anything else is initialized. Not safe to call into the game object.
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeSavePath( JNIEnv* env, jobject thiz, jstring _path )
{
	const char* path = (*env)->GetStringUTFChars( env, _path, 0 );

	androidSavePath[0] = 0;
	if ( strlen( path ) < (UFO_MAX_PATH-1) ) {
		strcpy( androidSavePath, path );
	}

    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "Save path=%s", androidSavePath );
	GrinlizSetReleaseAssertPath( androidSavePath );
	(*env)->ReleaseStringUTFChars( env, _path, path );
}


/* Call to initialize the graphics state */
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeInit( JNIEnv*  env )
{
    //importGLInit();

	// UFO attack doesn't handle resize after init well, so 
	// defer the resize until later.
	// However, if the game has been created, this is context loss.
	if ( game ) {
		GameDeviceLoss( game );
	}

    gAppAlive    = 1;
    //sDemoStopped = 0;
    //sTimeOffsetInit = 0;
#if defined(DEBUG) || defined(_DEBUG)
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "NewGame DEBUG.");
#else
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "NewGame RELEASE.");
#endif
}

void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeResize( JNIEnv*  env, jobject  thiz, jint w, jint h )
{
	if ( game == 0 ) {
		game = NewGame( w, h, 1, androidSavePath );
	}
	else {
		//GameDeviceLoss( game );
		GameResize( game, w, h, 1 );
	}
	
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "resize w=%d h=%d", w, h);
}

/* Call to finalize the graphics state */
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeDone( JNIEnv*  env )
{
	if ( game )
		DeleteGame( game );
	game = 0;
    //importGLDeinit();
}

/* This is called to indicate to the render loop that it should
 * stop as soon as possible.
 */
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativePause( JNIEnv* env, jobject thiz, jint paused )
{
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "nativePause=%s", paused ? "paused" : "resume" );

    if ( paused ) {
        // Pause
        //sTimeStopped = _getTime();
		if ( game )
			GameSave( game );
    } else {
        // Wake up
        //sTimeOffset -= _getTime() - sTimeStopped;
		if ( game )
			GameDeviceLoss( game );
    }
}

/* Call to render the next GL frame */
void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeRender( JNIEnv*  env )
{
    long currentClockTime;
	long delta;

    /* NOTE: if sDemoStopped is TRUE, then we re-render the same frame
     *       on each iteration.
     */
    //if (sDemoStopped) {
    //    curTime = sTimeStopped + sTimeOffset;
    //} else {
    //   curTime = _getTime() + sTimeOffset;
    //    if (sTimeOffsetInit == 0) {
    //       sTimeOffsetInit = 1;
    //        sTimeOffset     = -curTime;
    //        curTime         = 0;
    //    }
		
	currentClockTime = _getTime();
	
	delta = currentClockTime - lastClockTime;
	if ( delta > MAX_TIME_DELTA ) {
		delta = MAX_TIME_DELTA;
	}
	
	gameTime += delta;
	lastClockTime = currentClockTime;

	GameDoTick( game, gameTime );
}


void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeTap( JNIEnv* env, jobject thiz, jint action, jfloat x, jfloat y )
{
    //__android_log_print(ANDROID_LOG_INFO, "UFOAttack", "nativeTap action=%d %d,%d", (int)action, (int)x, (int)y );
	if ( game ) {
		GameTap( game, action, (int)x, (int)y );
	}
}


void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeZoom( JNIEnv* env, jobject thiz, jint style, jfloat distance )
{
    //__android_log_print(ANDROID_LOG_INFO, "UFOAttack", "nativeTap action=%d %d,%d", (int)action, (int)x, (int)y );
	if ( game ) {
		GameZoom( game, style, distance );
	}
}


void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeRotate( JNIEnv* env, jobject thiz, jfloat delta )
{
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "nativeRotate delta=%f", delta );
	if ( game ) {
		GameCameraRotate( game, delta );
	}
}


void
Java_com_grinninglizard_UFOAttack_UFORenderer_nativeHotKey( JNIEnv* env, jobject thiz, jint key )
{
	if ( game ) {
		GameHotKey( game, key );
	}
}
