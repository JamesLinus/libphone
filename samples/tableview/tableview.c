#include "libphone.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define BACKGROUND_COLOR 0xefefef

static int backgroundView = 0;
static int tableView = 0;
static int topView = 0;
static int refreshAnimationTimer = 0;
static float refreshAnimationLastRotateDegree = 0;
static int refreshAnimationView = 0;

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

typedef struct cellContext {
  int iconView;
} cellContext;

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
      return dp(70);
    case PHONE_VIEW_REQUEST_TABLE_REFRESH_VIEW: {
      int refreshView;
      int refreshIconView;
      refreshView = phoneCreateContainerView(0, 0);
      refreshIconView = phoneCreateContainerView(refreshView, 0);
      refreshAnimationView = refreshIconView;
      phoneSetViewFrame(refreshView, 0, 0, phoneGetViewWidth(0),
        phoneGetTableViewStableRefreshHeight() +
          phoneGetTableViewStableRefreshHeight());
      phoneSetViewFrame(refreshIconView, (phoneGetViewWidth(0) - dp(48)) / 2,
        (phoneGetTableViewStableRefreshHeight() - dp(48)) / 2, dp(48), dp(48));
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
        dp(70));
      phoneSetHandleTag(customView, cell);
      cell->iconView = phoneCreateContainerView(customView, 0);
      phoneSetViewFrame(cell->iconView, dp(10), dp(10), dp(50), dp(50));
      phoneSetViewCornerRadius(cell->iconView, dp(25));
      phoneSetViewBackgroundColor(cell->iconView, 0x555555);
      phoneEnableViewEvent(customView, PHONE_VIEW_TOUCH);
      //phoneSetViewBackgroundImageResource(cell->iconView, "asset1.png");
      //phoneSetViewBorderColor(cell->iconView, 0x00ffff);
      //phoneSetViewBorderWidth(cell->iconView, dp(1));
      return customView;
    } break;
    case PHONE_VIEW_REQUEST_TABLE_CELL_RENDER: {
      phoneViewRequestTable *request = (phoneViewRequestTable *)param;
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
          "render view(handle:%d) for section %d row %d",
          request->renderHandle, request->section, request->row);
      if (request->renderHandle) {
        if (request->row % 2) {
          phoneSetViewBackgroundColor(request->renderHandle, 0xefefef);
        } else {
          phoneSetViewBackgroundColor(request->renderHandle, 0xcccccc);
        }
      }
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
    appTerminating
  };
  phoneSetAppNotificationHandler(&handler);

  phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "display size: %f x %f",
    phoneGetViewWidth(0), phoneGetViewHeight(0));

  phoneSetStatusBarBackgroundColor(BACKGROUND_COLOR);
  backgroundView = phoneCreateContainerView(0, 0);
  phoneSetViewFrame(backgroundView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneSetViewBackgroundColor(backgroundView, BACKGROUND_COLOR);

  /*
  topView = phoneCreateContainerView(backgroundView, onTopViewEvent);
  phoneSetViewFrame(topView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0) / 2);
  phoneEnableViewEvent(topView, PHONE_VIEW_TOUCH);*/

  tableView = phoneCreateTableView(backgroundView, onTableEvent);
  phoneSetViewFrame(tableView, 0, 0, phoneGetViewWidth(0),
    phoneGetViewHeight(0));
  phoneReloadTableView(tableView);

  return 0;
}
