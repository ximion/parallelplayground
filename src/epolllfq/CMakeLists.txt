
add_executable(eplfqtransfer
    main.cpp
    ../proconhelper.h
    ../proconhelper.cpp
    eplfqstream.h
)

target_link_libraries(eplfqtransfer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
