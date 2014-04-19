#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "mameMalloc.h"

void test_mame_malloc()
{
    uint8_t pool[ 8192 ];
    mameMalloc_initialize( (void*)pool, 8192, 3 );

    void* p[16];
    int slot_index;
    for( slot_index = 0; slot_index < 7; slot_index++ )
    {
        p[ slot_index ] = malloc( 1024 );
        assert( p[ slot_index ] );
    }

    p[ slot_index ] = malloc( 896 );
    assert(p[ slot_index ]);
    slot_index++;

    p[ slot_index ] = malloc( 1 );
    assert(p[ slot_index ] == NULL);
    slot_index++;

    free( p[ 5 ] );
    p[ 5 ] = NULL;

    while( slot_index < 16 )
    {
        p[ slot_index ] = malloc( 100 );
        assert( p[ slot_index ] );
        slot_index++;
    }

    for( int i = 0; i < 16; i++ )
    {
        if( p[ i ] )
        {
            free( p[ i ] );
        }
    }
    
    mameMalloc_finalize();
}

int main(int argc, const char *argv[])
{
    test_mame_malloc();

    return 0;
}
