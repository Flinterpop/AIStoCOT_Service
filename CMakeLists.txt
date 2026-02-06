cmake_minimum_required(VERSION 3.20...4.0) # FetchContent is available in 3.11+

project(MyMongooseApp CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

#set(CMAKE_SYSTEM_VERSION 10.0) # Targets Windows 10
#set(WINDOW_BIT x64) # arm, arm64, x64, x86

add_definitions(-DUNICODE -D_UNICODE)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

option (BUILD_SHARED_LIBRARY OFF)

# statically link MSVC runtime in thrift to being fully self-contained.
# this increases .exe size but guarantees working on all windows versions
# see https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")



# Add Mongoose source and include paths (adjust paths)
set(MONGOOSE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mongoose/mongoose.c)
set(MONGOOSE_INC ${CMAKE_CURRENT_SOURCE_DIR}/mongoose)

# Create a static library for Mongoose
add_library(mongoose_lib STATIC ${MONGOOSE_SRC})
target_include_directories(mongoose_lib PUBLIC ${MONGOOSE_INC})



set(
    COT_list 

    src_COT/bg_TakMessage.cpp
    src_COT/COTSender.cpp
)

# Add COT source and include paths (adjust paths)
#set(COT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src_COT/${COT_list})
set(COT_INC ${CMAKE_CURRENT_SOURCE_DIR}/src_COT)

# Create a static library for COT
add_library(COT_lib STATIC ${COT_list})
target_include_directories(COT_lib PUBLIC ${COT_INC})


set(
    AISLIB_list 
    	
	aislib/ais.cpp
	aislib/ais_bitset.cpp
	aislib/ais1_2_3.cpp
	aislib/ais4_11.cpp
	aislib/ais5.cpp
	aislib/ais6.cpp
	aislib/ais7_13.cpp
	aislib/ais8.cpp
	aislib/ais8_1_22.cpp
	aislib/ais8_1_26.cpp
	aislib/ais8_200.cpp
	aislib/ais8_366.cpp
	aislib/ais8_366_22.cpp
	aislib/ais8_367.cpp
	aislib/ais9.cpp
	aislib/ais10.cpp
	aislib/ais12.cpp
	aislib/ais14.cpp
	aislib/ais15.cpp
	aislib/ais16.cpp
	aislib/ais17.cpp
	aislib/ais18.cpp
	aislib/ais19.cpp
	aislib/ais20.cpp
	aislib/ais21.cpp
	aislib/ais22.cpp
	aislib/ais23.cpp
	aislib/ais24.cpp
	aislib/ais25.cpp
	aislib/ais26.cpp
	aislib/ais27.cpp
	aislib/decode_body.cpp
	aislib/vdm.cpp
)

# Add COT source and include paths (adjust paths)
#set(COT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src_COT/${COT_list})
set(AISLIB_INC ${CMAKE_CURRENT_SOURCE_DIR}/aislib)

# Create a static library for COT
add_library(AISLIB STATIC ${AISLIB_list})
target_include_directories(AISLIB PUBLIC ${AISLIB_INC})




set(
    AISCOT_list 

	src_AISCOT/AISParser.cpp
	src_AISCOT/AISParser.h 
	src_AISCOT/AIStoCOT.h
	
)

# Add COT source and include paths (adjust paths)
#set(COT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src_AIS2COT/${COT_list})
set(AISCOT_INC ${CMAKE_CURRENT_SOURCE_DIR}/src_AISCOT)

# Create a static library for COT
add_library(AISCOT STATIC ${AISCOT_list})
target_include_directories(AISCOT PUBLIC ${AISCOT_INC})
target_include_directories(AISCOT PUBLIC ${AISLIB_INC})



set(
    source_list 

    hasMain.cpp
    src_base/BG_SocketBase.h
    src_base/BG_SocketBase.cpp
)


set_property(GLOBAL PROPERTY USE_FOLDERS ON) 


#this organizes the VS project folder like the source tree folder
foreach(source IN LISTS source_list)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()


set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )



add_executable(${PROJECT_NAME} 
  ${CPP_HEADER_FILES}
  ${CPP_SOURCE_FILES}
  ${source_list}
)
 

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/RunFolder")


target_include_directories(${PROJECT_NAME}  PUBLIC src_base)
#target_include_directories(${PROJECT_NAME}  PUBLIC src_app)



# Link Mongoose library to your app
target_link_libraries(${PROJECT_NAME} PRIVATE mongoose_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE COT_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE AISLIB)
target_link_libraries(${PROJECT_NAME} PRIVATE AISCOT)















