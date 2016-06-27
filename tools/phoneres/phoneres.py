import sys
import os
import re
import subprocess

def executeCmd(cmd):
    try:
        out = subprocess.check_output(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        return (e.returncode, e.output)
    return (0, out)

def isEssentialInstalled():
    if -1 != executeCmd('identify -version')[1].find('ImageMagick') and \
            -1 != executeCmd('convert -version')[1].find('ImageMagick'):
        return True
    return False

class AndroidResGenerator:
    def __init__(self, resPath, outputPath):
        self.dpiMap = [
            [0.75, 'ldpi'],
            [1,    'mdpi'],
            [1.5,  'hdpi'],
            [2,    'xhdpi'],
            [3,    'xxhdpi'],
            [4,    'xxxhdpi']
        ]
        self.resPath = resPath
        self.outputPath = outputPath
        self.images = []
        for f in os.listdir(resPath):
            if f.endswith('.png'):
                self.images.append(f)

    def output(self):
        succeedNum = 0
        errorNum = 0
        for f in self.images:
            convertSucceed = True
            destFilename = f.split('.')[0]
            matchResult = re.search('([0-9]+)x([0-9]+)', f)
            if None != matchResult:
                widthInMDPI = matchResult.group(1)
                heightInMDPI = matchResult.group(2)
                destFilename = destFilename.replace('{}x{}'.format(widthInMDPI, heightInMDPI), '')
            else:
                cmd = 'identify -ping -format "%wx%w" "{}/{}"'.format(self.resPath, f)
                (err, out) = executeCmd(cmd)
                if 0 == err:
                    matchResult = re.search('([0-9]+)x([0-9]+)', out)
                    widthInMDPI = int(int(matchResult.group(1)) * float(self.dpiMap[len(self.dpiMap) - 1][0]))
                    heightInMDPI = int(int(matchResult.group(2)) * float(self.dpiMap[len(self.dpiMap) - 1][0]))
                    print('Warn: no WIDTHxHEIGHT pattern in filename, use image size {}x{} as {}'.format(widthInMDPI, heightInMDPI,
                        self.dpiMap[len(self.dpiMap) - 1][1]))
                    print('Note: if WIDTHxHEIGHT pattern find in filename, WIDTHxHEIGHT will be used as mdpi')
                else:
                    print('Failed to "{}"'.format(cmd))
                    convertSucceed = False
            if convertSucceed:
                for dpi in self.dpiMap:
                    try:
                        os.mkdir('{}/drawable-{}'.format(self.outputPath, dpi[1]))
                    except:
                        pass
                    destWidth = int(float(widthInMDPI) * dpi[0])
                    destHeight = int(float(heightInMDPI) * dpi[0])
                    cmd = 'convert "{}/{}" -background transparent -resize "{}x{}>" -gravity center -extent "{}x{}" "{}/drawable-{}/{}.png"'.format(
                        self.resPath, f,
                        destWidth, destHeight,
                        destWidth, destHeight,
                        self.outputPath, dpi[1], destFilename)
                    if (0 != executeCmd(cmd)[0]):
                        print('Failed to "{}"'.format(cmd))
                        convertSucceed = False
            if convertSucceed:
                succeedNum += 1
            else:
                errorNum += 1
        if errorNum > 0:
            print('Succeed({}) Error({})'.format(succeedNum, errorNum))
            return -1
        elif succeedNum > 0:
            print('All Succeed({})'.format(succeedNum))
            return 0
        else:
            print('Warn: empty folder')
            return 0

if __name__ == "__main__":
    if not isEssentialInstalled():
        print('Make sure `identify -version` and `convert -version` are worked before run phoneres.py')
        print('Note: identify and convert are part of ImageMagick command line tool.')
        print('      Install via `brew install imagemagick` on Mac or visit http://imagemagick.org')
        sys.exit(-1)
    if len(sys.argv) < 3:
        print('Usage: python phoneres.py <resPath> <androidResPath>')
        sys.exit(-1)
    resPath = sys.argv[1]
    androidResPath = sys.argv[2]
    if not os.path.isdir(os.path.normpath(resPath)):
        print('"{}" is not valid directory.'.format(resPath))
        sys.exit(-1)
    if not os.path.isdir(os.path.normpath(androidResPath)):
        print('"{}" is not valid directory.'.format(androidResPath))
        sys.exit(-1)
    generator = AndroidResGenerator(resPath, androidResPath)
    sys.exit(generator.output())
