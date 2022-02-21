
#define HASHMAP(token) hashmap_object2int ## _ ## token
#define K void*
#define V int
#define HASH_OF(key) (size_t) key
#include "hashmap.c"
#undef HASHMAP
#undef K
#undef V
#undef HASH_OF
