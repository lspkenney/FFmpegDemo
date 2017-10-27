package com.lsp.ffmpegdemo.util;

import android.view.Surface;

/**
 * Created by Kenney on 2017-08-16 15:44
 */

public class MediaUtils {
    //音视频播放
    public native static void play(String inputFilePath, Surface surface);
}
