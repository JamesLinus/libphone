import sys
import os
import re
import subprocess
import time

def executeCmd(cmd):
    try:
        out = subprocess.check_output(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        return (e.returncode, e.output)
    return (0, out)

def die(log):
    print(log)
    sys.stdout.flush()
    exit(1)

class EmulatorController:
    def __init__(self):
        self.maxBootWaitSeconds = 300
        self.maxRetryTimes = 30
        self.supportedPlatforms = None
        self.wantTestPlatform = 9

    def getAttachedDeviceList(self):
        result, out = executeCmd('adb devices')
        deviceList = []
        if 0 != result:
            return deviceList
        for line in out.split('\n'):
            id = line.split('\t')[0]
            if -1 != id.find('emulator-'):
                deviceList.append(id)
        return deviceList

    def getSortedSupportedPlatforms(self):
        '''
        if None == self.supportedPlatforms:
            result, adbPath = executeCmd('which adb')
            if 0 != result:
                return []
            sdkTokenStart = adbPath.find('/sdk/')
            if -1 == sdkTokenStart:
                return []
            platformsFolder = '{}/sdk/platforms/'.format(adbPath[0:sdkTokenStart])
            subdirs = os.listdir(platformsFolder)
            platformList = []
            for name in subdirs:
                matchResult = re.search('android-(\d+)', name)
                if None != matchResult:
                    platformList.append(int(matchResult.group(1)))
            platformList.sort()
            self.supportedPlatforms = platformList
        return self.supportedPlatforms
        '''
        if None == self.supportedPlatforms:
            result, out = executeCmd('android list targets')
            if 0 != result:
                return []
            platformList = []
            for item in out.split('----------'):
                platformMatch = re.search('"android-(\d+)"', item)
                idMatch = re.search('id: (\d+)', item)
                abiMatch = re.search('Tag/ABIs :(.+)', item)
                if None != platformMatch and None != idMatch and None != abiMatch:
                    abi = abiMatch.group(1).split(',')[0].strip()
                    if -1 == abi.find('no '):
                        platformList.append([int(platformMatch.group(1)),
                            int(idMatch.group(1)), abi])
                platformList.sort()
                self.supportedPlatforms = platformList
        return self.supportedPlatforms

    def killAllEmulators(self):
        '''
        deviceList = self.getAttachedDeviceList()
        for device in deviceList:
            print('will kill {}'.format(device))
            result, out = executeCmd('adb -s {} emu kill'.format(device))
            if 0 != result:
                return False
        return True
        '''
        result, out = executeCmd("ps aux | grep [e]mulator | grep avd | awk '{print $2}'")
        if 0 == result:
            for pid in out.split('\n'):
                if len(pid.strip()) > 0:
                    print('will kill pid: {}'.format(pid))
                    executeCmd('kill -9 {}'.format(pid))
        return True

    def createAndRunTestEmulator(self):
        selectedPlatform = None
        platformList = self.getSortedSupportedPlatforms()
        for platform in platformList:
            if platform[0] >= self.wantTestPlatform:
                print('will try create emulator android-{} {}'.format(platform[0],
                    platform[2]))
                result, out = executeCmd('echo no | android create avd --force --name test --target {} --abi {}'.format(platform[1], platform[2]))
                if 0 == result:
                    selectedPlatform = platform
                    break
        if None == selectedPlatform:
            die('no available emulator')
        print('selected platform: android-{} {}'.format(selectedPlatform[0],
            selectedPlatform[2]))
        print('will run emulator')
        os.system('emulator -avd test -no-skin -no-audio -no-window &')
        countForWait = 0
        print('will wait emulator')
        while True:
            result, out = executeCmd('adb get-state')
            if 0 != result:
                die('wait emulator failed, result: {} output: {}'.format(result, out))
            if -1 != out.find('device'):
                break
            time.sleep(1)
            countForWait += 1
            if countForWait > self.maxBootWaitSeconds:
                die('wait too long for emulator booting')
        print('will unlock emulator')
        os.system('adb shell input keyevent 82 &')
        print('will quit')

    def installAppToCurrentEmulator(self, package, appPath):
        count = 0
        executeCmd('adb shell pm uninstall {}'.format(package))
        while True:
            result, out = executeCmd('adb install {}'.format(appPath))
            if 0 == result:
                time.sleep(1)
                result, out = executeCmd('adb shell pm list packages')
                if 0 == result and -1 != out.find(package):
                    break
            time.sleep(1)
            count += 1
            if count > self.maxRetryTimes:
                return False
        return True

    def runAppOnCurrentEmulator(self, appName):
        result, out = executeCmd('adb shell monkey -p {} -c android.intent.category.LAUNCHER 1'.format(appName))
        if 0 != result:
            return False
        return True

def beforeInstall():
    print('will download sdk')
    result, out = executeCmd('wget http://dl.google.com/android/android-sdk_r23.0.2-linux.tgz')
    if 0 != result:
        die('failed to download sdk')
    result, out = executeCmd('tar xzf android-sdk_r23.0.2-linux.tgz')
    if 0 != result:
        die('failed to tar package')
    executeCmd('export ANDROID_HOME=$PWD/android-sdk-linux')
    executeCmd('echo $ANDROID_HOME')
    executeCmd('export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools')
    executeCmd('echo $PATH')
    print('will update sdk')
    result, out = executeCmd('echo "y" | android update sdk --filter platform-tools,build-tools-25.1.7,android-23,extra-android-support,$ANDROID_SDKS --no-ui --force > /dev/null')
    if 0 != result:
        die('failed to update sdk')
    print('will leave before_install')

def beforeScript():
    emuctrl = EmulatorController()
    print('will fetch supported platforms')
    print('supported platforms: {}'.format(emuctrl.getSortedSupportedPlatforms()))
    print('will kill attached devices {}'.format(emuctrl.getAttachedDeviceList()))
    if not emuctrl.killAllEmulators():
        die('failed to kill attached devices')
    print('will create and run emulator')
    emuctrl.createAndRunTestEmulator()
    print('will leave before_script')

def script():
    maxLogcatWaitSeconds = 60
    emuctrl = EmulatorController()
    print('will build')
    result, out = executeCmd('cd test/android && ./gradlew assembleDebug')
    if 0 != result:
        die('build failed, result: {} output: {}'.format(result, out))
    print('will install')
    if not emuctrl.installAppToCurrentEmulator('com.libphone.test',
            'test/android/app/build/outputs/apk/app-debug.apk'):
        die('failed to install')
    print('will clean logcat')
    executeCmd('adb logcat -c')
    print('will run')
    if not emuctrl.runAppOnCurrentEmulator('com.libphone.test'):
        die('failed to run')
    print('will collect logcat')
    countForLogcatWait = 0
    logmap = {}
    needQuit = False
    while not needQuit:
        result, out = executeCmd('adb logcat -d TEST[libphone]:I *:S')
        if 0 != result:
            die('failed to collect logcat')
        for line in out.split('\n'):
            if line not in logmap:
                print(line)
                logmap[line] = True
                if -1 != line.find('TEST[libphone]'):
                    countForLogcatWait = 0
                    if -1 != line.find('All Test Succeed('):
                        needQuit = True
                        break
                    if -1 != line.find('Test Failed('):
                        die('will quit because test not passed')
        if needQuit:
            break
        time.sleep(1)
        countForLogcatWait += 1
        if countForLogcatWait > maxLogcatWaitSeconds:
            die('wait too long for locat')
    print('will quit because test done')

if __name__ == "__main__":
    action = sys.argv[1] if len(sys.argv) >= 2 else None
    if None == action:
        die('usage: python .travis/testandroid.py <beforeInstall/beforeScript/script>')
    if 'beforeInstall' == action:
        beforeInstall()
    elif 'beforeScript' == action:
        beforeScript()
    elif 'script' == action:
        script()
    elif 'local' == action:
        beforeScript()
        script()
    else:
        die('unknown action: {}'.format(action))
