#include "libphone.h"
#include "libphoneprivate.h"
#include <pthread.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <QuartzCore/QuartzCore.h>

phoneAppDelegate *objcDelegate = nil;
NSMutableDictionary *objcHandleMap = nil;
UIWindow *objcWindow = nil;
UIImageView *objcContainer = nil;

///////////////////////////////////////////////////////////////////////////////

@interface UselessViewController : UIViewController
@end

@implementation UselessViewController

- (void)viewDidLoad {
  [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  phoneLog(PHONE_LOG_WARN, __FUNCTION__, "didReceiveMemoryWarning");
}

@end

///////////////////////////////////////////////////////////////////////////////

@interface PhoneForTouchDetectRecognizer : UIGestureRecognizer
@property (nonatomic, assign) id target;
@end

@implementation PhoneForTouchDetectRecognizer

- (id)initWithTarget:(id)target {
  if (self = [super initWithTarget:target action:nil]) {
    self.target = target;
  }
  return self;
}

- (void)reset {
  [super reset];
  //self.state = UIGestureRecognizerStatePossible;
}

- (void)touchesBegin:(NSSet *)touches withEvent:(UIEvent *)event {
  int handle = (int)((UIView *)self.target).tag;
  phoneHandle *handleData = pHandle(handle);
  [super touchesBegin:touches withEvent:event];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "touchesBegin");
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    memset(&touch, 0, sizeof(touch));
    touch.touchType = PHONE_VIEW_TOUCH_BEGIN;
    handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  int handle = (int)((UIView *)self.target).tag;
  phoneHandle *handleData = pHandle(handle);
  [super touchesMoved:touches withEvent:event];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "touchesMoved");
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    memset(&touch, 0, sizeof(touch));
    touch.touchType = PHONE_VIEW_TOUCH_MOVE;
    handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  int handle = (int)((UIView *)self.target).tag;
  phoneHandle *handleData = pHandle(handle);
  [super touchesEnded:touches withEvent:event];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "touchesEnded");
  if (handleData->u.view.eventHandler) {
    phoneViewTouch touch;
    memset(&touch, 0, sizeof(touch));
    touch.touchType = PHONE_VIEW_TOUCH_END;
    handleData->u.view.eventHandler(handle, PHONE_VIEW_TOUCH, &touch);
  }
}

@end

///////////////////////////////////////////////////////////////////////////////

static int phoneTableViewSeparatorStyleToIos(int style) {
  //UITableViewCellSeparatorStyleNone,
  //UITableViewCellSeparatorStyleSingleLine,
  //UITableViewCellSeparatorStyleSingleLineEtched
  return UITableViewCellSeparatorStyleSingleLine;
}

static int phoneTableViewStyleToIos(int style) {
  switch (style) {
    case PHONE_TABLE_VIEW_STYLE_PLAIN:
      return UITableViewStylePlain;
    case PHONE_TABLE_VIEW_STYLE_GROUPED:
      return UITableViewStyleGrouped;
    default:
      assert(0 && "Unknown table view style");
      return UITableViewStylePlain;
  }
}

static int phoneTableViewSelectionStyleToIos(int style) {
  //UITableViewCellSelectionStyleNone,
  //UITableViewCellSelectionStyleBlue,
  //UITableViewCellSelectionStyleGray,
  //UITableViewCellSelectionStyleDefault
  return UITableViewCellSelectionStyleGray;
}

@interface PhoneTableView : UITableView <UITableViewDelegate, UITableViewDataSource>
@end

@implementation PhoneTableView

- (id)initWithStyle:(UITableViewStyle)style {
  CGRect frame = {0, 0, 0, 0};
  self = [super initWithFrame:frame style:style];
  self.dataSource = self;
  self.delegate = self;
  self.tag = 0;
  return self;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
  int handle = tableView.tag;
  if (0 == handle) {
    return 0;
  }
  self.separatorStyle = phoneTableViewSeparatorStyleToIos(
    shareRequestTableViewCellSeparatorStyle(handle)
  );
  return shareRequestTableViewSectionCount(handle);
}

- (NSInteger)tableView:(UITableView *)tableView
    numberOfRowsInSection:(NSInteger)section {
  int handle = tableView.tag;
  if (0 == handle) {
    return 0;
  }
  return shareRequestTableViewRowCount(handle, (int)section);
}

- (void)tableView: (UITableView *)tableView
	  didSelectRowAtIndexPath: (NSIndexPath *)indexPath {
  int handle = tableView.tag;
  shareRequestTableViewCellClick(handle, indexPath.section, indexPath.row,
    [tableView getRenderHandleFromIndexPath:indexPath]);
}

- (CGFloat)tableView:(UITableView *)tableView
    heightForRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = tableView.tag;
  return (CGFloat)shareRequestTableViewRowHeight(handle,
    indexPath.section, indexPath.row);
}

- (NSString *)tableView:(UITableView *)tableView
    titleForHeaderInSection:(NSInteger)section {
  int handle = tableView.tag;
  char buf[4096];
  shareRequestTableViewSectionHeader(handle, (int)section, buf, sizeof(buf));
  return [NSString stringWithUTF8String:buf];
}

- (NSString *)tableView:(UITableView *)tableView
    titleForFooterInSection:(NSInteger)section {
  int handle = tableView.tag;
  char buf[4096];
  shareRequestTableViewSectionFooter(handle, (int)section, buf, sizeof(buf));
  return [NSString stringWithUTF8String:buf];
}

- (int)getRenderHandleFromIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [self cellForRowAtIndexPath:indexPath];
  if (nil != cell && nil != cell.contentView) {
    for (UIView *loopView in cell.contentView.subviews) {
      if (loopView.tag > 0) {
        return loopView.tag;
      }
    }
  }
  return 0;
}

- (void)tableView:(UITableView *)tableView
    willDisplayCell:(UITableViewCell *)cell
    forRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = tableView.tag;
  shareRequestTableViewCellRender(handle, indexPath.section, indexPath.row,
    [tableView getRenderHandleFromIndexPath:indexPath]);
}

- (UITableViewCell *)tableView:(UITableView *)tableView
    cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = tableView.tag;
  char buf[4096];
  UITableViewCell *cell;
  NSString *cellIdentifier;
  shareRequestTableViewCellIdentifier(handle, indexPath.section,
      indexPath.row, buf, sizeof(buf));
  cellIdentifier = [NSString stringWithUTF8String:buf];
  cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
  if (nil == cell) {
    int customView;
    int accessoryView;
    cell = [[UITableViewCell alloc]
        initWithStyle:UITableViewCellStyleDefault
        reuseIdentifier:cellIdentifier];
    customView = shareRequestTableViewCellCustomView(handle, indexPath.section,
        indexPath.row);
    if (customView) {
      [cell.contentView addSubview:(UIView *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:customView]]];
    }
    cell.accessoryType = UITableViewCellAccessoryNone;
    accessoryView = shareRequestTableViewCellAccessoryView(handle,
        indexPath.section, indexPath.row);
    if (accessoryView) {
      cell.accessoryView = (UIView *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:accessoryView]];
    }
  }
  shareRequestTableViewCellImageResource(handle, indexPath.section,
      indexPath.row, buf, sizeof(buf));
  if (buf[0]) {
    cell.imageView.image = [UIImage
        imageNamed:[NSString stringWithUTF8String:buf]];
  }
  cell.selectionStyle = phoneTableViewSelectionStyleToIos(
    shareRequestTableViewCellSelectionStyle(handle,
        indexPath.section, indexPath.row)
  );
  shareRequestTableViewCellText(handle, indexPath.section,
      indexPath.row, buf, sizeof(buf));
  cell.textLabel.text = [NSString stringWithUTF8String:buf];
  shareRequestTableViewCellDetailText(handle, indexPath.section,
      indexPath.row, buf, sizeof(buf));
  cell.detailTextLabel.text = [NSString stringWithUTF8String:buf];
  return cell;
}

@end

///////////////////////////////////////////////////////////////////////////////

@implementation phoneAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  CGRect containerFrame = [[UIScreen mainScreen] bounds];
  CGFloat statusBarHeight = [[UIApplication sharedApplication] statusBarFrame].size.height;
  containerFrame.origin.y += statusBarHeight;
  containerFrame.size.height -= statusBarHeight;
  objcDelegate = self;
  _handleMap = [[NSMutableDictionary alloc] init];
  objcHandleMap = _handleMap;
  _window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  _window.backgroundColor = [UIColor whiteColor];
  _window.rootViewController = [[UselessViewController alloc] init];
  objcWindow = _window;
  _container = [[UIImageView alloc] initWithFrame:containerFrame];
  _container.backgroundColor = [UIColor whiteColor];
  _container.userInteractionEnabled = YES;
  [_window addSubview:_container];
  objcContainer = _container;
  phoneInitApplication();
  phoneMain(0, 0);
  [_window makeKeyAndVisible];
  return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
  pApp->handler->hiding();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
  pApp->handler->showing();
}

- (void)applicationWillTerminate:(UIApplication *)application {
  pApp->handler->terminating();
}

- (void)dispatchTimer:(NSTimer *)timer {
  int handle = (int)[(NSNumber *)[timer userInfo] intValue];
  pHandle(handle)->u.timer.runHandler(handle);
}

- (UIColor *)makeColor:(unsigned int)rgb {
  return [UIColor colorWithRed:(((0x00ffffff & rgb) >> 16) & 0xFF)/255.0
    green:(((0x0000ffff & rgb) >> 8) & 0xFF) / 255.0
    blue:(((0x000000ff & rgb)) & 0xFF) / 255.0
    alpha:((rgb >> 24) & 0xFF) / 255.0];
}

- (void)dispatchTextFieldDidChange:(UITextField *)view {
  int handle = (int)view.tag;
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    handleData->u.view.eventHandler(handle, PHONE_VIEW_VALUE_CHANGE, 0);
  }
}

- (void)dispatchPressGesture:(UITapGestureRecognizer *)sender {
  UIView *view = sender.view;
  if (UIGestureRecognizerStateEnded == sender.state) {
    int handle = (int)view.tag;
    phoneHandle *handleData = pHandle(handle);
    if (handleData->u.view.eventHandler) {
      handleData->u.view.eventHandler(handle, PHONE_VIEW_CLICK, 0);
    }
  }
}

- (void)dispatchLongPressGesture:(UILongPressGestureRecognizer *)sender {
  UIView *view = sender.view;
  //CGPoint point = [sender locationInView:view.superview];
  if (UIGestureRecognizerStateBegan == sender.state) {
    int handle = (int)view.tag;
    phoneHandle *handleData = pHandle(handle);
    if (handleData->u.view.eventHandler) {
      handleData->u.view.eventHandler(handle, PHONE_VIEW_LONG_CLICK, 0);
    }
  }
}

@end

///////////////////////////////////////////////////////////////////////////////

static const char *phoneLogLevelToString(int level) {
  switch (level) {
    case PHONE_LOG_DEBUG:
      return "DEBUG";
    case PHONE_LOG_INFO:
      return "INFO";
    case PHONE_LOG_WARN:
      return "WARN";
    case PHONE_LOG_ERROR:
      return "ERROR";
    case PHONE_LOG_FATAL:
      return "FATAL";
    default: {
      if (level < PHONE_LOG_DEBUG) {
        return "DEBUG";
      } else {
        return "FATAL";
      }
    }
  }
}

int shareDumpLog(int level, const char *tag, const char *log, int len) {
  return printf("%s %s %.*s\n", phoneLogLevelToString(level), tag, len, log);
}

int shareNeedFlushMainWorkQueue(void) {
  dispatch_async(dispatch_get_main_queue(), ^{
    phoneFlushMainWorkQueue();
  });
  return 0;
}

int shareInitApplication(void) {
  return 0;
}

int shareCreateTimer(int handle, unsigned int milliseconds) {
  NSTimer *timer = [NSTimer
    scheduledTimerWithTimeInterval:((double)milliseconds) / 1000.0
    target:objcDelegate
    selector:@selector(dispatchTimer:)
    userInfo:[NSNumber numberWithInt:handle]
    repeats:YES];
  [objcHandleMap
    setObject:timer
    forKey:[NSNumber numberWithInt:handle]];
  return 0;
}

int shareRemoveTimer(int handle) {
  NSTimer *timer = (NSTimer *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [timer invalidate];
  [objcHandleMap removeObjectForKey:[NSNumber numberWithInt:handle]];
  return 0;
}

static void addViewToParent(UIView *view, int parentHandle) {
  if (0 == parentHandle) {
    [objcContainer addSubview:view];
  } else {
    UIView *parentView = (UIView *)[objcHandleMap
      objectForKey:[NSNumber numberWithInt:(int)parentHandle]];
    [parentView addSubview:view];
  }
}

int shareCreateContainerView(int handle, int parentHandle) {
  UIImageView *view = [[UIImageView alloc] init];
  [objcHandleMap
    setObject:view
    forKey:[NSNumber numberWithInt:handle]];
  addViewToParent(view, parentHandle);
  view.tag = handle;
  view.userInteractionEnabled = YES;
  return 0;
}

int shareSetViewBackgroundColor(int handle, unsigned int color) {
  UIView *view = 0 == handle ? objcContainer : (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.backgroundColor = [objcDelegate makeColor:(0xff000000 | color)];
  return 0;
}

int shareSetViewFrame(int handle, float x, float y, float width, float height) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.frame = CGRectMake((CGFloat)x,
    (CGFloat)y,
    (CGFloat)width,
    (CGFloat)height);
  return 0;
}

int shareCreateTextView(int handle, int parentHandle) {
  UILabel *view = [[UILabel alloc] init];
  [objcHandleMap
    setObject:view
    forKey:[NSNumber numberWithInt:handle]];
  addViewToParent(view, parentHandle);
  view.tag = handle;
  return 0;
}

int shareSetViewText(int handle, const char *val) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      view.text = [NSString stringWithUTF8String:val];
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      view.text = [NSString stringWithUTF8String:val];
    } break;
  }
  return 0;
}

int shareSetViewFontColor(int handle, unsigned int color) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      view.textColor = [objcDelegate makeColor:(0xff000000 | color)];
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      view.textColor = [objcDelegate makeColor:(0xff000000 | color)];
    } break;
  }
  return 0;
}

int shareShowView(int handle, int display) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.hidden = display ? NO : YES;
  return 0;
}

float shareGetViewWidth(int handle) {
  UIView *view;
  if (0 == handle) {
    return objcContainer.frame.size.width;
  }
  view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  return view.frame.size.width;
}

float shareGetViewHeight(int handle) {
  UIView *view;
  if (0 == handle) {
    return objcContainer.frame.size.height;
  }
  view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  return view.frame.size.height;
}

int shareCreateViewAnimationSet(int handle) {
  // need do nothing
  return 0;
}

int shareAddViewAnimationToSet(int animationHandle, int setHandle) {
  // need do nothing
  return 0;
}

int shareRemoveViewAnimationSet(int handle) {
  // need do nothing
  return 0;
}

int shareRemoveViewAnimation(int handle) {
  // need do nothing
  return 0;
}

int shareCreateViewTranslateAnimation(int handle, int viewHandle,
    float offsetX, float offsetY) {
  // need do nothing
  return 0;
}

int shareBeginAnimationSet(int handle, int duration) {
  int loopHandle = (int)pHandle(handle)->next;
  assert(PHONE_VIEW_ANIMATION_SET == pHandle(handle)->type);
  while (loopHandle) {
    phoneHandle *loopHandleData = pHandle(loopHandle);
    switch (loopHandleData->type) {
      case PHONE_VIEW_TRANSLATE_ANIMATION: {
      } break;
      case PHONE_VIEW_ALPHA_ANIMATION: {
        UIView *view = (UIView *)[objcHandleMap
          objectForKey:[NSNumber numberWithInt:loopHandleData->u.animation.viewHandle]];
        [view setAlpha:loopHandleData->u.animation.u.alpha.fromAlpha];
      } break;
    }
    loopHandle = (int)loopHandleData->next;
  }
  [UIView animateWithDuration:(float)duration / 1000 animations:^{
    int loopHandle = (int)pHandle(handle)->next;
    assert(PHONE_VIEW_ANIMATION_SET == pHandle(handle)->type);
    while (loopHandle) {
      phoneHandle *loopHandleData = pHandle(loopHandle);
      switch (loopHandleData->type) {
        case PHONE_VIEW_TRANSLATE_ANIMATION: {
          UIView *view = (UIView *)[objcHandleMap
            objectForKey:[NSNumber numberWithInt:loopHandleData->u.animation.viewHandle]];
          [view setFrame:CGRectMake((CGFloat)view.frame.origin.x +
              loopHandleData->u.animation.u.translate.offsetX,
            (CGFloat)view.frame.origin.y +
              loopHandleData->u.animation.u.translate.offsetY,
            (CGFloat)view.frame.size.width,
            (CGFloat)view.frame.size.height)];
        } break;
        case PHONE_VIEW_ALPHA_ANIMATION: {
          UIView *view = (UIView *)[objcHandleMap
            objectForKey:[NSNumber numberWithInt:loopHandleData->u.animation.viewHandle]];
          [view setAlpha:loopHandleData->u.animation.u.alpha.toAlpha];
        } break;
      }
      loopHandle = (int)loopHandleData->next;
    }
  } completion:^(BOOL finished) {
    phoneHandle *setHandleData = pHandle(handle);
    setHandleData->u.animationSet.finishHandler(handle);
  }];
  return 0;
}

int shareCreateViewAlphaAnimation(int handle, int viewHandle,
    float fromAlpha, float toAlpha) {
  // need do nothing
  return 0;
}

int shareBringViewToFront(int handle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [objcContainer bringSubviewToFront:view];
  return 0;
}

int shareSetViewAlpha(int handle, float alpha) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.alpha = alpha;
  return 0;
}

int shareSetViewFontSize(int handle, float fontSize) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      [view setFont:[UIFont systemFontOfSize:(GLfloat)fontSize]];
      if ([view.font.fontName isEqualToString:[UIFont
          systemFontOfSize:(GLfloat)fontSize].fontName]) {
        [view setFont:[UIFont systemFontOfSize:(GLfloat)fontSize]];
      } else {
        [view setFont:[UIFont boldSystemFontOfSize:(GLfloat)fontSize]];
      }
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      if ([view.font.fontName isEqualToString:[UIFont
          systemFontOfSize:(GLfloat)fontSize].fontName]) {
        [view setFont:[UIFont systemFontOfSize:(GLfloat)fontSize]];
      } else {
        [view setFont:[UIFont boldSystemFontOfSize:(GLfloat)fontSize]];
      }
    } break;
  }
  return 0;
}

int shareSetViewBackgroundImageResource(int handle,
    const char *imageResource) {
  UIImageView *view = (UIImageView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view setImage:[UIImage
    imageNamed:[NSString stringWithUTF8String:imageResource]]];
  return 0;
}

int shareSetViewBackgroundImagePath(int handle,
    const char *imagePath) {
  UIImageView *view = (UIImageView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view setImage:[UIImage
    imageWithContentsOfFile:[NSString stringWithUTF8String:imagePath]]];
  return 0;
}

int shareCreateEditTextView(int handle, int parentHandle) {
  UITextField *view = [[UITextField alloc] init];
  [objcHandleMap
    setObject:view
    forKey:[NSNumber numberWithInt:handle]];
  addViewToParent(view, parentHandle);
  view.tag = handle;
  return 0;
}

int shareShowSoftInputOnView(int handle) {
  UITextField *view = (UITextField *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view becomeFirstResponder];
  return 0;
}

int shareHideSoftInputOnView(int handle) {
  [objcContainer endEditing:YES];
  return 0;
}

int shareGetViewText(int handle, char *buf, int bufSize) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      phoneCopyString(buf, bufSize, [view.text UTF8String]);
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      phoneCopyString(buf, bufSize, [view.text UTF8String]);
    } break;
  }
  return 0;
}

int shareSetViewInputTypeAsVisiblePassword(int handle) {
  UITextField *view = (UITextField *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.secureTextEntry = NO;
  return 0;
}

int shareSetViewInputTypeAsPassword(int handle) {
  UITextField *view = (UITextField *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.secureTextEntry = YES;
  return 0;
}

int shareSetViewInputTypeAsText(int handle) {
  UITextField *view = (UITextField *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.secureTextEntry = NO;
  return 0;
}

int shareEnableViewClickEvent(int handle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  UITapGestureRecognizer *press = [[UITapGestureRecognizer
    alloc] initWithTarget:objcDelegate
    action:@selector(dispatchPressGesture:)];
  press.numberOfTapsRequired = 1;
  press.numberOfTouchesRequired = 1;
  //[press setDelegate:objcDelegate];
  [view addGestureRecognizer:press];
  return 0;
}

int shareEnableViewLongClickEvent(int handle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer
    alloc] initWithTarget:objcDelegate
    action:@selector(dispatchLongPressGesture:)];
  //[longPress setDelegate:objcDelegate];
  [view addGestureRecognizer:longPress];
  return 0;
}

int shareEnableViewValueChangeEvent(int handle) {
  switch (pHandle(handle)->type) {
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      [view addTarget:objcDelegate
        action:@selector(dispatchTextFieldDidChange:)
        forControlEvents:UIControlEventEditingChanged];
    } break;
  }
  return 0;
}

int shareEnableViewTouchEvent(int handle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  PhoneForTouchDetectRecognizer *recognizer = [[PhoneForTouchDetectRecognizer
    alloc] initWithTarget:view];
  [view addGestureRecognizer:recognizer];
  return 0;
}

int shareGetThreadId(void) {
  uint64_t tid = 0;
  pthread_threadid_np(0, &tid);
  return (int)tid;
}

int shareSetViewCornerRadius(int handle, float radius) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  view.layer.cornerRadius = radius;
  view.layer.masksToBounds = YES;
  return 0;
}

int shareSetViewBorderColor(int handle, unsigned int color) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setBorderColor:
    [objcDelegate makeColor:(0xff000000 | color)].CGColor];
  return 0;
}

int shareSetViewBorderWidth(int handle, float width) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setBorderWidth:(CGFloat)width];
  return 0;
}

int shareIsLandscape(void) {
  UIInterfaceOrientation orient = [[UIDevice currentDevice] orientation];
  if (UIInterfaceOrientationIsLandscape(orient)) {
    return 1;
  }
  return 0;
}

int shareSetStatusBarBackgroundColor(unsigned int color) {
  objcWindow.backgroundColor = [objcDelegate makeColor:(0xff000000 | color)];
  return 0;
}

static int phoneViewAlignToIos(int align) {
  switch (align) {
    case PHONE_VIEW_ALIGN_CENTER:
      return UITextAlignmentCenter;
    case PHONE_VIEW_ALIGN_LEFT:
      return UITextAlignmentLeft;
    case PHONE_VIEW_ALIGN_RIGHT:
      return UITextAlignmentRight;
  }
  return UITextAlignmentCenter;
}

int shareSetViewAlign(int handle, int align) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      view.textAlignment = phoneViewAlignToIos(align);
    } break;
  }
  return 0;
}

int shareSetViewVerticalAlign(int handle, int align) {
  // TODO:
  return 0;
}

int shareCreateTableView(int style, int handle, int parentHandle) {
  PhoneTableView *view = [[PhoneTableView alloc]
    initWithStyle:phoneTableViewStyleToIos(style)];
  [objcHandleMap
    setObject:view
    forKey:[NSNumber numberWithInt:handle]];
  addViewToParent(view, parentHandle);
  view.tag = handle;
  return 0;
}

int shareReloadTableView(int handle) {
  UITableView *view = (UITableView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view reloadData];
  return 0;
}

int shareSetViewShadowColor(int handle, unsigned int color) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setMasksToBounds:NO];
  [view.layer setShadowColor:[objcDelegate
    makeColor:(0xff000000 | color)].CGColor];
  return 0;
}

int shareSetViewShadowOffset(int handle, float offsetX, float offsetY) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setShadowOffset:CGSizeMake(5, 5)];
  return 0;
}

int shareSetViewShadowOpacity(int handle, float opacity) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setShadowOpacity:(CGFloat)opacity];
  return 0;
}

int shareSetViewShadowRadius(int handle, float radius) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view.layer setShadowRadius:(CGFloat)radius];
  return 0;
}

int shareSetViewBackgroundImageRepeat(int handle, int repeat) {
  // TODO:
  return 0;
}

int shareSetViewFontBold(int handle, int bold) {
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      if (bold) {
        [view setFont:[UIFont boldSystemFontOfSize:view.font.pointSize]];
      } else {
        [view setFont:[UIFont systemFontOfSize:view.font.pointSize]];
      }
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      if (bold) {
        [view setFont:[UIFont boldSystemFontOfSize:view.font.pointSize]];
      } else {
        [view setFont:[UIFont systemFontOfSize:view.font.pointSize]];
      }
    } break;
  }
  return 0;
}
