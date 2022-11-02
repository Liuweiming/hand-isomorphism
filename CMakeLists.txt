set(ISO_CLIB_FILES
    src/deck.h
    src/hand_index.h
    src/deck.c 
    src/hand_index.c
) 

add_library(hand_iso_clib OBJECT ${ISO_CLIB_FILES} )
set_target_properties(hand_iso_clib PROPERTIES POSITION_INDEPENDENT_CODE ON)
