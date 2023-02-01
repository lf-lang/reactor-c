set(HASHSET_SOURCES hashset.c hashset_itr.c)

list(APPEND INFO_SOURCES ${HASHSET_SOURCES})

list(TRANSFORM HASHSET_SOURCES PREPEND utils/hashset/)
target_sources(core PRIVATE ${HASHSET_SOURCES})
