#include "libphone.h"
#include "libphoneprivate.h"
#include <pthread.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <QuartzCore/QuartzCore.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

phoneAppDelegate *objcDelegate = nil;
NSMutableDictionary *objcHandleMap = nil;
UIWindow *objcWindow = nil;
UIImageView *objcContainer = nil;
CGFloat statusBarSize = 0;

///////////////////////////////////////////////////////////////////////////////

@interface UselessViewController : UIViewController
@end

@implementation UselessViewController

- (void)viewDidLoad {
  [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
}

- (BOOL)shouldAutorotate {
  return YES;
}

- (BOOL)prefersStatusBarHidden {
  return NO;
}

- (BOOL)becomeFirstResponder {
  return YES;
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event {
  if (UIEventSubtypeMotionShake == motion) {
    shareDispatchShake();
  }
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

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  int handle = (int)((UIView *)self.target).tag;
  phoneHandle *handleData = pHandle(handle);
  [super touchesBegan:touches withEvent:event];
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "touchesBegan");
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

@interface PhoneOpenGLView : GLKView
@property (nonatomic, strong) CADisplayLink *displayLink;
@end

@implementation PhoneOpenGLView

- (void)render:(CADisplayLink *)displayLink {
  [self display];
}

@end

///////////////////////////////////////////////////////////////////////////////

@interface PhoneTableView : UITableView <UITableViewDelegate, UITableViewDataSource>
@property (nonatomic, strong) UIRefreshControl *refreshControl;
@property (nonatomic, strong) UIView *refreshOverlayView;
@end

@implementation PhoneTableView

- (id)initWithStyle:(UITableViewStyle)style {
  CGRect frame = {0, 0, 0, 0};
  self = [super initWithFrame:frame style:style];
  self.dataSource = self;
  self.delegate = self;
  self.tag = 0;
  self.backgroundColor = [UIColor clearColor];
  self.backgroundView = nil;
  self.refreshControl = nil;
  self.separatorStyle = UITableViewCellSeparatorStyleNone;
  return self;
}

- (void)refresh:(UIRefreshControl *)refreshControl {
  shareRequestTableViewRefresh((int)self.tag);
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  if (nil != self.refreshControl) {
    CGRect overlayFrame = self.refreshControl.bounds;
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "overlayFrame: %f,%f",
      self.refreshControl.bounds.size.width,
      self.refreshControl.bounds.size.height);
    overlayFrame.size.height = -scrollView.contentOffset.y;
    self.refreshOverlayView.frame = overlayFrame;
    shareRequestTableViewUpdateRefreshView((int)self.tag,
      (int)self.refreshOverlayView.tag);
  }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
  int refreshHandle;
  int handle = (int)tableView.tag;
  if (0 == handle) {
    return 0;
  }

  if (nil == self.refreshControl) {
    refreshHandle = shareRequestTableViewRefreshView(handle);
    if (0 != refreshHandle) {
      UIView *refreshView = (UIView *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:refreshHandle]];
      self.refreshControl = [[UIRefreshControl alloc] init];
      self.refreshControl.backgroundColor = [UIColor clearColor];
      self.refreshOverlayView = [[UIView alloc] init];
      self.refreshOverlayView.backgroundColor = [UIColor lightGrayColor];
      self.refreshOverlayView.layer.masksToBounds = YES;
      [self.refreshControl addTarget:objcDelegate
        action:@selector(dispatchTableViewRefresh:)
        forControlEvents:UIControlEventValueChanged];
      [self addSubview:self.refreshControl];
      [self.refreshControl addSubview:self.refreshOverlayView];
      [self.refreshOverlayView addSubview:refreshView];
      self.refreshControl.tag = handle;
      self.refreshOverlayView.tag = refreshHandle;
    }
  }

  return shareRequestTableViewSectionCount(handle);
}

- (NSInteger)tableView:(UITableView *)tableView
    numberOfRowsInSection:(NSInteger)section {
  int handle = (int)tableView.tag;
  if (0 == handle) {
    return 0;
  }
  return shareRequestTableViewRowCount(handle, (int)section);
}

- (void)tableView: (UITableView *)tableView
	  didSelectRowAtIndexPath: (NSIndexPath *)indexPath {
  int handle = (int)tableView.tag;
  shareRequestTableViewCellClick(handle, (int)indexPath.section,
    (int)indexPath.row, [(PhoneTableView *)tableView
      getRenderHandleFromIndexPath:indexPath]);
}

- (CGFloat)tableView:(UITableView *)tableView
    heightForRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = (int)tableView.tag;
  return (CGFloat)shareRequestTableViewRowHeight(handle,
    (int)indexPath.section, (int)indexPath.row);
}

- (NSString *)tableView:(UITableView *)tableView
    titleForHeaderInSection:(NSInteger)section {
  return nil;
}

- (NSString *)tableView:(UITableView *)tableView
    titleForFooterInSection:(NSInteger)section {
  return nil;
}

- (int)getRenderHandleFromCell:(UITableViewCell *)cell {
  if (nil != cell && nil != cell.contentView) {
    for (UIView *loopView in cell.contentView.subviews) {
      if (loopView.tag > 0) {
        return (int)loopView.tag;
      }
    }
  }
  return 0;
}

- (int)getRenderHandleFromIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [self cellForRowAtIndexPath:indexPath];
  if (nil != cell) {
    return [self getRenderHandleFromCell:cell];
  }
  return 0;
}

- (void)tableView:(UITableView *)tableView
    willDisplayCell:(UITableViewCell *)cell
    forRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = (int)tableView.tag;
  if (0 == handle) {
    return;
  }
  shareRequestTableViewCellRender(handle, (int)indexPath.section,
    (int)indexPath.row, [(PhoneTableView *)tableView
      getRenderHandleFromCell:cell]);
}

- (UITableViewCell *)tableView:(UITableView *)tableView
    cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  int handle = (int)tableView.tag;
  char buf[4096];
  UITableViewCell *cell;
  NSString *cellIdentifier;
  shareRequestTableViewCellIdentifier(handle, (int)indexPath.section,
      (int)indexPath.row, buf, sizeof(buf));
  cellIdentifier = [NSString stringWithUTF8String:buf];
  cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
  if (nil == cell) {
    int customView;
    cell = [[UITableViewCell alloc]
        initWithStyle:UITableViewCellStyleDefault
        reuseIdentifier:cellIdentifier];
    customView = shareRequestTableViewCellCustomView(handle,
        (int)indexPath.section, (int)indexPath.row);
    if (customView) {
      [cell.contentView addSubview:(UIView *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:customView]]];
    }
    cell.accessoryType = UITableViewCellAccessoryNone;
  }
  cell.selectionStyle = UITableViewCellSelectionStyleNone;
  return cell;
}

@end

///////////////////////////////////////////////////////////////////////////////

@implementation phoneAppDelegate

- (void)layoutContainer {
  CGRect containerFrame = [[UIScreen mainScreen] bounds];
  UIInterfaceOrientation orient =
    [UIApplication sharedApplication].statusBarOrientation;
  switch (orient) {
    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
      containerFrame.origin.y += statusBarSize;
      containerFrame.size.height -= statusBarSize;
      break;
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
      containerFrame.origin.y += statusBarSize;
      containerFrame.size.height -= statusBarSize;
      break;
    default:
      assert(0 && "Unknown orientation");
  }
  objcContainer.frame = containerFrame;
}

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  UselessViewController *controller = [[UselessViewController alloc] init];
  statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size.height;
  if (statusBarSize > 100) {
    statusBarSize = 20;
  }
  objcDelegate = self;
  _handleMap = [[NSMutableDictionary alloc] init];
  objcHandleMap = _handleMap;
  _container = [[UIImageView alloc] init];
  _container.backgroundColor = [UIColor whiteColor];
  _container.userInteractionEnabled = YES;
  objcContainer = _container;
  controller.view = [[UIView alloc] initWithFrame:[[UIScreen
      mainScreen] bounds]];
  [controller.view addSubview:_container];
  _window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  _window.backgroundColor = [UIColor whiteColor];
  _window.userInteractionEnabled = YES;
  _window.rootViewController = controller;
  objcWindow = _window;
  [self layoutContainer];
  [[NSNotificationCenter defaultCenter] addObserver:self
      selector:@selector(statusBarOrientationChange:)
      name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
  phoneInitApplication();
  phoneMain(0, 0);
  [_window makeKeyAndVisible];
  return YES;
}

- (void)statusBarOrientationChange:(NSNotification *)notification {
  [self layoutContainer];
  if (pApp->handler->layoutChanging) {
    pApp->handler->layoutChanging();
  }
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

- (void)dispatchTableViewRefresh:(UIRefreshControl *)view {
  int handle = (int)view.tag;
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.eventHandler) {
    handleData->u.view.eventHandler(handle,
      PHONE_VIEW_REQUEST_TABLE_REFRESH, 0);
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

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
  int handle = (int)view.tag;
  if (nil == view.context) {
    return;
  }
  phoneHandle *handleData = pHandle(handle);
  if (handleData->u.view.u.opengl.renderHandler) {
    handleData->u.view.u.opengl.renderHandler(handle);
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
  if (level >= PHONE_LOG_INFO) {
    NSLog(@"%@", [NSString stringWithUTF8String:log]);
  } else {
    printf("%s %s %.*s\n", phoneLogLevelToString(level), tag, len, log);
  }
  return 0;
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
  view.lineBreakMode = UILineBreakModeWordWrap;
  view.numberOfLines = 0;
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
  // no-op
  return 0;
}

int shareAddViewAnimationToSet(int animationHandle, int setHandle) {
  // no-op
  return 0;
}

int shareRemoveViewAnimationSet(int handle) {
  // no-op
  return 0;
}

int shareRemoveViewAnimation(int handle) {
  // no-op
  return 0;
}

int shareCreateViewTranslateAnimation(int handle, int viewHandle,
    float offsetX, float offsetY) {
  // no-op
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
  // no-op
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
  float realFontSize = fontSize * 0.9;
  switch (pHandle(handle)->type) {
    case PHONE_TEXT_VIEW: {
      UILabel *view = (UILabel *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      [view setFont:[UIFont systemFontOfSize:(GLfloat)realFontSize]];
      if ([view.font.fontName isEqualToString:[UIFont
          systemFontOfSize:(GLfloat)realFontSize].fontName]) {
        [view setFont:[UIFont systemFontOfSize:(GLfloat)realFontSize]];
      } else {
        [view setFont:[UIFont boldSystemFontOfSize:(GLfloat)realFontSize]];
      }
    } break;
    case PHONE_EDIT_TEXT_VIEW: {
      UITextField *view = (UITextField *)[objcHandleMap
        objectForKey:[NSNumber numberWithInt:handle]];
      if ([view.font.fontName isEqualToString:[UIFont
          systemFontOfSize:(GLfloat)realFontSize].fontName]) {
        [view setFont:[UIFont systemFontOfSize:(GLfloat)realFontSize]];
      } else {
        [view setFont:[UIFont boldSystemFontOfSize:(GLfloat)realFontSize]];
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
  UIInterfaceOrientation orient =
    [UIApplication sharedApplication].statusBarOrientation;
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

int shareCreateTableView(int handle, int parentHandle) {
  PhoneTableView *view = [[PhoneTableView alloc]
    initWithStyle:UITableViewStylePlain];
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

int shareBeginTableViewRefresh(int handle) {
  PhoneTableView *view = (PhoneTableView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  if (nil != view.refreshControl) {
    [view.refreshControl beginRefreshing];
  }
  return 0;
}

int shareEndTableViewRefresh(int handle) {
  PhoneTableView *view = (PhoneTableView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  if (nil != view.refreshControl) {
    [view.refreshControl endRefreshing];
  }
  return 0;
}

float shareGetTableViewRefreshHeight(int handle) {
  PhoneTableView *view = (PhoneTableView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  return -view.contentOffset.y;
}

int shareRotateView(int handle, float degree) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  CGAffineTransform transform =
    CGAffineTransformMakeRotation(M_PI * degree / 180);
  view.transform = transform;
  return 0;
}

int shareSetEditTextViewPlaceholder(int handle, const char *text,
    unsigned int color) {
  UITextField *view = (UITextField *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  if ([view respondsToSelector:@selector(setAttributedPlaceholder:)]) {
    view.attributedPlaceholder = [[NSAttributedString alloc]
      initWithString:[NSString stringWithUTF8String:text]
      attributes:@{NSForegroundColorAttributeName:[objcDelegate
        makeColor:(0xff000000 | color)]}];
  } else {
    view.placeholder = [NSString stringWithUTF8String:text];
  }
  return 0;
}

int shareSetViewParent(int handle, int parentHandle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  UIView *newParentView = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:parentHandle]];
  [newParentView addSubview:view];
  return 0;
}

int shareRemoveView(int handle) {
  phoneHandle *handleData = pHandle(handle);
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  [view removeFromSuperview];
  [objcHandleMap removeObjectForKey:[NSNumber numberWithInt:handle]];
  if (PHONE_OPENGL_VIEW == handleData->type) {
    PhoneOpenGLView *glView = (PhoneOpenGLView *)view;
    if (nil != glView.displayLink) {
      [glView.displayLink invalidate];
      glView.displayLink = nil;
    }
    if ([EAGLContext currentContext] == glView.context) {
      [EAGLContext setCurrentContext:nil];
    }
  }
  return 0;
}

int shareCreateOpenGLView(int handle, int parentHandle) {
  PhoneOpenGLView *view = [[PhoneOpenGLView alloc] init];
  [objcHandleMap
    setObject:view
    forKey:[NSNumber numberWithInt:handle]];
  addViewToParent(view, parentHandle);
  view.tag = handle;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
  view.context = nil;
#pragma clang diagnostic pop
  view.enableSetNeedsDisplay = NO;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  view.delegate = objcDelegate;
  view.displayLink = nil;
  return 0;
}

int shareBeginOpenGLViewRender(int handle,
    phoneOpenGLViewRenderHandler renderHandler) {
  PhoneOpenGLView *view = (PhoneOpenGLView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  if (nil == view.context) {
    view.context = [[EAGLContext alloc]
      initWithAPI:kEAGLRenderingAPIOpenGLES2];
  }
  if (nil != view.context) {
    [EAGLContext setCurrentContext:view.context];
    view.displayLink = [CADisplayLink
      displayLinkWithTarget:view
      selector:@selector(render:)];
    [view.displayLink addToRunLoop:[NSRunLoop currentRunLoop]
      forMode:NSDefaultRunLoopMode];
  }
  return 0;
}

int shareWorkQueueThreadInit(void) {
  // no-op
  return 0;
}

int shareWorkQueueThreadUninit(void) {
  // no-op
  return 0;
}

typedef struct threadContext {
  int handle;
  char *threadName;
  pthread_t threadId;
  phoneThreadRunHandler runHandler;
} threadContext;

static void *runThread(void *arg) {
  @autoreleasepool {
    threadContext *ctx = (threadContext *)arg;
    if (ctx->threadName) {
      [[NSThread currentThread] setName:[NSString
        stringWithUTF8String:ctx->threadName]];
    }
    ctx->runHandler(ctx->handle);
  }
  return 0;
}

int shareCreateThread(int handle, const char *threadName) {
  threadContext *ctx = (threadContext *)shareCalloc(1, sizeof(threadContext));
  phoneHandle *handleData = pHandle(handle);
  if (!ctx) {
    phoneSetHandleContext(handle, 0);
    return -1;
  }
  phoneSetHandleContext(handle, ctx);
  if (threadName) {
    int threadNameLen = (int)strlen(threadName);
    ctx->threadName = shareMalloc(threadNameLen + 1);
    memcpy(ctx->threadName, threadName, threadNameLen + 1);
  }
  ctx->handle = handle;
  ctx->runHandler = handleData->u.thread.runHandler;
  return 0;
}

int shareStartThread(int handle) {
  threadContext *ctx = (threadContext *)phoneGetHandleContext(handle);
  pthread_create(&ctx->threadId, 0, runThread, ctx);
  return 0;
}

int shareJoinThread(int handle) {
  threadContext *ctx = (threadContext *)phoneGetHandleContext(handle);
  if (ctx) {
    void *threadReturn = 0;
    pthread_join(ctx->threadId, &threadReturn);
  }
  return 0;
}

int shareRemoveThread(int handle) {
  threadContext *ctx = (threadContext *)phoneGetHandleContext(handle);
  if (ctx) {
    if (ctx->threadName) {
      free(ctx->threadName);
      ctx->threadName = 0;
    }
    free(ctx);
    phoneSetHandleContext(handle, 0);
  }
  return 0;
}

FILE *shareOpenAsset(const char *filename) {
  return fopen([[[[NSBundle mainBundle] resourcePath]
      stringByAppendingPathComponent:[NSString stringWithUTF8String:filename]]
    UTF8String], "rb");
}

int shareStartShakeDetection(void) {
  // no-op
  return 0;
}

int shareStopShakeDetection(void) {
  // no-op
  return 0;
}

int shareIsShakeSensorSupported(void) {
  return 1;
}

int shareGetViewParent(int handle) {
  UIView *view = (UIView *)[objcHandleMap
    objectForKey:[NSNumber numberWithInt:handle]];
  return (int)[view superview].tag;
}

#pragma clang diagnostic pop
