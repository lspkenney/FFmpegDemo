package com.lsp.ffmpegdemo.util;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Created by Kenney on 2017-08-10 15:39
 */

public class AudioUtils {
    public static native void decodeAudio(String inputFile, String outputFile);
    public static native void play(String inputFile);

    /**
     * 创建AudioTrack用于播放音频
     * @param sampleRateInHz
     * @param nb_channels
     * @return
     */
    public AudioTrack createAudioTrack(int sampleRateInHz, int nb_channels){
        //固定格式的音频码流
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        Log.i("lsp", "nb_channels = " + nb_channels);
        //声道布局
        int channelConfig;
        if(nb_channels == 1){
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        }else if(nb_channels == 2){
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }else{
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);

        AudioTrack audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRateInHz,
                channelConfig,
                audioFormat,
                bufferSizeInBytes,
                AudioTrack.MODE_STREAM
        );

        //播放
        //audioTrack.play();
        //写入PCM
        //audioTrack.write(audioData, offsetInBytes, sizeInBytes);
        //播放完调用stop即可
        //audioTrack.stop();

        return audioTrack;
    }
}
