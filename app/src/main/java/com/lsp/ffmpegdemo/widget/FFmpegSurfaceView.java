package com.lsp.ffmpegdemo.widget;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by Kenney on 2017-08-09 10:46
 */

public class FFmpegSurfaceView extends SurfaceView {
    public FFmpegSurfaceView(Context context) {
        this(context, null);
    }

    public FFmpegSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public FFmpegSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        //初始化像素绘制的格式为RGBA_8888（色彩最丰富）
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }
}
