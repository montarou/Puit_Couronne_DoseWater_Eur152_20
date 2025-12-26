#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.16...3.27)
project(PuitsCouronne)

#----------------------------------------------------------------------------
# Find Geant4 package
find_package(Geant4 REQUIRED ui_all vis_all)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
add_executable(puits_couronne puits_couronne.cc ${sources} ${headers})
target_link_libraries(puits_couronne ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy all scripts to the build directory
set(PUITS_COURONNE_SCRIPTS
    init_vis.mac
    run.mac
    vis.mac
)

foreach(_script ${PUITS_COURONNE_SCRIPTS})
    configure_file(
        ${PROJECT_SOURCE_DIR}/${_script}
        ${PROJECT_BINARY_DIR}/${_script}
        COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
install(TARGETS puits_couronne DESTINATION bin)
