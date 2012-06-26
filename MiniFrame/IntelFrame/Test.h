#include "MiniArch.h"

struct _tTestStruct
{
    ILibChain_PreSelect preStart;
    ILibChain_PostSelect start;
    ILibChain_Destroy destory;
};

void PreStart(void* object,void *readset, void *writeset, void *errorset, int* blocktime)
{
    printf( "PreStart: %d\n", *blocktime );
}

void Start(void* object,int slct, void *readset, void *writeset, void *errorset)
{
    printf( "Start\n" );
}

void Destory(void* object)
{
    printf( "Destory\n" );
}

void * createTest( void * chain )
{
    struct _tTestStruct * retVal = ( struct _tTestStruct * )malloc( sizeof( struct _tTestStruct ) );

    if ( !retVal ) return NULL;

    retVal->preStart = PreStart;
    retVal->start = Start;
    retVal->destory = Destory;

    ILibAddToChain( chain, ( void * )retVal );

    return ( void * )retVal;
}