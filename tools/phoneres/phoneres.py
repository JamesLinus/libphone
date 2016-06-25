import sys
import os

class AndroidResGenerator:
    def __init__(self, widthInMDPI, heightInMDPI, resPath, outputPath):
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
        self.widthInMDPI = widthInMDPI
        self.heightInMDPI = heightInMDPI
        self.images = []
        for f in os.listdir(resPath):
            if f.endswith('.png'):
                self.images.append(f)

    def output(self):
        succeedNum = 0
        errorNum = 0
        for f in self.images:
            convertSucceed = True
            for dpi in self.dpiMap:
                destWidth = int(float(self.widthInMDPI) * dpi[0])
                destHeight = int(float(self.heightInMDPI) * dpi[0])
                try:
                    os.mkdir('{}/drawable-{}'.format(self.outputPath, dpi[1]))
                except:
                    pass
                cmd = 'convert "{}/{}" -background transparent -gravity center -resize {}x{}^ -extent {}x{} "{}/drawable-{}/{}{}x{}.png"'.format(
                    self.resPath, f,
                    destWidth, destHeight,
                    destWidth, destHeight,
                    self.outputPath, dpi[1], f.split('.')[0],
                    self.widthInMDPI, self.heightInMDPI)
                if (0 != os.system(cmd)):
                    print 'CMD ERROR OF: {}'.format(cmd)
                    convertSucceed = False
            if convertSucceed:
                succeedNum += 1
            else:
                errorNum += 1
        if errorNum > 0:
            print 'Succeed({}) Error({})'.format(succeedNum, errorNum)
        elif succeedNum > 0:
            print 'All Succeed({})'.format(succeedNum)
        else:
            print 'Not found valid entries'

if __name__ == "__main__":
    widthXheight = sys.argv[1]
    (widthInMDPI, heightInMDPI) = widthXheight.split('x')
    resPath = sys.argv[2]
    androidPath = sys.argv[3]
    print 'widthInMDPI:', widthInMDPI
    print 'heightInMDPI:', heightInMDPI
    print 'resPath:', resPath
    print 'androidPath:', androidPath
    generator = AndroidResGenerator(widthInMDPI, heightInMDPI,
        resPath, androidPath)
    generator.output()
