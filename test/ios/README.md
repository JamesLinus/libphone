Generating the libphonetest.pbxproj
------------
```sh
$ cd test/ios
$ python createtestproject.py
```
See details please move to wiki [Create XCode Project Using Python](https://github.com/huxingyi/libphone/wiki/Create-XCode-Project-Using-Python)  

Building
---------
```sh
$ xcodebuild -project libphonetest.xcodeproj -target test -sdk iphonesimulator ONLY_ACTIVE_ARCH=NO
```

Running
-------------
```sh
$ xcrun simctl list
$ open -a Simulator --args -CurrentDeviceUDID <DEVIE UDID>
#; My <DEVIE UDID> is 3883F70B-AB98-492A-8FF7-504A53289AEF
$ xcrun simctl install booted /Users/jeremy/Repositories/libphone/test/ios/build/Release-iphonesimulator/test.app
$ xcrun simctl launch booted libphone.test
```

Checking Log of App
---------------
```sh
$ tail -f ~/Library/Logs/CoreSimulator/<DEVIE UDID>/system.log
#; tail -f ~/Library/Logs/CoreSimulator/3883F70B-AB98-492A-8FF7-504A53289AEF/system.log | grep "TEST\[libphone\]"
```
Note: Use NSLog instead of printf in app, unless there will be no output.  
