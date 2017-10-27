#include "com_lsp_ffmpegdemo_util_AudioUtils.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//重采样
#include "libswresample/swresample.h"

//日志
#include <android/log.h>
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"lsp",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"lsp",FORMAT,##__VA_ARGS__);

//48000 * 4
#define MAX_AUDIO_FRME_SIZE 48000 * 4

JNIEXPORT void JNICALL Java_com_lsp_ffmpegdemo_util_AudioUtils_decodeAudio
        (JNIEnv *env, jclass cls, jstring jinput, jstring joutput){
    const char* cinput = (*env)->GetStringUTFChars(env, jinput, NULL);
    const char* coutput = (*env)->GetStringUTFChars(env, joutput, NULL);

    //注册组件
    av_register_all();

    //音视频格式上下文
    AVFormatContext* pFormatContext = avformat_alloc_context();

    //打开音频文件
    if(avformat_open_input(&pFormatContext, cinput, NULL, NULL) != 0){
        LOGE("%s\n", "无法打开音频文件");
        return ;
    }

    //获取文件输入信息
    if(avformat_find_stream_info(pFormatContext, NULL) < 0){
        LOGE("%s\n", "无法获取音频文件信息");
        return ;
    }

    //获取音频流索引位置
    int audio_stream_index = -1;
    int i = 0;
    for(; i < pFormatContext->nb_streams; i++){
        if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
            break;
        }
    }

    if(-1 == audio_stream_index){
        LOGE("%s\n", "获取音频流索引失败");
        return ;
    }

    //解码器上下文
    AVCodecContext *pCodecContext = pFormatContext->streams[audio_stream_index]->codec;
    //获取音频解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecContext->codec_id);

    if(NULL == pCodec){
        LOGE("%s\n", "获取音频解码器失败");
        return ;
    }

    //打开解码器
    if(avcodec_open2(pCodecContext, pCodec, NULL) < 0){
        LOGE("%s\n", "打开解码器失败");
        return ;
    }

    //压缩数据包
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //解压缩数据帧
    AVFrame *frame = av_frame_alloc();

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
    int out_sample_rate = 44100;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    uint64_t in_ch_layout = pCodecContext->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(pswrContext, out_ch_layout, out_sample_fmt,
    out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate,0, NULL);

    swr_init(pswrContext);
    //------------------------------------------------------
    //输出的声道个数
    int out_ch_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //16bit 44100 PCM 数据
    uint8_t *outBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

    FILE *fp_pcm = fopen(coutput, "wb+");
    int got_frame = 0, count = 0, len;
    //读取压缩数据
    while (av_read_frame(pFormatContext, packet) >= 0){
        //解码音频packet
        if(packet->stream_index == audio_stream_index){
            //解码
            len = avcodec_decode_audio4(pCodecContext, frame, &got_frame, packet);
            if(len < 0){
                LOGE("%s\n", "解码失败");
                return ;
            }

            if(got_frame > 0){
                count++;
                LOGI("解码第%d帧\n", count);
                swr_convert(pswrContext, &outBuffer, MAX_AUDIO_FRME_SIZE, frame->data, frame->nb_samples);
                //获取sample的size
                int outBufferSize = av_samples_get_buffer_size(NULL, out_ch_nb, frame->nb_samples, out_sample_fmt, 1);
                fwrite(outBuffer, sizeof(uint8_t), outBufferSize, fp_pcm);
            }
        }
        av_free_packet(packet);

    }

    fclose(fp_pcm);
    av_frame_free(&frame);
    av_free(outBuffer);

    swr_free(&pswrContext);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);

    (*env)->ReleaseStringUTFChars(env, jinput, cinput);
    (*env)->ReleaseStringUTFChars(env, joutput, coutput);
    LOGI("解码完成\n");
}


JNIEXPORT void JNICALL Java_com_lsp_ffmpegdemo_util_AudioUtils_play
        (JNIEnv *env , jclass clazz, jstring jinput){
    const char* cinput = (*env)->GetStringUTFChars(env, jinput, NULL);

    //注册组件
    av_register_all();

    //音视频格式上下文
    AVFormatContext* pFormatContext = avformat_alloc_context();

    //打开音频文件
    if(avformat_open_input(&pFormatContext, cinput, NULL, NULL) != 0){
        LOGE("%s\n", "无法打开音频文件");
        return ;
    }

    //获取文件输入信息
    if(avformat_find_stream_info(pFormatContext, NULL) < 0){
        LOGE("%s\n", "无法获取音频文件信息");
        return ;
    }

    //获取音频流索引位置
    int audio_stream_index = -1;
    int i = 0;
    for(; i < pFormatContext->nb_streams; i++){
        if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
            break;
        }
    }

    if(-1 == audio_stream_index){
        LOGE("%s\n", "获取音频流索引失败");
        return ;
    }

    //解码器上下文
    AVCodecContext *pCodecContext = pFormatContext->streams[audio_stream_index]->codec;
    //获取音频解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecContext->codec_id);

    if(NULL == pCodec){
        LOGE("%s\n", "获取音频解码器失败");
        return ;
    }

    //打开解码器
    if(avcodec_open2(pCodecContext, pCodec, NULL) < 0){
        LOGE("%s\n", "打开解码器失败");
        return ;
    }

    //压缩数据包
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //解压缩数据帧
    AVFrame *frame = av_frame_alloc();

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


    //调用Java方法获取AudioTrack对象
    jmethodID constructor_mid = (*env)->GetMethodID(env, clazz, "<init>", "()V");
    jobject jobj = (*env)->NewObject(env, clazz, constructor_mid);
    jmethodID createAudioTrack_mid = (*env)->GetMethodID(env, clazz, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    jobject audioTrack = (*env)->CallObjectMethod(env, jobj, createAudioTrack_mid, out_sample_rate, out_ch_nb);

    //获取AudioTrack的play、stop、write方法
    jclass audio_track_clazz = (*env)->GetObjectClass(env, audioTrack);
    jmethodID play_mid = (*env)->GetMethodID(env, audio_track_clazz, "play", "()V");
    jmethodID write_mid = (*env)->GetMethodID(env, audio_track_clazz, "write", "([BII)I");
    jmethodID stop_mid = (*env)->GetMethodID(env, audio_track_clazz, "stop", "()V");

    //调用AudioTrack的play方法
    (*env)->CallVoidMethod(env, audioTrack, play_mid);



    //16bit 44100 PCM 数据
    uint8_t *outBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

    int got_frame = 0, count = 0, len;
    //读取压缩数据
    while (av_read_frame(pFormatContext, packet) >= 0){
        //解码音频类型的Packet
        if(packet->stream_index == audio_stream_index){
            //解码
            len = avcodec_decode_audio4(pCodecContext, frame, &got_frame, packet);
            if(len < 0){
                LOGE("%s\n", "解码失败");
                return ;
            }

            if(got_frame > 0){
                count++;
                LOGI("解码第%d帧\n", count);
                swr_convert(pswrContext, &outBuffer, MAX_AUDIO_FRME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);
                //获取sample的size
                int outBufferSize = av_samples_get_buffer_size(NULL, out_ch_nb, frame->nb_samples, out_sample_fmt, 1);

                //outBuffer缓冲区数据转成byte数组
                jbyteArray  audio_sample_array = (*env)->NewByteArray(env, outBufferSize);
                jbyte *p_sample_array = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
                //outBuffer数据复制到p_sample_array
                memcpy(p_sample_array, outBuffer, outBufferSize);
                //同步
                (*env)->ReleaseByteArrayElements(env, audio_sample_array, p_sample_array, 0);

                //调用AudioTrack的write方法
                (*env)->CallIntMethod(env, audioTrack, write_mid, audio_sample_array, 0, outBufferSize);

                //释放局部引用
                (*env)->DeleteLocalRef(env, audio_sample_array);
            }
        }
        av_free_packet(packet);

    }

    //调用AudioTrack的stop方法
    (*env)->CallVoidMethod(env, audioTrack, stop_mid);

    av_frame_free(&frame);
    av_free(outBuffer);

    swr_free(&pswrContext);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);

    (*env)->ReleaseStringUTFChars(env, jinput, cinput);
    LOGI("解码完成\n");
}
