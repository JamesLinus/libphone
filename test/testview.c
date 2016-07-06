#include "test.h"

void testContainerViewChangeParent(testItem *item) {
  int handle = phoneCreateContainerView(0, 0);
  int newParent = phoneCreateContainerView(0, 0);
  phoneSetViewParent(handle, newParent);
  if (newParent == phoneGetViewParent(handle)) {
    testSucceed(item);
  } else {
    testFail(item, "should be %d but %d", newParent,
      phoneGetViewParent(handle));
  }
}

void testRemoveView(testItem *item) {
  int handle = phoneCreateContainerView(0, 0);
  phoneRemoveView(handle);
  testSucceed(item);
}
