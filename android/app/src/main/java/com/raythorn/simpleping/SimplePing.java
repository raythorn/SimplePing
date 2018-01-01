package com.raythorn.simpleping;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created by Derek Ray on 2017/12/30.
 */

public class SimplePing {

    // Used to load the 'SimplePing' library on application startup.
    static {
        System.loadLibrary("SimplePing");
    }

    private SimplePingDelegate delegate = null;
    private SimplePingResult result = new SimplePingResult();

    public final static int SPNS_ERROR          = -1;
    public final static int SPNS_DNSUNREACH     = -2;
    public final static int SPNS_HOSTUNREACH    = -3;
    public final static int SPNS_HOSTDOWN       = -4;
    public final static int SPNS_HOSTNOTFOUND   = -5;
    public final static int SPNS_NETUNREACH     = -6;
    public final static int SPNS_NETDOWN        = -7;
    public final static int SPNS_TIMEOUT        = -8;
    public final static int SPNS_NOMEM          = -9;
    public final static int SPNS_IO             = -10;
    public final static int SPNS_OK             = 0;
    public final static int SPNS_PINGINIT       = 1;
    public final static int SPNS_PING           = 2;
    public final static int SPNS_PINGSTATISTIC  = 3;
    public final static int SPNS_PINGRESULT     = 4;

    public void init(SimplePingDelegate delegate) {
        this.delegate = delegate;
    }

    public void simplePing(int state, String message) {

        if (state == SPNS_PINGRESULT) {
            result.parse(message);
        }

        if (null != delegate) {
            delegate.simplePing(state, message);
        }
    }

    public class SimplePingResult {
        public int state = 0;
        public int send = 0;
        public int recv = 0;
        public int maxrtt = 0;
        public int minrtt = 0;
        public int avgrtt = 0;
        public float loss = 0;

        public void parse(String result) {
            try {
                JSONObject json = new JSONObject(result);
                state = json.getInt("state");
                send = json.getInt("send");
                recv = json.getInt("recv");
                maxrtt = json.getInt("maxrtt");
                minrtt = json.getInt("minrtt");
                avgrtt = json.getInt("avgrtt");
                loss = (float) json.getDouble("loss");
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
    }

    public interface SimplePingDelegate {
        void simplePing(int state, String message);
    }

    /**
     * A native method that is implemented by the 'SimplePing' native library,
     * which is packaged with this application.
     */
    public native void ping(String host, int count);
}
