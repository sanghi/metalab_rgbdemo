IF (NESTK_USE_OPENNI)

   ADD_EXECUTABLE(grab-mesh grab_mesh.cpp)
   TARGET_LINK_LIBRARIES(grab-mesh nestk)

ENDIF(NESTK_USE_OPENNI)

IF (NESTK_USE_PCL)

    ADD_EXECUTABLE(table-top-object table_top_object.cpp)
    TARGET_LINK_LIBRARIES(table-top-object nestk)
ENDIF()
