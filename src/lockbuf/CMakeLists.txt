
add_executable(lockbuftransfer
    main.cpp
    ../proconhelper.h
    ../proconhelper.cpp
    lockstream.h
)

target_link_libraries(lockbuftransfer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
