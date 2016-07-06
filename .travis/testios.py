import sys
import os
import re
import subprocess
import time
from threading import Thread
from Queue import Queue, Empty

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

class LogWather:
    def __init__(self, filename):
        self.fp = open(filename, 'r')
        self.fp.seek(0, 2)
        while self.read():
            pass

    def read(self):
        line = self.fp.readline()
        if 0 == len(line):
            return None
        return line

class SimulatorController:
    def __init__(self):
        self.maxBootWaitSeconds = 300
        self.maxRetryTimes = 30
        self.xcodeVer = None

    def getXcodeVersion(self):
        if None != self.xcodeVer:
            return self.xcodeVer
        result, out = executeCmd('xcodebuild -version')
        if 0 != result:
            self.xcodeVer = 'Xcode Notfound'
            return self.xcodeVer
        matchResult = re.search('(Xcode .+)', out)
        if None != matchResult:
            self.xcodeVer = matchResult.group(1)
            return self.xcodeVer
        self.xcodeVer = 'Xcode Unknown'
        return self.xcodeVer

    def getSimulatorList(self):
        result, out = executeCmd('xcrun simctl list')
        if 0 != result:
            return []
        list = out.split('\n')
        returnList = []
        for line in list:
            matchResult = re.search('(i[^\(]+)\(([A-Z0-9\-]+)\) \(([^\)]+)\)', line)
            if None != matchResult:
                deviceTypeName = matchResult.group(1)
                deviceUDID = matchResult.group(2)
                deviceStatus = matchResult.group(3)
                returnList.append({'UDID':deviceUDID, 'typeName':deviceTypeName,
                    'status':deviceStatus})
        return returnList

    def isSimulatorStatusBooted(self, device):
        return 'Booted' == device['status']

    def isSimulatorBooted(self, UDID):
        list = self.getSimulatorList()
        for device in list:
            if device['UDID'] == UDID:
                if self.isSimulatorStatusBooted(device):
                    return True
                else:
                    return False
        return False

    def getSimulatorProcessName(self):
        if -1 != self.getXcodeVersion().find('Xcode 7'):
            return 'Simulator'
        return 'iOS Simulator'

    def bootSimulator(self, UDID):
        needKill = True
        countSecondsForBoot = 0;
        while True:
            if self.isSimulatorBooted(UDID):
                return True
            if needKill:
                needKill = False
                executeCmd('killall "{}"'.format(self.getSimulatorProcessName()))
            if countSecondsForBoot > self.maxBootWaitSeconds:
                break
            result, out = executeCmd('open -a "{}" --args -CurrentDeviceUDID {}'.format(self.getSimulatorProcessName(), device['UDID']))
            if 0 != result:
                return False
            time.sleep(1)
            countSecondsForBoot += 1
        return False

    def installAppToBootedSimulator(self, appPath):
        count = 0
        while count < self.maxRetryTimes:
            result, out = executeCmd('xcrun simctl install booted {}'.format(appPath))
            if 0 == result:
                break
            count += 1
            time.sleep(1)
        return True

    def runAppOnBootedSimulator(self, appName):
        result, out = executeCmd('xcrun simctl launch booted {}'.format(appName))
        if 0 != result:
            return False
        return True

    def getOrderedTestList(self):
        deviceList = self.getSimulatorList()
        iPhone = None
        iPad = None
        bootedList = []
        shutdownList = []
        testList = []
        for device in deviceList:
            if -1 != device['typeName'].find('iPhone'):
                if None == iPhone or self.isSimulatorStatusBooted(device):
                    iPhone = device
            elif -1 != device['typeName'].find('iPad'):
                if None == iPad or self.isSimulatorStatusBooted(device):
                    iPad = device
        if None == iPhone and None == iPad:
            return []
        if None != iPhone:
            if self.isSimulatorStatusBooted(iPhone):
                bootedList.append(iPhone)
            else:
                shutdownList.append(iPhone)
        if None != iPad:
            if self.isSimulatorStatusBooted(iPad):
                bootedList.append(iPad)
            else:
                shutdownList.append(iPad)
        for device in bootedList:
            testList.append(device)
        for device in shutdownList:
            testList.append(device)
        return testList

if __name__ == "__main__":
    simctrl = SimulatorController()
    print('{}'.format(simctrl.getXcodeVersion()))
    testList = simctrl.getOrderedTestList()
    maxWaitLogSeconds = 10
    if 0 == len(testList):
        die('getOrderedTestList failed')
    print('will build')
    result, out = executeCmd('xcodebuild -project test/ios/libphonetest.xcodeproj -target test -sdk iphonesimulator ONLY_ACTIVE_ARCH=NO')
    if 0 != result:
        die('build failed, result: {} output: {}'.format(result, out))
    for device in testList:
        print('will test on {} {}'.format(device['UDID'], device['typeName']))
        if not simctrl.isSimulatorBooted(device['UDID']):
            print('will boot simulator {} {}'.format(device['UDID'], device['typeName']))
            if not simctrl.bootSimulator(device['UDID']):
                die('bootSimulator {} {} failed'.format(device['UDID'], device['typeName']))
        print('will install test app to {} {}'.format(device['UDID'], device['typeName']))
        if not simctrl.installAppToBootedSimulator('test/ios/build/Release-iphonesimulator/test.app'):
            die('installAppToBootedSimulator failed')
        logWather = LogWather(os.path.expanduser('~/Library/Logs/CoreSimulator/{}/system.log'.format(device['UDID'])))
        print('will run test app on {} {}'.format(device['UDID'], device['typeName']))
        if not simctrl.runAppOnBootedSimulator('libphone.test'):
            die('runAppOnBootedSimulator failed')
        print('will wait test log')
        countSecondsForWait = 0
        while True:
            output = logWather.read()
            if None != output:
                if -1 != output.find('TEST[libphone]'):
                    countSecondsForWait = 0
                    print(output)
                    if -1 != output.find('All Test Succeed('):
                        break
                    if -1 != output.find('Test Failed('):
                        die('will quit travis because test not passed')
            else:
                if countSecondsForWait > maxWaitLogSeconds:
                    die('waiting too long for log')
                time.sleep(1)
                countSecondsForWait += 1
    print('will quit travis because test done')
