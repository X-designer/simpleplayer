#find_package(SDL2 CONFIG)

add_executable(hello
	./hello_main.cpp
	)

target_link_libraries(hello
	PUBLIC FFMPEG::avformat
	PUBLIC FFMPEG::avcodec
	PUBLIC SDL2::SDL2-static
	)

target_compile_definitions(hello
	PUBLIC SDL_MAIN_HANDLED
	)

add_executable(basic
	./basic.cpp
	)

target_link_libraries(basic
	PUBLIC FFMPEG::avformat
	PUBLIC FFMPEG::avcodec
	PUBLIC SDL2::SDL2-static
	)

target_compile_definitions(basic
	PUBLIC SDL_MAIN_HANDLED
	)

