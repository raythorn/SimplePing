package com.raythorn.simpleping;

/**
 * Created by Derek Ray on 2017/12/30.
 */

public class SimplePing {

    // Used to load the 'SimplePing' library on application startup.
    static {
        System.loadLibrary("SimplePing");
    }

    /**
     * A native method that is implemented by the 'SimplePing' native library,
     * which is packaged with this application.
     */
    public native void ping(String host, int count, int timeout);
    private static SimplePingDelegate delegate = null;
    public static void init(SimplePingDelegate delegate) {
        SimplePing.delegate = delegate;
    }

    public static void simplePingMessage(int state, String message) {
        if (null != delegate) {
            delegate.simplePingMessage(state, message);
        }
    }

    public static void simplePingResult() {
        if (null != delegate) {
            delegate.simplePingResult();
        }
    }

    public static void simplePingFail(int code, String error) {
        if (null != delegate) {
            delegate.simplePingFail(code, error);
        }
    }

    public interface SimplePingDelegate {
        void simplePingMessage(int state, String message);
        void simplePingResult();
        void simplePingFail(int code, String error);
    }
}
