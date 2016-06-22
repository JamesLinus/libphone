package com.libphone;

import com.libphone.PhoneActivity;
import android.os.Handler;

public class PhoneNotifyThread extends Thread {
    public Handler handler;
    public PhoneActivity activity;
    private native int nativeInvokeNotifyThread();
    private int javaNotifyMainThread(long needNotifyMask) {
        final long finalNeedNotifyMask = needNotifyMask;
        PhoneActivity.PhoneNotifyRunnable runnable = activity.new PhoneNotifyRunnable();
        runnable.needNotifyMask = needNotifyMask;
        handler.post(runnable);
        return 0;
    }
    public void run() {
        nativeInvokeNotifyThread();
    }
}

