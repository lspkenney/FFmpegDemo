package com.lsp.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Toast;

import com.lsp.ffmpegdemo.util.MediaUtils;
import com.lsp.ffmpegdemo.util.VideoUtils;
import com.lsp.ffmpegdemo.widget.FFmpegSurfaceView;

import java.io.File;

/**
 * Created by Kenney on 2017-08-09 16:10
 */

public class VideoPlayerActivity extends AppCompatActivity {
    private int from;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_player);

        from = getIntent().getIntExtra("from", -1);
    }

    public void mPlay(View view) {
        if(R.id.btn_sync == from){
            final String path = Environment.getExternalStorageDirectory().getAbsolutePath() + "/test.mp4";
            File file = new File(path);
            if(!file.exists()) {
                Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
                return;
            }
            FFmpegSurfaceView mSurfaceView = (FFmpegSurfaceView) findViewById(R.id.mSurfaceView);
            MediaUtils.play(path, mSurfaceView.getHolder().getSurface());
        }else{
            final String path = Environment.getExternalStorageDirectory().getAbsolutePath() + "/test.mp4";
            File file = new File(path);
            if(!file.exists()) {
                Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
                return;
            }
            FFmpegSurfaceView mSurfaceView = (FFmpegSurfaceView) findViewById(R.id.mSurfaceView);
            VideoUtils.render(path, mSurfaceView.getHolder().getSurface());
        }

    }
}
