package com.lsp.ffmpegdemo;

import android.app.Application;

/**
 * Created by Kenney on 2017-08-10 14:45
 */

public class MyApp extends Application {
    static {
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avformat-56");
        System.loadLibrary("swscale-3");
        System.loadLibrary("postproc-53");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("yuv");
        System.loadLibrary("ffmpeg-lib");
    }
}
