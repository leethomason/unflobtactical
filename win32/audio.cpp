#include "audio.h"
#include "SDL.h "
#include <stdlib.h>
#include "../grinliz/gldebug.h"


SDL_AudioSpec audioSpec;
bool audioOpen = false;


#define NUM_SOUNDS 4

struct Sample {
    Uint8 *data;		// allocated by LoadWAV
    int		pos;
    int		len;
} sounds[NUM_SOUNDS];


void MixAudio(void *unused, Uint8 *stream, int len)
{
    for ( int i=0; i<NUM_SOUNDS; ++i ) {
        int amount = (sounds[i].len-sounds[i].pos);
        if ( amount > len ) {
            amount = len;
        }
		if ( amount > 0 ) {
	        SDL_MixAudio(	stream, 
							sounds[i].data + sounds[i].pos,
							amount, 
							80 );	//SDL_MIX_MAXVOLUME);
			sounds[i].pos += amount;
		}
    }
}


void Audio_Init()
{
	memset( sounds, 0, sizeof(Sample)*NUM_SOUNDS );

    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S16;
    audioSpec.channels = 2;
    audioSpec.samples = 2048;
    audioSpec.callback = MixAudio;
    audioSpec.userdata = NULL;

    if ( SDL_OpenAudio(&audioSpec, 0) < 0 ) {
		return;
    }
	audioOpen = true;
    SDL_PauseAudio(0);
}


void Audio_Close()
{
	if ( !audioOpen )
		return;

	SDL_PauseAudio(1);
	SDL_CloseAudio();
	for( int i=0; i<NUM_SOUNDS; ++i ){
		if ( sounds[i].data ) {
			free( sounds[i].data );
		}
	}
}


void Audio_PlayWav( const void* mem, int size )
{
	if ( !audioOpen )
		return;

    SDL_AudioSpec wave;
    Uint8 *data;
    Uint32 len;
    SDL_AudioCVT cvt;

    int index;

	SDL_LockAudio();
	{
		for ( index=0; index<NUM_SOUNDS; ++index ) {
			if ( sounds[index].pos == sounds[index].len ) {
				break;
			}
		}
		if ( index < NUM_SOUNDS ) {
		    if ( sounds[index].data ) {
		        free(sounds[index].data);
			}
			sounds[index].data = 0;
			sounds[index].len = 0;
			sounds[index].pos = 0;
		}
	}
    SDL_UnlockAudio();

    if ( index == NUM_SOUNDS )
        return;	// no empty slot

	SDL_RWops* fp = SDL_RWFromConstMem( mem, size );
	if ( !SDL_LoadWAV_RW( fp, 0, &wave, &data, &len ) )
	{
		SDL_FreeRW( fp );
		GLASSERT( 0 );
		return;
	}

    SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
							audioSpec.format, audioSpec.channels, audioSpec.freq );

	cvt.buf = (Uint8*)malloc(len*cvt.len_mult);
    memcpy(cvt.buf, data, len);
    cvt.len = len;
    SDL_ConvertAudio(&cvt);
    SDL_FreeWAV(data);
	SDL_FreeRW( fp );

	SDL_LockAudio();
	{
	    sounds[index].data = cvt.buf;
		sounds[index].len = cvt.len_cvt;
		sounds[index].pos = 0;
	}
    SDL_UnlockAudio();
}
