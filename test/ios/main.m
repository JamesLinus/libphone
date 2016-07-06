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