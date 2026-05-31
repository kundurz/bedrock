#include <stdint.h>

/* Structs for chunks and metadata  */
struct fast_chunk
{
    uint16_t size_class;
    struct fast_chunk *fd;
    char *user_data;
};

struct large_chunk
{
    uint64_t size_class;
    struct large_chunk *fd;
    struct large_chunk *bk;
    char *user_data;
};
