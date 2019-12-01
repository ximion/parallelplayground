
qt5_generate_repc(REP_INTF_S_SRC ../interface.rep SOURCE)

add_executable(qroworker
    main.cpp
    ${REP_INTF_S_SRC}
    ../../proconhelper.h
    ../../proconhelper.cpp
    ../cvmatshm.h
    ../cvmatshm.cpp
    worker.h
    worker.cpp
)

target_link_libraries(qroworker
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
    Qt5::Core
    Qt5::RemoteObjects
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
