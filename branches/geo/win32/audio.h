#ifndef UFOATTACK_AUDIO_INCLUDED
#define UFOATTACK_AUDIO_INCLUDED

#include <stdio.h>

void Audio_Init();
void Audio_PlayWav( const char* path, int offset, int size );
void Audio_Close();


#endif UFOATTACK_AUDIO_INCLUDED
