package com.lsp.ffmpegdemo;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;


public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    private Button btn_video_decode, btn_video_player, btn_audio,btn_sync;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initViews();
    }

    private void initViews() {
        btn_video_decode = (Button) findViewById(R.id.btn_video_decode);
        btn_video_player = (Button) findViewById(R.id.btn_video_player);
        btn_audio = (Button) findViewById(R.id.btn_audio);
        btn_sync = (Button) findViewById(R.id.btn_sync);

        btn_video_decode.setOnClickListener(this);
        btn_video_player.setOnClickListener(this);
        btn_audio.setOnClickListener(this);
        btn_sync.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_video_decode:
                startActivity(new Intent(this, DecodeVideoActivity.class));
                break;
            case R.id.btn_video_player:
                startActivity(new Intent(this, VideoPlayerActivity.class));
                break;
            case R.id.btn_sync:
                Intent intent = new Intent(this, VideoPlayerActivity.class);
                intent.putExtra("from", R.id.btn_sync);
                startActivity(intent);
                break;
            case R.id.btn_audio:
                startActivity(new Intent(this, AudioActivity.class));
                break;
        }
    }
}
