#include "com_lsp_ffmpegdemo_util_VideoUtils.h"
#include <android/log.h>
//使用这两个Window相关的头文件需要在CMake脚本中引入android库
#include <android/native_window_jni.h>
#include <android/native_window.h>
//编解码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
//yuv
//extern "C"{
#include "libyuv.h"
//}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"lsp",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"lsp",FORMAT,##__VA_ARGS__);


JNIEXPORT void JNICALL
Java_com_lsp_ffmpegdemo_util_VideoUtils_decode(JNIEnv *env, jclass type, jstring input,
                                               jstring output) {
    const char *c_input = (*env)->GetStringUTFChars(env, input, 0);
    const char *c_output = (*env)->GetStringUTFChars(env, output, 0);

    //1、注册所有组件，例如初始化一些全局的变量、初始化网络等等
    av_register_all();
    //封装格式上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    //2、打开输入视频文件
    if(avformat_open_input(&avFormatContext, c_input, NULL, NULL) != 0){
        LOGE("%s\n", "无法打开输入文件");
        return ;
    }

    //3、获取视频文件信息，例如得到视频的宽高
    if(avformat_find_stream_info(avFormatContext, NULL) < 0){
        LOGE("%s\n", "无法获取视频文件信息");
        return ;
    }

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    for(; i < avFormatContext->nb_streams; i++){
        //流的类型
        if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            v_stream_idx = i;
            break;
        }
    }

    if(-1 == v_stream_idx){
        LOGE("%s\n", "未找到视频流");
        return ;
    }

    //解码器上下文
    AVCodecContext *avCodecContext = avFormatContext->streams[v_stream_idx]->codec;
    //4、找到解码器
    //根据编解码上下文中的编码id查找对应的解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    if(avCodec == NULL){
        LOGE("%s\n", "未找到解码器");
        return ;
    }

    //5、打开解码器，解码器有问题（比如说我们编译FFmpeg的时候没有编译对应类型的解码器）
    if(avcodec_open2(avCodecContext,avCodec, NULL) < 0){
        LOGE("%s\n", "解码器无法打开");
        return ;
    }

    //输出视频信息
    LOGI("视频的文件格式：%s", avFormatContext->iformat->name);
    LOGI("视频时长：%lld", (avFormatContext->duration) / 1000000);
    LOGI("视频的宽高：%d,%d", avCodecContext->width, avCodecContext->height);
    LOGI("解码器的名称：%s", avCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *avPacket = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *avFrame = av_frame_alloc();

    //YUV420
    AVFrame *avFrameYUV = av_frame_alloc();

    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, avCodecContext->width, avCodecContext->height));
    //初始化缓冲区
    avpicture_fill((AVPicture *) avFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, avCodecContext->width,
                   avCodecContext->height);

    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
    struct SwsContext *sws_ctx = sws_getContext(avCodecContext->width, avCodecContext->height,
                                                avCodecContext->pix_fmt,
                             avCodecContext->width, avCodecContext->height,
                                                AV_PIX_FMT_YUV420P,
                                                SWS_BILINEAR, NULL, NULL, NULL);

    int got_picture, ret = 0;
    FILE *fp_yuv = fopen(c_output, "wb+");
    int frame_count = 0;
    int size = sizeof(char);
    LOGI("sizeof(char) = %d\n", size);
    //6、一帧一帧的读取压缩数据
    while(av_read_frame(avFormatContext, avPacket) >= 0){
        //只要视频压缩数据（根据流的索引位置判断）
        if(avPacket->stream_index == v_stream_idx){
            //7、解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(avCodecContext, avFrame, &got_picture, avPacket);
            if(ret < 0){
                LOGE("%s\n", "解码错误");
                return ;
            }

            //got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero
            //0解码完成、非0正在解码
            if(got_picture){
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                sws_scale(sws_ctx, avFrame->data,
                          avFrame->linesize, 0, avFrame->height, avFrameYUV->data, avFrameYUV->linesize);

                //输出到YUV文件
                //AVFrame像素帧写入文件
                //data解码后的图像像素数据（音频采样数据）
                //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
                //U V 个数是Y的1/4

                int y_size = avCodecContext->width * avCodecContext->height;
                fwrite(avFrameYUV->data[0], size,  y_size, fp_yuv);
                fwrite(avFrameYUV->data[1], size,  y_size / 4, fp_yuv);
                fwrite(avFrameYUV->data[2], size,  y_size / 4, fp_yuv);

                frame_count++;
                LOGI("第%d帧解码完成\n", frame_count);
            }
        }
        //释放资源
        av_free_packet(avPacket);
    }

    fclose(fp_yuv);
    //av_frame_free(&avFrameYUV);
    av_frame_free(&avFrame);
    avcodec_close(avCodecContext);
    //avformat_close_input(&avFormatContext);
    avformat_free_context(avFormatContext);

    (*env)->ReleaseStringUTFChars(env, input, c_input);
    (*env)->ReleaseStringUTFChars(env, output, c_output);
}

JNIEXPORT void JNICALL
Java_com_lsp_ffmpegdemo_util_VideoUtils_render(JNIEnv *env, jclass type, jstring input,
                                               jobject surface) {
    const char *c_input = (*env)->GetStringUTFChars(env, input, 0);

    //1、注册所有组件，例如初始化一些全局的变量、初始化网络等等
    av_register_all();
    //封装格式上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    //2、打开输入视频文件
    if(avformat_open_input(&avFormatContext, c_input, NULL, NULL) != 0){
        LOGE("%s\n", "无法打开输入文件");
        return ;
    }

    //3、获取视频文件信息，例如得到视频的宽高
    if(avformat_find_stream_info(avFormatContext, NULL) < 0){
        LOGE("%s\n", "无法获取视频文件信息");
        return ;
    }

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    for(; i < avFormatContext->nb_streams; i++){
        //流的类型
        if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            v_stream_idx = i;
            break;
        }
    }

    if(-1 == v_stream_idx){
        LOGE("%s\n", "未找到视频流");
        return ;
    }

    //解码器上下文
    AVCodecContext *avCodecContext = avFormatContext->streams[v_stream_idx]->codec;
    //4、找到解码器
    //根据编解码上下文中的编码id查找对应的解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    if(avCodec == NULL){
        LOGE("%s\n", "未找到解码器");
        return ;
    }

    //5、打开解码器，解码器有问题（比如说我们编译FFmpeg的时候没有编译对应类型的解码器）
    if(avcodec_open2(avCodecContext,avCodec, NULL) < 0){
        LOGE("%s\n", "解码器无法打开");
        return ;
    }

    //输出视频信息
    LOGI("视频的文件格式：%s", avFormatContext->iformat->name);
    LOGI("视频时长：%lld", (avFormatContext->duration) / 1000000);
    LOGI("视频的宽高：%d,%d", avCodecContext->width, avCodecContext->height);
    LOGI("解码器的名称：%s", avCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *avPacket = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //YUV420
    AVFrame *avFrameYUV = av_frame_alloc();
    //RGB Frame
    AVFrame * rgbFrame = av_frame_alloc();

    int got_picture, ret = 0;
    int frame_count = 0;
    int size = sizeof(char);

    //SurfaceView窗体
    ANativeWindow*  aNativeWindow = ANativeWindow_fromSurface(env,surface);
    //SurfaceView缓冲区
    ANativeWindow_Buffer out_buffer;

    //6、一帧一帧的读取压缩数据
    while(av_read_frame(avFormatContext, avPacket) >= 0){
        //只要视频压缩数据（根据流的索引位置判断）
        if(avPacket->stream_index == v_stream_idx){
            //7、解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(avCodecContext, avFrameYUV, &got_picture, avPacket);
            if(ret < 0){
                LOGE("%s\n", "解码错误");
                return ;
            }

            //got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero
            //0解码完成、非0正在解码
            if(got_picture){

                //a、lock window
                //设置缓冲区的属性：宽高、像素格式（需要与Java层的格式一致）
                ANativeWindow_setBuffersGeometry(aNativeWindow, avCodecContext->width, avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_lock(aNativeWindow, &out_buffer, NULL);

                //b、填充缓冲区
                //初始化缓冲区
                //设置属性，像素格式、宽高
                //rgb_frame的缓冲区就是Window的缓冲区，同一个，解锁的时候就会进行绘制
                //rgb_frame缓冲区与out_buffer.bits是同一块内存
                avpicture_fill((AVPicture*)rgbFrame, out_buffer.bits, AV_PIX_FMT_RGBA,
                               avCodecContext->width, avCodecContext->height);

                //YUV格式的数据转换成RGBA 8888格式的数据
                //FFmpeg可以转，但是会有问题，因此我们使用libyuv这个库来做
                //https://chromium.googlesource.com/external/libyuv

                //参数分别是数据、对应一行的大小
                I420ToARGB(avFrameYUV->data[0], avFrameYUV->linesize[0],
                           avFrameYUV->data[2], avFrameYUV->linesize[2],
                           avFrameYUV->data[1], avFrameYUV->linesize[1],
                           rgbFrame->data[0], rgbFrame->linesize[0],
                           avCodecContext->width, avCodecContext->height
                );


                //unlock window
                ANativeWindow_unlockAndPost(aNativeWindow);

                frame_count++;
                LOGI("解码绘制第%d帧完成\n", frame_count);
            }
        }
        //释放资源
        av_free_packet(avPacket);
    }

    ANativeWindow_release(aNativeWindow);
    av_frame_free(&avFrameYUV);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);

    (*env)->ReleaseStringUTFChars(env, input, c_input);
}