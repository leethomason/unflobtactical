/* San Angeles Observation OpenGL ES version example
 * Copyright 2009 The Android Open Source Project
 * All rights reserved.
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 */
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>
#include <string.h>

#include "../../../game/cgame.h"

int   gAppAlive   = 1;

static int  sDemoStopped  = 0;
static long sTimeOffset   = 0;
static int  sTimeOffsetInit = 0;
static long sTimeStopped  = 0;
static void* game = 0;

int androidResourceOffset;
int androidResourceLen;
char androidResourcePath[200];

static long
_getTime(void)
{
    struct timeval  now;

    gettimeofday(&now, NULL);
    return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativeResource( JNIEnv* env, jobject thiz, jstring _path, jlong offset, jlong len  )
{
	jboolean copy;
	const char* path = (*env)->GetStringUTFChars( env, _path, 0 );

	androidResourcePath[0] = 0;
	if ( strlen( path ) < 199 ) {
		strcpy( androidResourcePath, path );
	}
	androidResourceOffset = (int)offset;
	androidResourceLen = (int)len;

    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "Resource path=%s offset=%d len=%d", path, (int)offset, (int)len );
	(*env)->ReleaseStringUTFChars( env, _path, path );
}


/* Call to initialize the graphics state */
void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativeInit( JNIEnv*  env )
{
    importGLInit();

	// UFO attack doesn't handle resize after init well, so 
	// defer the resize until later.
	//game = NewGame( 320, 480, 1, ".\\" );

    gAppAlive    = 1;
    sDemoStopped = 0;
    sTimeOffsetInit = 0;
#if defined(DEBUG) || defined(_DEBUG)
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "NewGame DEBUG.");
#else
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "NewGame RELEASE.");
#endif
}

void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativeResize( JNIEnv*  env, jobject  thiz, jint w, jint h )
{
	if ( game == 0 )
		game = NewGame( w, h, 1, ".\\" );
	else
		GameResize( game, w, h, 1 );
    __android_log_print(ANDROID_LOG_INFO, "UFOAttack", "resize w=%d h=%d", w, h);
}

/* Call to finalize the graphics state */
void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativeDone( JNIEnv*  env )
{
	if ( game )
		DeleteGame( game );
	game = 0;
    importGLDeinit();
}

/* This is called to indicate to the render loop that it should
 * stop as soon as possible.
 */
void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativePause( JNIEnv*  env, jint paused )
{
    sDemoStopped = paused;
    if (sDemoStopped) {
        /* we paused the animation, so store the current
         * time in sTimeStopped for future nativeRender calls */
        sTimeStopped = _getTime();
    } else {
        /* we resumed the animation, so adjust the time offset
         * to take care of the pause interval. */
        sTimeOffset -= _getTime() - sTimeStopped;
		if ( game )
			GameDeviceLoss( game );
    }
}

/* Call to render the next GL frame */
void
Java_com_grinninglizard_UFOAttack_DemoRenderer_nativeRender( JNIEnv*  env )
{
    long   curTime;

    /* NOTE: if sDemoStopped is TRUE, then we re-render the same frame
     *       on each iteration.
     */
    if (sDemoStopped) {
        curTime = sTimeStopped + sTimeOffset;
    } else {
        curTime = _getTime() + sTimeOffset;
        if (sTimeOffsetInit == 0) {
            sTimeOffsetInit = 1;
            sTimeOffset     = -curTime;
            curTime         = 0;
        }
		GameDoTick( game, curTime );
    }
}
