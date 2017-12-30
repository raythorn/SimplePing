package com.raythorn.simpleping;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements SimplePing.SimplePingDelegate {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SimplePing.init(this);
    }

    @Override
    public void simplePingMessage(int state, String message) {

    }

    @Override
    public void simplePingResult() {

    }

    @Override
    public void simplePingFail(int code, String error) {

    }
}
