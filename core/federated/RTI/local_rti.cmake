# This adds the local RTI sources required for scheduling enclaves 
# to the build.
set(
    LOCAL_RTI_SOURCES
    rti_common.c
    rti_local.c
)

list(APPEND INFO_SOURCES ${LOCAL_RTI_SOURCES})

list(TRANSFORM LOCAL_RTI_SOURCES PREPEND federated/RTI/)
target_sources(core PRIVATE ${LOCAL_RTI_SOURCES})

