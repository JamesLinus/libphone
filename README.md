Overview <img src="https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000" width="122" height="20"/> [![Build Status](https://travis-ci.org/huxingyi/libphone.svg?branch=master)](https://travis-ci.org/huxingyi/libphone)
-------------
libphone is a cross-platform library which supports iOS and Android development in C language, with a focus on a core set features.

Code once, target both.

Contributing
----------------
Don't hesitate to make a pull request.  All kinds of contribution are welcome.  
*Note: Please follow the coding style of the existed codes.*

Screenshots
-----------------
<a href="https://github.com/huxingyi/libphone/tree/master/samples/rpggame"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/rpggame/screenshots/mmorpg.gif" width="240" height="360"/></a>
<a href="https://github.com/huxingyi/libphone/tree/master/samples/rpggame"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/rpggame/screenshots/android.png" width="240" height="360"/></a>  

<a href="https://github.com/huxingyi/libphone/tree/master/samples/tableview"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/tableview/screenshots/ios.png" width="240" height="360"/></a>
<a href="https://github.com/huxingyi/libphone/tree/master/samples/tableview"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/tableview/screenshots/android.png" width="240" height="360"/></a>  

<a href="https://github.com/huxingyi/libphone/tree/master/samples/helloworld"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/helloworld/screenshots/ios.png" width="240" height="360"/></a>
<a href="https://github.com/huxingyi/libphone/tree/master/samples/helloworld"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/helloworld/screenshots/android.png" width="240" height="360"/></a>  

<a href="https://github.com/huxingyi/libphone/tree/master/samples/xmllayout"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/xmllayout/screenshots/ios.png" width="240" height="360"/></a>
<a href="https://github.com/huxingyi/libphone/tree/master/samples/xmllayout"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/xmllayout/screenshots/android.png" width="240" height="360"/></a>  

<a href="https://github.com/huxingyi/libphone/tree/master/samples/viewstack"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/viewstack/screenshots/ios.png" width="240" height="360"/></a>
<a href="https://github.com/huxingyi/libphone/tree/master/samples/viewstack"><img src="https://raw.githubusercontent.com/huxingyi/libphone/master/samples/viewstack/screenshots/android.png" width="240" height="360"/></a>  

Quick Start
-------------
libphone is not an IDE, so it's still a requirement that following the official guide to install `Xcode` or `Android Studio`(including the jni supports). After doing that, then
- iOS
  - Create a new Xcode project(Single View Application with all default options)
  - Remove all auto generated files except main.m and Info.plist
  - Replace the whole content inside `main.m` with the following:  

    ```objc
    #import "libphone.h"

    @interface iOSAppDelegate : phoneAppDelegate
    @end

    @implementation iOSAppDelegate
    @end

    int main(int argc, char * argv[]) {
      @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([iOSAppDelegate class]));
      }
    }
    ```
  - Remove the `Main storyboard file base name` key in Info.plist
  - Drag the following files to Xcode:

    ```
    src/libphone.c
    src/ios/iosphone.m
    samples/helloworld/helloworld.c
    ```
  - Change Build Settings
    - Header Search Paths `<REPLACE WITH REAL PATH>/libphone/include <REPLACE WITH REAL PATH>/libphone/src`
  - Run and and enjoy!

- Android
  - Create a new project(With Empty Activity and all default options)
  - Modify `classpath` inside build.grade (Project: &lt;YOUR PROJECT NAME&gt;)
    ```
    classpath 'com.android.tools.build:gradle-experimental:0.7.2'
    ```
  - Modify build.grade (Module: app) according to the following example:
    ```gradle
    apply plugin: 'com.android.model.application'

    model {
        android {
            compileSdkVersion 23
            buildToolsVersion "23.0.3"

            defaultConfig {
                applicationId "com.example.jeremy.test"
                minSdkVersion.apiLevel 9
                targetSdkVersion.apiLevel 23
                versionCode 1
                versionName "1.0"
            }
            buildTypes {
                release {
                    minifyEnabled false
                    //proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
                    proguardFiles.add(file('proguard-android.txt'))
                }
            }
        }

        android.ndk {
            moduleName "libphone"
            CFlags.add("-I/Users/jeremy/Repositories/libphone/include")
            CFlags.add("-I/Users/jeremy/Repositories/libphone/src")
            ldLibs.add("log")
            ldLibs.add("GLESv2")
            ldLibs.add("android")
        }

        android.sources.main {
            java.source.srcDirs = ["src/main/java", "/Users/jeremy/Repositories/libphone/src/android/java"]
            jni.source.srcDirs = ["/Users/jeremy/Repositories/libphone/src/android/jni",
                                  "/Users/jeremy/Repositories/libphone/src",
                                  "/Users/jeremy/Repositories/libphone/samples/helloworld"]
        }
    }

    dependencies {
        compile fileTree(dir: 'libs', include: ['*.jar'])
        testCompile 'junit:junit:4.12'
        compile 'com.android.support:appcompat-v7:23.4.0'
    }
    ```
  - Modify MainActivity.java  

    ```java
    package <YOUR PACKET NAME>;

    import android.support.v7.app.AppCompatActivity;
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
    ```
  - Run 'app' and have fun!

Supported platforms
-------------------
- Android 2.3+
- iOS 5.0+

Features
-------------
- View
  - Corner Radius
  - Border
  - Shadow
  - Animation
  - Alpha
  - Rotate
  - Pull to Refresh  
  - OpenGL ES 2.0  
- Work Queue  
- Timer  
- Native Thread  
- Asset Reader  

Tools
-----------
**tools/phoneres.py**  
Generate different kinds of specific image resources which required by both platforms automatically.

**tools/phonelayout.py**   
Generate C codes for view layout based on xml  

Licensing
-----------------
libphone is licensed under the MIT license.  
