package com.raythorn.simpleping;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements SimplePing.SimplePingDelegate {
    SimplePing pinger = new SimplePing();

    EditText hostname;
    EditText count;
    Button ping;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        pinger.init(this);

        hostname = findViewById(R.id.host_edit);
        count = findViewById(R.id.count_edit);
        ping = findViewById(R.id.ping);
        ping.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String _hostname = hostname.getText().toString();
                int _count = Integer.parseInt(count.getText().toString());
                if (_hostname.isEmpty()) return;
                if (_count <= 0) _count = 4;
                pinger.ping(_hostname, _count);
            }
        });
    }

    @Override
    public void simplePing(int state, String message) {
        if (state >= SimplePing.SPNS_PINGINIT && state <= SimplePing.SPNS_PINGSTATISTIC) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {

                }
            });
        }
    }

    class SPRunnable implements Runnable {

        private String message;
        public SPRunnable(String message) {
            this.message = message;
        }

        @Override
        public void run() {
            TextView logview = findViewById(R.id.message);
            String text = logview.getText().toString();
            if (!text.isEmpty()) {
                text += "\n";
            }

            text += message;
            logview.setText(text);
        }
    }
}
