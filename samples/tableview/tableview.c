#include "libphone.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define BACKGROUND_COLOR 0xefefef
#define ROW_HEIGHT dp(90)
#define ROW_IMAGE_SIZE dp(50)
#define REFRESH_ICON_SIZE dp(48)
#define ROW_BORDER_COLOR 0xaaaaaa
#define NAME_COLOR 0x333333
#define DETAIL_COLOR 0x333333
#define TIME_COLOR 0xadadad

static int backgroundView = 0;
static int tableView = 0;
static int topView = 0;
static int refreshAnimationTimer = 0;
static float refreshAnimationLastRotateDegree = 0;
static int refreshAnimationView = 0;

static void layout(void);

static void stopRefreshAnimation(void) {
  if (refreshAnimationTimer) {
    phoneRemoveTimer(refreshAnimationTimer);
    refreshAnimationTimer = 0;
  }
}

static void playRefreshAnimation(int timer) {
  refreshAnimationLastRotateDegree -= 36;
  phoneRotateView(refreshAnimationView, refreshAnimationLastRotateDegree);
  if (refreshAnimationLastRotateDegree <= -1800) {
    phoneEndTableViewRefresh(tableView);
    stopRefreshAnimation();
  }
}

static void startRefreshAnimation(void) {
  if (!refreshAnimationTimer) {
    refreshAnimationTimer = phoneCreateTimer(100,
      playRefreshAnimation);
  }
}

static void appShowing(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app showing");
}

static void appHiding(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app hiding");
}

static void appTerminating(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app terminating");
}

static int appBackClick(void) {
  return PHONE_DONTCARE;
}

static void appLayoutChanging(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "app orientationChanging");
  layout();
}

typedef struct cellContext {
  int iconView;
  int nameView;
  int detailView;
  int timeView;
  int bottomLineView;
} cellContext;

const char *someNames[] = {
  "Anna Blum",
  "Irea Nathan",
  "Christopher Ogden",
  "Gavin",
  "Kimberly Hardman",
  "Kaylee Morrison"
};

const char *someDetails[] = {
  "This sounded nonsense to Alice, so she said nothing, but set off at once toward...",
  "Thus much I thought proper to tell you in relation to yourself, and to the trust I...",
  "OK!",
  "This sounded a very good reason, and Alice was quite pleased to know it. ",
  "It was some time before he obtained any answer, and the reply, when made, was...",
  "To these in the morning I sent the captain, who was to enter into a parley..."
};

const char *someTimes[] = {
  "1:08 PM",
  "YESTERDAY",
  "APRIL 22",
  "APRIL 21"
};

const char *somePhotos[] = {
  "photo01.png",
  "photo02.png",
  "photo03.png",
  "photo04.png",
  "photo05.png",
  "photo06.png"
};

const char *getCellNameByRow(int row) {
  return someNames[row % (sizeof(someNames) / sizeof(someNames[0]))];
}

const char *getCellDetailByRow(int row) {
  return someDetails[row % (sizeof(someDetails) / sizeof(someDetails[0]))];
}

const char *getCellTimeByRow(int row) {
  return someTimes[row % (sizeof(someTimes) / sizeof(someTimes[0]))];
}

const char *getCellPhotoByRow(int row) {
  return somePhotos[row % (sizeof(somePhotos) / sizeof(somePhotos[0]))];
}

static int onCellEvent(int handle, int eventType, void *param) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "table event:%s",
             phoneViewEventTypeToName(eventType));
    return PHONE_DONTCARE;
}

static int onTableEvent(int handle, int eventType, void *param) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "table event:%s",
    phoneViewEventTypeToName(eventType));
  switch (eventType) {
    case PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER_TYPE_COUNT:
      return 1;
    case PHONE_VIEW_REQUEST_TABLE_SECTION_COUNT:
      return 1;
    case PHONE_VIEW_REQUEST_TABLE_ROW_COUNT:
      return 10;
    case PHONE_VIEW_REQUEST_TABLE_ROW_HEIGHT:
      return ROW_HEIGHT;
    case PHONE_VIEW_REQUEST_TABLE_REFRESH_VIEW: {
      int refreshView;
      int refreshIconView;
      refreshView = phoneCreateContainerView(0, 0);
      refreshIconView = phoneCreateContainerView(refreshView, 0);
      refreshAnimationView = refreshIconView;
      phoneSetViewFrame(refreshView, 0, 0, phoneGetViewWidth(0),
        phoneGetTableViewStableRefreshHeight() +
          phoneGetTableViewStableRefreshHeight());
      phoneSetViewFrame(refreshIconView,
        (phoneGetViewWidth(0) - REFRESH_ICON_SIZE) / 2,
        (phoneGetTableViewStableRefreshHeight() - REFRESH_ICON_SIZE) / 2,
        REFRESH_ICON_SIZE, REFRESH_ICON_SIZE);
      phoneSetViewBackgroundImageResource(refreshIconView, "refreshing.png");
      phoneSetHandleTag(refreshView, refreshIconView);
      return refreshView;
    } break;
    case PHONE_VIEW_REQUEST_TABLE_REFRESH: {
      startRefreshAnimation();
    } break;
    case PHONE_VIEW_REQUEST_TABLE_UPDATE_REFRESH_VIEW: {
      phoneViewRequestTable *request = (phoneViewRequestTable *)param;
      int iconView = (int)phoneGetHandleTag(request->renderHandle);
      if (!refreshAnimationTimer) {
        float degree = -360 * phoneGetTableViewRefreshHeight(tableView) /
          (phoneGetTableViewStableRefreshHeight() * 2);
        refreshAnimationLastRotateDegree = degree;
        phoneRotateView(iconView, degree);
      }
    } break;
    case PHONE_VIEW_REQUEST_TABLE_CELL_IDENTIFIER: {
      phoneViewRequestTable *request = (phoneViewRequestTable *)param;
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "request section %d row %d identifier",
        request->section, request->row);
      return phoneCopyString(request->buf, request->bufSize, "testCell");
    } break;
    case PHONE_VIEW_REQUEST_TABLE_CELL_CUSTOM_VIEW: {
      phoneViewRequestTable *request = (phoneViewRequestTable *)param;
      int customView = phoneCreateContainerView(0, onCellEvent);
      cellContext *cell = (cellContext *)calloc(1, sizeof(cellContext));
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
          "custom view(handle:%d) created for section %d row %d",
          customView, request->section, request->row);
      phoneSetViewFrame(customView, 0, 0, phoneGetViewWidth(0),
        ROW_HEIGHT);
      phoneSetHandleTag(customView, cell);

      cell->iconView = phoneCreateContainerView(customView, 0);
      phoneSetViewFrame(cell->iconView,
        dp(15), (ROW_HEIGHT - ROW_IMAGE_SIZE) / 2,
        ROW_IMAGE_SIZE, ROW_IMAGE_SIZE);
      phoneSetViewCornerRadius(cell->iconView, ROW_IMAGE_SIZE / 2);

      cell->bottomLineView = phoneCreateContainerView(customView, 0);
      phoneSetViewFrame(cell->bottomLineView,
          0, ROW_HEIGHT - dp(1), phoneGetViewWidth(0), dp(1));
      phoneSetViewBackgroundColor(cell->bottomLineView,
          ROW_BORDER_COLOR);

      cell->nameView = phoneCreateTextView(customView, 0);
      phoneSetViewFrame(cell->nameView, dp(80), dp(15),
        phoneGetViewWidth(0) / 2, dp(20));
      phoneSetViewFontColor(cell->nameView, NAME_COLOR);
      phoneSetViewFontBold(cell->nameView, 1);
      phoneSetViewAlign(cell->nameView, PHONE_VIEW_ALIGN_LEFT);

      cell->detailView = phoneCreateTextView(customView, 0);
      phoneSetViewFrame(cell->detailView, dp(80), dp(40),
        dp(220), dp(40));
      phoneSetViewFontSize(cell->detailView, dp(14));
      phoneSetViewFontColor(cell->detailView, DETAIL_COLOR);
      phoneSetViewAlign(cell->detailView, PHONE_VIEW_ALIGN_LEFT);

      cell->timeView = phoneCreateTextView(customView, 0);
      phoneSetViewFrame(cell->timeView, phoneGetViewWidth(0) - dp(100) - dp(10),
        dp(13), dp(100), dp(9));
      phoneSetViewFontSize(cell->timeView, dp(9));
      phoneSetViewFontColor(cell->timeView, TIME_COLOR);
      phoneSetViewAlign(cell->timeView, PHONE_VIEW_ALIGN_RIGHT);
      return customView;
    } break;
    case PHONE_VIEW_REQUEST_TABLE_CELL_RENDER: {
      phoneViewRequestTable *request = (phoneViewRequestTable *)param;
      cellContext *cell = phoneGetHandleTag(request->renderHandle);
      phoneSetViewText(cell->nameView, getCellNameByRow(request->row));
      phoneSetViewText(cell->detailView, getCellDetailByRow(request->row));
      phoneSetViewText(cell->timeView, getCellTimeByRow(request->row));
      phoneSetViewBackgroundImageResource(cell->iconView,
          getCellPhotoByRow(request->row));
    } break;
  }
  return PHONE_DONTCARE;
}

static int onTopViewEvent(int handle, int eventType, void *param) {
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "top event:%s",
             phoneViewEventTypeToName(eventType));
    return PHONE_DONTCARE;
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating,
    appBackClick,
    appLayoutChanging
  };
  phoneSetAppNotificationHandler(&handler);

  phoneSetStatusBarBackgroundColor(BACKGROUND_COLOR);
  backgroundView = phoneCreateContainerView(0, 0);
  phoneSetViewBackgroundColor(backgroundView, BACKGROUND_COLOR);

  tableView = phoneCreateTableView(backgroundView, onTableEvent);

  layout();
  return 0;
}

void layout(void) {
  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "display size: %f x %f",
    phoneGetViewWidth(0), phoneGetViewHeight(0));

  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneSetViewFrame(tableView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneReloadTableView(tableView);
}
