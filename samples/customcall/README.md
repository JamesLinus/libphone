```java
package com.example.jeremy.test;

import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import com.libphone.PhoneActivity;

public class MainActivity extends PhoneActivity {
    static {
        System.loadLibrary("libphone");
    }

    public native int customNativeInit();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        customNativeInit();
    }

    public String customJavaGetSystemVersionDisplayText() {
        return Build.VERSION.RELEASE;
    }
}
```

```objc
#import "libphone.h"

@interface iOSAppDelegate : phoneAppDelegate
@end

@implementation iOSAppDelegate
@end

int customGetSystemVersionDisplayText(char *buf, int bufSize) {
  NSString *version = [[UIDevice currentDevice] systemVersion];
  return phoneCopyString(buf, bufSize, [version UTF8String]);
}

int main(int argc, char * argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([iOSAppDelegate class]));
  }
}
```
