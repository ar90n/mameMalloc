#ifndef MAME_MALLOC_H
# define MAME_MALLOC_H

# include<stdbool.h>

# define MAME_MALLOC_ENABLE_ASSERTION
# define MAME_MALLOC_ENABLE_PROFILE

# ifdef __cplusplus
extern "C" {
# endif

void mameMalloc_initialize( void* pool, size_t size, int block_width );
void mameMalloc_finalize();
void* malloc( size_t size );
void free( void* p );

typedef enum {
    MAME_MALLOC_UNINITIALIZED = 0,
    MAME_MALLOC_INITIALIZED,
} mameMalloc_state;

typedef struct mameMalloc_header{
    struct mameMalloc_header* next;
    bool is_used;
} mameMalloc_header;

typedef struct {
    mameMalloc_state state;

    mameMalloc_header* pool_head;
    mameMalloc_header* pool_tail;
    int block_width;
} mameMalloc_environment;

static inline int mameMalloc_calcRequiredBlocks( size_t size );
static inline int mameMalloc_getFreeBlocks( mameMalloc_header* header );
static inline void* mameMalloc_calcPtrToNextNode( mameMalloc_header* current_node, int current_blocks );
static inline void mameMalloc_printProfile();

# ifdef MAME_MALLOC_ENABLE_ASSERTION
#  include<assert.h>
#  define MAME_MALLOC_ASSERT( X ) do{ assert( (X) );}while(0)
# else
#  define MAME_MALLOC_ASSERT( X )
# endif

# ifdef MAME_MALLOC_ENABLE_PROFILE
#  include<stdio.h>
static int used_blocks = 0;
#  define MAME_MALLOC_ADD_used_blocks( X )  \
    do { \
        used_blocks += (X); \
        size_t whole_memory_size = (size_t)mameMalloc_env->pool_tail - (size_t)mameMalloc_env->pool_head; \
        int whole_blocks = whole_memory_size >> mameMalloc_env->block_width; \
        printf("Allocate %d blocks\n Total block usage : %d / %d\n", (X), used_blocks, whole_blocks ); \
    }while(0)
#  define MAME_MALLOC_SUB_used_blocks( X )  \
    do{ \
        used_blocks -= (X); \
        size_t whole_memory_size = (size_t)mameMalloc_env->pool_tail - (size_t)mameMalloc_env->pool_head; \
        int whole_blocks = whole_memory_size >> mameMalloc_env->block_width; \
        printf("Free %d blocks\n Total block usage : %d / %d\n", (X), used_blocks, whole_blocks ); \
    }while(0)
# else
#  define MAME_MALLOC_ADD_used_blocks( X )
#  define MAME_MALLOC_SUB_used_blocks( X )
# endif

static mameMalloc_environment mameMalloc_env[1];

void mameMalloc_initialize( void* pool, size_t pool_size, int block_width )
{
    MAME_MALLOC_ASSERT( mameMalloc_env->state == MAME_MALLOC_UNINITIALIZED );
    MAME_MALLOC_ASSERT( pool != NULL );
    MAME_MALLOC_ASSERT( 0 < pool_size );
    MAME_MALLOC_ASSERT( 0 < block_width );

    int whole_blocks = pool_size >> mameMalloc_env->block_width;

    mameMalloc_env->state = MAME_MALLOC_INITIALIZED;
    mameMalloc_env->pool_head = pool;
    mameMalloc_env->pool_tail = mameMalloc_calcPtrToNextNode( pool,  whole_blocks);
    mameMalloc_env->block_width = block_width;

    mameMalloc_env->pool_head = mameMalloc_env->pool_head;
    mameMalloc_env->pool_head->next = mameMalloc_env->pool_tail;
    mameMalloc_env->pool_head->is_used = false;
}

void mameMalloc_finalize()
{
    mameMalloc_env->state = MAME_MALLOC_UNINITIALIZED;

    mameMalloc_env->pool_head = NULL;
    mameMalloc_env->pool_tail = NULL;
    mameMalloc_env->block_width = 0;
}

void* malloc( size_t required_size )
{
    MAME_MALLOC_ASSERT( mameMalloc_env->state == MAME_MALLOC_INITIALIZED );
    MAME_MALLOC_ASSERT( 0 < required_size );

    int required_blocks = mameMalloc_calcRequiredBlocks( required_size );
    MAME_MALLOC_ASSERT( 0 < required_blocks );

    mameMalloc_header* current_node = mameMalloc_env->pool_head;
    while( current_node < mameMalloc_env->pool_tail )
    {
        int current_blocks = mameMalloc_getFreeBlocks( current_node );
        if( required_blocks <= current_blocks )
        {
            MAME_MALLOC_ADD_used_blocks( required_blocks );

            mameMalloc_header* new_free_node_header = mameMalloc_calcPtrToNextNode( current_node, required_blocks );
            if( new_free_node_header < mameMalloc_env->pool_tail )
            {
                new_free_node_header->next = current_node->next;
                new_free_node_header->is_used = false;
                current_node->next = new_free_node_header;
            }

            mameMalloc_header* new_used_node_header = current_node;
            new_used_node_header->is_used = true;
            return new_used_node_header + 1;
        }

        current_node = current_node->next;
    }

    return NULL;
}

void free( void* p )
{
    MAME_MALLOC_ASSERT( mameMalloc_env->state == MAME_MALLOC_INITIALIZED );

    mameMalloc_header* request_free_node = ( ( mameMalloc_header* ) p ) - 1;
    request_free_node->is_used = false;

    mameMalloc_header* prev_request_free_node = mameMalloc_env->pool_head;
    while( prev_request_free_node->next == request_free_node )
    {
        prev_request_free_node = prev_request_free_node->next;
    }

    MAME_MALLOC_SUB_used_blocks( mameMalloc_getFreeBlocks( request_free_node ) );

    if( !request_free_node->next->is_used )
    {
        request_free_node->next = request_free_node->next->next;
    }

    if( !prev_request_free_node->is_used )
    {
        prev_request_free_node->next = request_free_node->next;
    }
}

static inline int mameMalloc_calcRequiredBlocks( size_t size )
{
    return ( ( size + sizeof( mameMalloc_header ) - 1 ) >> mameMalloc_env->block_width ) + 1;
}

static inline int mameMalloc_getFreeBlocks( mameMalloc_header* header )
{
    if ( header->is_used ) {
        return 0;
    }

    size_t node_size = ( (size_t)header->next - (size_t)header );
    return ( ( node_size - 1 ) >> mameMalloc_env->block_width ) + 1;
}

static inline void* mameMalloc_calcPtrToNextNode( mameMalloc_header* current_node, int current_blocks )
{
    size_t node_size = current_blocks << mameMalloc_env->block_width;
    return (void*)( (size_t)current_node +  node_size );
}

# ifdef __cplusplus
}
# endif

#  undef MAME_MALLOC_ASSERT
#  undef MAME_MALLOC_ADD_used_blocks
#  undef MAME_MALLOC_SUB_used_blocks

# undef MAME_MALLOC_ENABLE_PROFILE
# undef MAME_MALLOC_ENABLE_ASSERTION

#endif // MAME_MALLOC_H
