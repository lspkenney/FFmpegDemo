package com.lsp.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Toast;

import com.lsp.ffmpegdemo.util.VideoUtils;

import java.io.File;

/**
 * Created by Kenney on 2017-08-09 16:09
 */

public class DecodeVideoActivity extends AppCompatActivity {
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_decode);
    }

    public void decode(View view) {
        final String path = Environment.getExternalStorageDirectory().getAbsolutePath() + "/test.mp4";
        final String outputPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/test-420p.yuv";
        File file = new File(path);
        if(!file.exists()){
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return ;
        }
        new Thread(){
            @Override
            public void run() {
                VideoUtils.decode(path, outputPath);
            }
        }.start();
    }
}
