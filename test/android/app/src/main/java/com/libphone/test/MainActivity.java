package com.libphone.test;

import android.os.Bundle;
import com.libphone.PhoneActivity;

public class MainActivity extends PhoneActivity {
    static {
        System.loadLibrary("libphone");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
