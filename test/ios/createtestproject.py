from mod_pbxproj import XcodeProject
import sys
import os
from shutil import copyfile

with open('project.pbxproj', 'r') as seedFile:
  seedContents = seedFile.read()
  seedContents = seedContents\
    .replace('seedProductName', 'test') \
    .replace('seedOrganizationName', 'libphone') \
    .replace('seedOrganizationIdentifier', 'libphone')
  os.system('rm -rf libphonetest.xcodeproj')
  os.mkdir('libphonetest.xcodeproj')
  os.mkdir('libphonetest.xcodeproj/project.xcworkspace')
  copyfile('contents.xcworkspacedata', \
    'libphonetest.xcodeproj/project.xcworkspace/contents.xcworkspacedata')
  with open('libphonetest.xcodeproj/project.pbxproj', 'w') as projectFile:
    projectFile.write(seedContents)
  project = XcodeProject.Load('libphonetest.xcodeproj/project.pbxproj')
  project.add_file_if_doesnt_exist('../../src/libphone.c')
  project.add_file_if_doesnt_exist('../../src/ios/iosphone.m')
  project.add_file_if_doesnt_exist('../../test/test.c')
  project.add_file_if_doesnt_exist('../../test/testtimer.c')
  project.add_file_if_doesnt_exist('../../test/testview.c')
  project.add_file_if_doesnt_exist('../../test/testother.c')
  project.add_file_if_doesnt_exist('../../test/ios/main.m')
  project.add_flags({'HEADER_SEARCH_PATHS':['$(SRCROOT)/../../include', \
    '$(SRCROOT)/../../src', '$(SRCROOT)/../../test']});
  project.add_single_valued_flag('INFOPLIST_FILE', '$(SRCROOT)/Info.plist')
  project.add_single_valued_flag('PRODUCT_BUNDLE_IDENTIFIER', 'libphone.test')
  project.add_single_valued_flag('IPHONEOS_DEPLOYMENT_TARGET', '8.2')
  project.add_single_valued_flag('GCC_TREAT_WARNINGS_AS_ERRORS', 'YES')
  project.remove_single_valued_flag('ASSETCATALOG_COMPILER_APPICON_NAME')
  project.save()
