#include "com_lsp_ffmpegdemo_util_MediaUtils.h"
#include <android/log.h>
#include <unistd.h>
//使用这两个Window相关的头文件需要在CMake脚本中引入android库
#include <android/native_window_jni.h>
#include <android/native_window.h>
//编解码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
//posix线程
#include <pthread.h>
//yuv
//extern "C"{
#include "libyuv.h"
//}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"lsp",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"lsp",FORMAT,##__VA_ARGS__);

//定义媒体流最大值，现解析视频、音频流所以定义为2，如果还要解析字幕则定义为3，以此类推
#define MAX_STREAM_TYPE 2
#define MAX_AUDIO_FRME_SIZE 48000 * 4

struct Player{
    //封装格式上下文
    AVFormatContext *avFormatContext;
    //视频流索引
    int video_stream_index;
    //音频流索引
    int audio_stream_index;
    //解码器上下文数组
    AVCodecContext *aVCodecContexts[MAX_STREAM_TYPE];
    //解码线程数组
    pthread_t decodeThreads[MAX_STREAM_TYPE];
    //绘图窗口
    ANativeWindow*  aNativeWindow;
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt;
    //输出的采样格式
    enum AVSampleFormat out_sample_fmt;
    //输入采样率
    int in_sample_rate;
    //输出采样率
    int out_sample_rate;
    //输出的声道个数
    int out_ch_nb;
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *pswrContext;

    //JNI
    JavaVM *javaVM;
    jobject audioTrack;
    jmethodID audioTrack_write_mid;
    jmethodID audioTrack_stop_mid;
};

//初始化AVFormatContext上下文,获取音视频流的索引位置
void init_avformat_context(struct Player *player, const char* c_input){
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

    //获取音视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到音视频流
    int i = 0;
    for(; i < avFormatContext->nb_streams; i++){
        //流的类型
        if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            player->video_stream_index = i;
        }else if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            player->audio_stream_index = i;
        }
    }

    player->avFormatContext = avFormatContext;
}

//初始化音视频解码器的上下文
void init_avcodec_context(struct Player *player, int stream_index){
    //解码器上下文
    AVCodecContext *avCodecContext = player->avFormatContext->streams[stream_index]->codec;
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
    player->aVCodecContexts[stream_index] = avCodecContext;
}

//视频解码准备
void decode_video_prepare(JNIEnv* env, struct Player *player, jobject surface){
    //SurfaceView窗体
    player->aNativeWindow = ANativeWindow_fromSurface(env,surface);
}

//视频解码
void decode_video(struct Player *player, AVPacket *avPacket){
    //AVFrame用于存储解码后的像素数据(YUV)
    //YUV420
    AVFrame *avFrameYUV = av_frame_alloc();
    //RGB Frame
    AVFrame * rgbFrame = av_frame_alloc();

    int got_picture, ret = 0;
    //int frame_count = 0;
    //int size = sizeof(char);

    //SurfaceView缓冲区
    ANativeWindow_Buffer out_buffer;

    AVCodecContext *avCodecContext = player->aVCodecContexts[player->video_stream_index];
    //ANativeWindow *aNativeWindow = player->aNativeWindow;

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
        ANativeWindow_setBuffersGeometry(player->aNativeWindow, avCodecContext->width, avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
        ANativeWindow_lock(player->aNativeWindow, &out_buffer, NULL);

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
        ANativeWindow_unlockAndPost(player->aNativeWindow);

        //sleep(1000 * 16);

        //frame_count++;
        //LOGI("解码绘制第%d帧完成\n", frame_count);
        av_frame_free(&avFrameYUV);
        av_frame_free(&rgbFrame);
        //avcodec_close(avCodecContext);
        //avformat_free_context(avFormatContext);
    }
}


void decode_audio(struct Player* player, AVPacket *avPacket){
    //16bit 44100 PCM 数据
    uint8_t *outBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
    AVCodecContext *pCodecContext = player->aVCodecContexts[player->audio_stream_index];

    //解压缩数据帧
    AVFrame *frame = av_frame_alloc();

    int got_frame = 0,  len;
    //解码
    len = avcodec_decode_audio4(pCodecContext, frame, &got_frame, avPacket);
    if(len < 0){
        LOGE("%s\n", "解码失败");
        return ;
    }

    if(got_frame > 0){
        JNIEnv *env;
        JavaVM *javaVM = player->javaVM;
        (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
        swr_convert(player->pswrContext, &outBuffer, MAX_AUDIO_FRME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);
        //获取sample的size
        int outBufferSize = av_samples_get_buffer_size(NULL, player->out_ch_nb, frame->nb_samples, player->out_sample_fmt, 1);

        //outBuffer缓冲区数据转成byte数组
        jbyteArray  audio_sample_array = (*env)->NewByteArray(env, outBufferSize);
        jbyte *p_sample_array = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
        //outBuffer数据复制到p_sample_array
        memcpy(p_sample_array, outBuffer, outBufferSize);
        //同步
        (*env)->ReleaseByteArrayElements(env, audio_sample_array, p_sample_array, 0);

        //调用AudioTrack的write方法
        (*env)->CallIntMethod(env, player->audioTrack, player->audioTrack_write_mid, audio_sample_array, 0, outBufferSize);

        //释放局部引用
        (*env)->DeleteLocalRef(env, audio_sample_array);
        (*javaVM)->DetachCurrentThread(javaVM);
    }

    //释放资源
    av_free(outBuffer);
    av_frame_free(&frame);
}

void decode_audio_prepare(struct Player* player){
    AVCodecContext *pCodecContext = player->aVCodecContexts[player->audio_stream_index];
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *pswrContext = swr_alloc();

    //重采样参数设置-------------------------------
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    //输出的采样格式 16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodecContext->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    uint64_t in_ch_layout = pCodecContext->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(pswrContext, out_ch_layout, out_sample_fmt,
                       out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate,0, NULL);

    swr_init(pswrContext);
    //输出的声道个数
    int out_ch_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    //------------------------------------------------------

    player->in_sample_fmt = in_sample_fmt;
    player->out_sample_fmt = out_sample_fmt;
    player->in_sample_rate = in_sample_rate;
    player->out_sample_rate = out_sample_rate;
    player->out_ch_nb = out_ch_nb;
    player->pswrContext = pswrContext;

}

void jni_audio_prepare(JNIEnv *env, struct Player* player){
    jclass clazz = (*env)->FindClass(env, "com/lsp/ffmpegdemo/util/AudioUtils");
    //调用Java方法获取AudioTrack对象
    jmethodID constructor_mid = (*env)->GetMethodID(env, clazz, "<init>", "()V");
    jobject jobj = (*env)->NewObject(env, clazz, constructor_mid);
    jmethodID createAudioTrack_mid = (*env)->GetMethodID(env, clazz, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    jobject audioTrack = (*env)->CallObjectMethod(env, jobj, createAudioTrack_mid, player->out_sample_rate, player->out_ch_nb);

    //获取AudioTrack的play、stop、write方法
    jclass audio_track_clazz = (*env)->GetObjectClass(env, audioTrack);
    jmethodID play_mid = (*env)->GetMethodID(env, audio_track_clazz, "play", "()V");
    jmethodID write_mid = (*env)->GetMethodID(env, audio_track_clazz, "write", "([BII)I");
    jmethodID stop_mid = (*env)->GetMethodID(env, audio_track_clazz, "stop", "()V");

    //调用AudioTrack的play方法
    (*env)->CallVoidMethod(env, audioTrack, play_mid);

    //player->audioTrack = (*env)->NewGlobalRef(env, audioTrack);
    player->audioTrack = audioTrack;
    player->audioTrack_write_mid = write_mid;
    player->audioTrack_stop_mid = stop_mid;
}


//解码
void* decode_data(void* arg){
    struct Player *player = (struct Player *)arg;
    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *avPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFormatContext* avFormatContext = player->avFormatContext;

    int video_frame_count = 0;

    //6、一帧一帧的读取压缩数据
    while(av_read_frame(avFormatContext, avPacket) >= 0){
        //只要视频压缩数据（根据流的索引位置判断）
        if(avPacket->stream_index == player->video_stream_index){
            //decode_video(player, avPacket);
            //LOGI("####解码视频流第%d帧", video_frame_count++);
        }else if(avPacket->stream_index == player->audio_stream_index ){
            decode_audio(player, avPacket);
        }
        //释放资源
        av_free_packet(avPacket);
    }
}




JNIEXPORT void JNICALL
Java_com_lsp_ffmpegdemo_util_MediaUtils_play(JNIEnv *env, jclass type, jstring input,
                                               jobject surface) {
    const char *c_input = (*env)->GetStringUTFChars(env, input, NULL);
    struct Player *player = (struct Player*)malloc(sizeof(struct Player));
    (*env)->GetJavaVM(env, &(player->javaVM));

    //初始化AVFormatContext
    init_avformat_context(player, c_input);
    //获取音视频流索引
    int video_stream_index = player->video_stream_index;
    int audio_stream_index = player->audio_stream_index;

    //初始化视频解码器上下文
    init_avcodec_context(player, video_stream_index);
    //初始化音频解码器上下文
    init_avcodec_context(player, audio_stream_index);

    //视频解码准备
    decode_video_prepare(env, player, surface);
    decode_audio_prepare(player);
    //JNI
    jni_audio_prepare(env, player);

    //开启线程进行视频解码
    //pthread_create(&(player->decodeThreads[video_stream_index]), NULL, decode_data, (void*)player);
    pthread_create(&(player->decodeThreads[audio_stream_index]), NULL, decode_data, (void*)player);

    //ANativeWindow_release(aNativeWindow);
    //free(player);
    //(*env)->ReleaseStringUTFChars(env, input, c_input);
}