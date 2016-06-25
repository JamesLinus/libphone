import xml.etree.ElementTree
import sys

class LayoutToCodeTranslator:
    def __init__(self, xmlPath, headerPath, sourcePath):
        self.root = xml.etree.ElementTree.parse(xmlPath).getroot()
        self.mockWidth = float(self.root.attrib['width'])
        self.mockHeight = float(self.root.attrib['height'])
        self.mockStatusBarHeight = float(self.root.attrib['statusBarHeight']) if 'statusBarHeight' in self.root.attrib else 0
        self.mockMenuBarHeight = float(self.root.attrib['menuBarHeight']) if 'menuBarHeight' in self.root.attrib else 0
        self.mockHeight -= self.mockStatusBarHeight
        self.mockHeight -= self.mockMenuBarHeight
        self.root.attrib['offsetY'] = self.mockStatusBarHeight
        self.mockLeft = float(self.root.attrib['left']) if 'left' in self.root.attrib else 0
        self.mockTop = float(self.root.attrib['top']) if 'top' in self.root.attrib else 0
        self.root.parent = None
        self.structOutputs = []
        self.bodyOutputs = []
        self.headerPath = headerPath
        self.sourcePath = sourcePath

    def hor(self, width):
        if width == 0:
            return '0'
        if width == self.mockWidth:
            return 'page->displayWidth'
        return 'page->displayWidth * {}'.format(width / self.mockWidth)

    def ver(self, height):
        if height == 0:
            return '0'
        if height == self.mockHeight:
            return 'page->displayHeight'
        return 'page->displayHeight * {}'.format(height / self.mockHeight)

    def getElementWidth(self, elem):
        loop = elem
        while None != loop:
            if 'width' in loop.attrib:
                return float(loop.attrib['width'])
            loop = loop.parent
        return 0

    def getElementHeight(self, elem):
        loop = elem
        while None != loop:
            if 'height' in loop.attrib:
                return float(loop.attrib['height'])
            loop = loop.parent
        return 0

    def isTrueValue(self, val):
        return '0' != val and 'false' != val.lower()

    def isElementLayoutHorizontal(self, elem):
        if 'isHorizontal' in elem.attrib:
            return self.isTrueValue(elem.attrib['isHorizontal'])
        return False

    def getElementLeft(self, elem):
        left = self.mockLeft
        if 'left' in elem.attrib:
            left = float(elem.attrib['left'])
        elif None != elem.parent:
            if self.isElementLayoutHorizontal(elem.parent) and 'top' not in elem.attrib:
                left = self.getElementLeft(elem.parent) + float(elem.parent.attrib['offsetX'])
            else:
                left = self.getElementLeft(elem.parent)
        return left

    def getElementTop(self, elem):
        top = self.mockTop
        if 'top' in elem.attrib:
            top = float(elem.attrib['top'])
        elif None != elem.parent:
            if not self.isElementLayoutHorizontal(elem.parent) and 'left' not in elem.attrib:
                top = self.getElementTop(elem.parent) + float(elem.parent.attrib['offsetY'])
            else:
                top = self.getElementTop(elem.parent)
        return top

    def getParentHandleInPage(self, elem):
        if elem.parent == self.root:
            return '0'
        return 'page->{}'.format(elem.parent.tag)

    def getElementType(self, elem):
        return elem.attrib['type'] if 'type' in elem.attrib else ''

    def outputElement(self, elem):
        if 'offsetX' not in elem.parent.attrib:
            elem.parent.attrib['offsetX'] = 0
        if 'offsetY' not in elem.parent.attrib:
            elem.parent.attrib['offsetY'] = 0
        width = self.getElementWidth(elem)
        height = self.getElementHeight(elem)
        if elem.tag not in ['padding']:
            self.structOutputs.append('int {};'.format(elem.tag))
            elemType = self.getElementType(elem)
            if 'text' == elemType:
                self.bodyOutputs.append('page->{} = phoneCreateTextView({}, 0);'.format(elem.tag,
                    self.getParentHandleInPage(elem)))
                if 'fontSize' in elem.attrib:
                    self.bodyOutputs.append('phoneSetViewFontSize(page->{}, {});'.format(
                        elem.tag, self.hor(float(elem.attrib['fontSize']))
                    ))
                else:
                    self.bodyOutputs.append('phoneSetViewFontSize(page->{}, {});'.format(
                        elem.tag, self.ver(height)
                    ))
                if 'fontColor' in elem.attrib:
                    self.bodyOutputs.append('phoneSetViewFontColor(page->{}, 0x{});'.format(
                        elem.tag, elem.attrib['fontColor']
                    ))
                if 'value' in elem.attrib:
                    self.bodyOutputs.append('phoneSetViewText(page->{}, "{}");'.format(
                        elem.tag, elem.attrib['value']
                    ))
                if 'fontBold' in elem.attrib:
                    if self.isTrueValue(elem.attrib['fontBold']):
                        self.bodyOutputs.append('phoneSetViewFontBold(page->{}, 1);'.format(
                            elem.tag
                        ))
            else:
                self.bodyOutputs.append('page->{} = phoneCreateContainerView({}, 0);'.format(elem.tag,
                    self.getParentHandleInPage(elem)))
            x = self.getElementLeft(elem) - self.getElementLeft(elem.parent);
            y = self.getElementTop(elem) - self.getElementTop(elem.parent);
            if elem.parent == self.root:
                y -= self.mockStatusBarHeight
            print '{} x:{} y:{} {} left:{} top:{} mockStatusBarHeight:{}'.format(elem.tag, x, y, elem.parent.tag, self.getElementLeft(elem.parent), self.getElementTop(elem.parent), self.mockStatusBarHeight)
            self.bodyOutputs.append('phoneSetViewFrame(page->{}, {}, {}, {}, {});'.
                format(elem.tag, self.hor(x), self.ver(y), self.hor(width), self.ver(height)))
            if 'backgroundColor' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewBackgroundColor(page->{}, 0x{});'.format(
                    elem.tag, elem.attrib['backgroundColor']
                ))
            if 'borderColor' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewBorderColor(page->{}, 0x{});'.format(
                    elem.tag, elem.attrib['borderColor']
                ))
            if 'shadowColor' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewShadowColor(page->{}, 0x{});'.format(
                    elem.tag, elem.attrib['shadowColor']
                ))
            if 'shadowRadius' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewShadowRadius(page->{}, {});'.format(
                    elem.tag, self.hor(float(elem.attrib['shadowRadius']))
                ))
            if 'shadowOpacity' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewShadowOpacity(page->{}, {});'.format(
                    elem.tag, float(elem.attrib['shadowOpacity'])
                ))
            if 'shadowOffsetX' in elem.attrib and 'shadowOffsetY' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewShadowOffset(page->{}, {}, {});'.format(
                    elem.tag, self.hor(float(elem.attrib['shadowOffsetX'])), self.ver(
                        float(elem.attrib['shadowOffsetY']))
                ))
            if 'cornerRadius' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewCornerRadius(page->{}, {});'.format(
                    elem.tag, self.hor(float(elem.attrib['cornerRadius']))
                ))
            if 'borderWidth' in elem.attrib:
                self.bodyOutputs.append('phoneSetViewBorderWidth(page->{}, dp({}));'.format(
                    elem.tag, elem.attrib['borderWidth']
                ))
            for child in elem:
                child.parent = elem
                self.outputElement(child)
        if (self.isElementLayoutHorizontal(elem.parent)):
            elem.parent.attrib['offsetX'] += width
        else:
            elem.parent.attrib['offsetY'] += height

    def outputWarn(self, f):
        f.write('// !!! DONT MODIFY THIS FILE DIRECTLY OR YOU WILL LOST THE CHANGE !!!\n')
        f.write('// THE FILE WAS GENERATED BY github.com/huxingyi/libphone/tools/phonelayout\n')

    def output(self):
        for child in self.root:
            child.parent = self.root
            self.outputElement(child)
        with open(self.headerPath, 'w') as header:
            self.outputWarn(header)
            header.write('\n')
            header.write('#ifndef __{}_H__\n'.format(self.root.tag))
            header.write('#define __{}_H__\n'.format(self.root.tag))
            header.write('\n')
            header.write('typedef struct {} {{\n'.format(self.root.tag))
            header.write('  float displayWidth;\n')
            header.write('  float displayHeight;\n  ')
            header.write('\n  '.join(self.structOutputs))
            header.write('\n}} {};\n'.format(self.root.tag))
            header.write('\n')
            header.write('{} *{}Create(void);\n'.format(self.root.tag, self.root.tag))
            header.write('\n')
            header.write('#endif\n')
            header.write('\n')
        with open(self.sourcePath, 'w') as source:
            self.outputWarn(source)
            source.write('\n')
            source.write('#include "libphone.h"\n')
            source.write('#include "{}"\n'.format(self.headerPath))
            source.write('\n')
            source.write('{} *{}Create(void) {{\n'.format(self.root.tag, self.root.tag))
            source.write('  {} *page = ({} *)calloc(1, sizeof({}));\n'.format(self.root.tag, self.root.tag, self.root.tag))
            source.write('  page->displayWidth = phoneGetViewWidth(0);\n')
            source.write('  page->displayHeight = phoneGetViewHeight(0);\n')
            source.write('  ')
            source.write('\n  '.join(self.bodyOutputs))
            source.write('\n  return page;\n')
            source.write('}\n')
            source.write('\n')

if __name__ == "__main__":
    xmlPath = sys.argv[1]
    headerPath = sys.argv[2]
    sourcePath = sys.argv[3]
    trans = LayoutToCodeTranslator(xmlPath, headerPath, sourcePath)
    trans.output()
