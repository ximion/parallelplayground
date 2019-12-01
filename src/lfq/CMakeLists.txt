
add_executable(lfqtransfer
    main.cpp
    ../proconhelper.h
    ../proconhelper.cpp
    lfqstream.h
    ProducerConsumerQueue.h
)

target_link_libraries(lfqtransfer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
