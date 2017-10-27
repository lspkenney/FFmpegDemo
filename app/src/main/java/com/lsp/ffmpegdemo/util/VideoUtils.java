package com.lsp.ffmpegdemo.util;

import android.view.Surface;

/**
 * Created by Kenney on 2017-08-08 09:20
 */

public class VideoUtils {
    //视频解码
    public native static void decode(String input, String output);

    //视频播放
    public native static void render(String inputFilePath, Surface surface);

}
