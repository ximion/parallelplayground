
find_package(Qt5RemoteObjects)

qt5_generate_repc(REP_INTF_R_SRC interface.rep REPLICA)

add_executable(qrotransfer
    main.cpp
    ${REP_INTF_R_SRC}
    ../proconhelper.h
    ../proconhelper.cpp
    ../barrier.h
    cvmatshm.h
    cvmatshm.cpp
    qrostream.h
    readerwriterqueue.h
    atomicops.h
    workerconnector.h
    workerconnector.cpp
)

target_link_libraries(qrotransfer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
    Qt5::Core
    Qt5::RemoteObjects
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_subdirectory(worker/)
