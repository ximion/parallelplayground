
add_executable(mklfqtransfer
    main.cpp
    ../proconhelper.h
    ../proconhelper.cpp
    ../barrier.h
    mklfqstream.h
    readerwriterqueue.h
    atomicops.h
)

target_link_libraries(mklfqtransfer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
    Qt5::Core
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
