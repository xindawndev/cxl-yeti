#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "MiniArch.h"
#include "Test.h"

void * TskChain;
void * Timer;

DWORD WINAPI ILibPoolThread( void *args )
{
    getchar();
    ILibStopChain( TskChain );
    return( 0 );
}

void onTimer( void * args )
{
    printf( "Timers...\n" );
    ILibLifeTime_Add( Timer, NULL, 1, onTimer, NULL );
}

int main( int argc, char ** argv )
{
    DWORD ptid = 0;

    TskChain = ILibCreateChain();

    createTest( TskChain );

    CreateThread( NULL,0,&ILibPoolThread,NULL,0,&ptid );

    Timer = ILibCreateLifeTime( TskChain );
    ILibLifeTime_Add( Timer, NULL, 1, onTimer, NULL );

    ILibStartChain( TskChain );

    return 0;
}