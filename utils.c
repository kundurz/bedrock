/*
    Determines the fast chunk size class 
    a size request would correspond to. 
    
    AVAILABLE SIZE CLASSES:
    16 B 
    32 B 
    64 B 
    128 B 
    256 B 
    512 B 
    1024 B
    2048 B

    Return value:
    Returns the size class or -1. 
    -1 indicates that the requested chunk is actually a large
    chunk.
*/
int determine_size_class(int size) {
    for (unsigned int i = 16; i <= 2048; i = i * 2) {
        size = size & ~i;
        if (size == 0) return i; 
        if (size < i) return i * 2;
    }

    return -1; 
}

int get_fast_chunk_index(int size_class) {
    switch(size_class) {
        case 16:
            return 0;
        case 32: 
            return 1;
        case 64:
            return 2;
        case 128:
            return 3;
        case 256:
            return 4;
        case 512:
            return 5;
        case 1024:
            return 6;
        case 2048:
            return 7;
        default:
            return -1; // Something went wrong.
    }
}