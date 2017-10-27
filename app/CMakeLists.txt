# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html


#指定cmake最低版本号
cmake_minimum_required(VERSION 3.4.1)
#配置工程路径
set(path_project F:/workspace/FFmpegDemo)
#配置头文件的包含路径
include_directories(${path_project}/app/src/main/cpp/ffmpeg/include)
include_directories(${path_project}/app/src/main/cpp/yuv/include)

#设置生成的so动态库最后输出的路径
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${path_project}/app/libs/${ANDROID_ABI})

#添加自己编写的动态库
add_library( ffmpeg-lib
             SHARED
             src/main/cpp/ffmpeg-video.c
             src/main/cpp/ffmpeg-audio.c
             src/main/cpp/ffmpeg-media.c
              )

#添加预编译的库
add_library(avutil
             SHARED
             IMPORTED)
#设置预编译的库的路径
set_target_properties(
                    avutil
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libavutil-54.so
                 )

add_library(swresample
             SHARED
             IMPORTED)
set_target_properties(
                    swresample
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libswresample-1.so
                 )

add_library(avcodec
             SHARED
             IMPORTED)
set_target_properties(
                    avcodec
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libavcodec-56.so
                 )

add_library(avformat
             SHARED
             IMPORTED)
set_target_properties(
                    avformat
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libavformat-56.so
                 )

add_library(swscale
             SHARED
             IMPORTED)
set_target_properties(
                    swscale
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libswscale-3.so
                 )

add_library(postproc
             SHARED
             IMPORTED)
set_target_properties(
                    postproc
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libpostproc-53.so
                 )

add_library(avfilter
             SHARED
             IMPORTED)
set_target_properties(
                    avfilter
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libavfilter-5.so
                 )

add_library(avdevice
             SHARED
             IMPORTED)
set_target_properties(
                    avdevice
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libavdevice-56.so
                 )

add_library(yuv
             SHARED
             IMPORTED)
set_target_properties(
                    yuv
                    PROPERTIES IMPORTED_LOCATION
                    ${path_project}/app/libs/${ANDROID_ABI}/libyuv.so
                 )


#查找系统日志动态库文件(这个库已经在Android平台中了)
find_library( log-lib
              log )
#查找系统android动态库文件(这个库已经在Android平台中了)
find_library( android-lib
              android )

# 把需要的库链接到自己的动态库中
target_link_libraries( ffmpeg-lib
                       ${log-lib}
                       ${android-lib}
                        avutil
                        swresample
                        avcodec
                        avformat
                        swscale
                        postproc
                        avfilter
                        avdevice
                        yuv
                        )