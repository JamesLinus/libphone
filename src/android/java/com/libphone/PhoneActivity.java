package com.libphone;

import android.app.Activity;
import android.os.Bundle;
import android.util.SparseArray;
import android.os.Handler;
import android.view.animation.TranslateAnimation;
import android.view.animation.AlphaAnimation;
import android.widget.AbsoluteLayout;
import android.view.ViewGroup;
import android.view.View;
import android.content.Context;
import android.widget.TextView;
import com.libphone.PhoneNotifyThread;
import android.view.animation.AnimationSet;
import android.view.animation.Animation;
import java.util.ArrayList;
import java.util.Iterator;
import android.view.animation.Animation.AnimationListener;
import android.graphics.drawable.Drawable;
import android.widget.EditText;
import android.text.InputType;
import android.view.inputmethod.InputMethodManager;
import android.text.TextWatcher;
import android.text.Editable;
import android.view.MotionEvent;

public class PhoneActivity extends Activity {

    class PhoneContainerView extends AbsoluteLayout {
        public PhoneContainerView(Context context) {
            super(context);
        }
    }

    private PhoneContainerView container;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        container = new PhoneContainerView(this);
        container.setId(0);
        setContentView(container);

        nativeSendAppShowing();

        PhoneNotifyThread notifyThread = new PhoneNotifyThread();
        notifyThread.handler = handler;
        notifyThread.activity = this;
        notifyThread.start();
    }

    public class PhoneNotifyRunnable implements Runnable {
        public long needNotifyMask;
        @Override
        public void run() {
            PhoneActivity.this.nativeNotifyMainThread(needNotifyMask);
        }
    }

    private SparseArray<Object> handleMap = new SparseArray<Object>();
    private Handler handler = new Handler();

    private native int nativeSendAppShowing();
    private native int nativeSendAppHiding();
    private native int nativeSendAppTerminating();
    private native int nativeInvokeTimer(int handle);
    private native int nativeNotifyMainThread(long needNotifyMask);
    private native int nativeAnimationFinished(int handle);
    private native int nativeDispatchViewTouchBeginEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchEndEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchMoveEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchCancelEvent(int handle, int x, int y);

    private Object findHandleObject(int handle) {
        return handleMap.get(handle);
    }

    private void setHandleObject(int handle, Object obj) {
        handleMap.put(handle, obj);
    }

    private void removeHandleObject(int handle) {
        handleMap.remove(handle);
    }

    public class PhoneTimerRunnable implements Runnable {
        public long period;
        public int handle;
        public boolean removed = false;
        public void run() {
            if (removed) {
                return;
            }
            nativeInvokeTimer(handle);
            if (!removed) {
                handler.postDelayed(this, period);
            }
        }
    }

    public int javaCreateTimer(int handle, long milliseconds) {
        PhoneTimerRunnable timerRunnable = new PhoneTimerRunnable();
        timerRunnable.period = milliseconds;
        timerRunnable.handle = handle;
        setHandleObject(handle, timerRunnable);
        handler.postDelayed(timerRunnable, milliseconds);
        return 0;
    }

    public int javaRemoveTimer(int handle) {
        PhoneTimerRunnable timerRunnable = (PhoneTimerRunnable)findHandleObject(handle);
        timerRunnable.removed = true;
        removeHandleObject(handle);
        return 0;
    }

    public int javaCreateContainerView(int handle, int parentHandle) {
        PhoneContainerView view = new PhoneContainerView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaSetViewFrame(int handle, int x, int y, int width, int height) {
        if (0 == handle) {
            return -1;
        }
        View view = (View)findHandleObject(handle);
        view.setLayoutParams(new AbsoluteLayout.LayoutParams(width, height, x, y));
        return 0;
    }

    public int javaSetViewBackgroundColor(int handle, int color) {
        View view = 0 == handle ? container : (View)findHandleObject(handle);
        view.setBackgroundColor(color | 0xff000000);
        return 0;
    }

    public int javaSetViewFontColor(int handle, int color) {
        TextView view = (TextView)findHandleObject(handle);
        view.setTextColor(color | 0xff000000);
        return 0;
    }

    private void addViewToParent(View view, int parentHandle) {
        if (0 != parentHandle) {
            ((ViewGroup)findHandleObject(parentHandle)).addView(view);
        } else {
            container.addView(view);
        }
    }

    public int javaCreateTextView(int handle, int parentHandle) {
        TextView view = new TextView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaSetViewText(int handle, String val) {
        TextView view = (TextView)findHandleObject(handle);
        view.setText(val);
        return 0;
    }

    public int javaShowView(int handle, int display) {
        View view = (View)findHandleObject(handle);
        view.setVisibility(0 == display ? View.GONE : View.VISIBLE);
        return 0;
    }

    public int javaGetViewWidth(int handle) {
        if (0 == handle) {
            return container.getWidth();
        }
        return ((View)findHandleObject(handle)).getWidth();
    }

    public int javaGetViewHeight(int handle) {
        if (0 == handle) {
            return container.getHeight();
        }
        return ((View)findHandleObject(handle)).getHeight();
    }

    public int javaCreateViewAnimationSet(int handle) {
        ArrayList<Integer> animationSet = new ArrayList<Integer>();
        setHandleObject(handle, animationSet);
        return 0;
    }

    public int javaAddViewAnimationToSet(int animationHandle, int setHanle) {
        ArrayList<Integer> set = (ArrayList<Integer>)findHandleObject(setHanle);
        set.add(animationHandle);
        return 0;
    }

    public int javaRemoveViewAnimationSet(int handle) {
        removeHandleObject(handle);
        return 0;
    }

    public int javaRemoveViewAnimation(int handle) {
        removeHandleObject(handle);
        return 0;
    }

    public class PhoneAnimationPair {
        public Animation animation;
        public View view;
    }

    public int javaCreateViewTranslateAnimation(int handle, int viewHandle,
                                                 int offsetX, int offsetY) {
        PhoneAnimationPair pair = new PhoneAnimationPair();
        TranslateAnimation ani = new TranslateAnimation(0, offsetX, 0, offsetY);
        View view = (View)findHandleObject(viewHandle);
        ani.setFillAfter(true);
        final int finalHandle = handle;
        ani.setAnimationListener(new AnimationListener(){
            public void onAnimationStart(Animation ani) {
            };
            public void onAnimationRepeat(Animation ani) {
            };
            public void onAnimationEnd(Animation ani) {
                nativeAnimationFinished(finalHandle);
            };
        });
        pair.animation = ani;
        pair.view = view;
        setHandleObject(handle, pair);
        return 0;
    }

    public int javaCreateViewAlphaAnimation(int handle, int viewHandle,
                                            float fromAlpha, float toAlpha) {
        PhoneAnimationPair pair = new PhoneAnimationPair();
        AlphaAnimation ani = new AlphaAnimation(fromAlpha, toAlpha);
        View view = (View)findHandleObject(viewHandle);
        ani.setFillAfter(true);
        final int finalHandle = handle;
        ani.setAnimationListener(new AnimationListener(){
            public void onAnimationStart(Animation ani) {
            };
            public void onAnimationRepeat(Animation ani) {
            };
            public void onAnimationEnd(Animation ani) {
                nativeAnimationFinished(finalHandle);
            };
        });
        pair.animation = ani;
        pair.view = view;
        setHandleObject(handle, pair);
        return 0;
    }

    public int javaBeginAnimationSet(int handle, int duration) {
        ArrayList<Integer> list = (ArrayList<Integer>)findHandleObject(handle);
        for (Iterator<Integer> it = list.iterator(); it.hasNext();) {
            int animationHandle = (int)it.next();
            PhoneAnimationPair pair = (PhoneAnimationPair)findHandleObject(animationHandle);
            pair.animation.setDuration(duration);
            pair.view.startAnimation(pair.animation);
        }
        return 0;
    }

    public int javaBringViewToFront(int handle) {
        View view = (View)findHandleObject(handle);
        view.bringToFront();
        return 0;
    }

    public int javaSetViewAlpha(int handle, float alpha) {
        View view = (View)findHandleObject(handle);
        AlphaAnimation ani = new AlphaAnimation(alpha, alpha);
        ani.setFillAfter(true);
        ani.setDuration(0);
        view.startAnimation(ani);
        return 0;
    }

    public int javaSetViewFontSize(int handle, int fontSize) {
        TextView view = (TextView)findHandleObject(handle);
        view.setTextSize(fontSize);
        return 0;
    }

    public int javaSetViewBackgroundImageResource(int handle, String imageResource) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        int dotPos = imageResource.indexOf('.');
        String name = -1 == dotPos ? imageResource : imageResource.substring(0, dotPos);
        view.setBackgroundResource(getResources().getIdentifier(name,
                "drawable",
                getPackageName()));
        return 0;
    }

    public int javaSetViewBackgroundImagePath(int handle, String imagePath) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        Drawable drawable = null;
        try {
            drawable = Drawable.createFromPath(imagePath);
        } catch (Throwable t) {
        }
        if (null == drawable) {
            return -1;
        }
        view.setBackgroundDrawable(drawable);
        return 0;
    }

    public int javaCreateEditTextView(int handle, int parentHandle) {
        EditText view = new EditText(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaShowSoftInputOnView(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
        return 0;
    }

    public int javaHideSoftInputOnView(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        return 0;
    }

    public String javaGetViewText(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        return (String)view.getText();
    }

    public int javaSetViewInputTypeAsVisiblePassword(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT |
                InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        return 0;
    }

    public int javaSetViewInputTypeAsPassword(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        return 0;
    }

    public int javaSetViewInputTypeAsText(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT);
        return 0;
    }

    public native int nativeDispatchViewClickEvent(int handle);
    public native int nativeDispatchViewLongClickEvent(int handle);
    public native int nativeDispatchViewValueChangeEvent(int handle);

    public int javaEnableViewClickEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PhoneActivity.this.nativeDispatchViewClickEvent(v.getId());
            }
        });
        return 0;
    }

    public int javaEnableViewLongClickEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                int result = PhoneActivity.this.nativeDispatchViewLongClickEvent(v.getId());
                if (1 == result) {
                    return true;
                }
                return false;
            }
        });
        return 0;
    }

    public int javaEnableViewValueChangeEvent(int handle) {
        View unknownTypeView = (View)findHandleObject(handle);
        String className = unknownTypeView.getClass().getName();
        if (className.equals("android.widget.EditText")) {
            EditText view = (EditText)unknownTypeView;
            final int finalHandle = handle;
            view.addTextChangedListener(new TextWatcher() {
                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }
                @Override
                public void afterTextChanged(Editable s) {
                    PhoneActivity.this.nativeDispatchViewValueChangeEvent(finalHandle);
                }
            });
        }
        return 0;
    }

    public int javaEnableViewTouchEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        return 1 == nativeDispatchViewTouchBeginEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_UP:
                        return 1 == nativeDispatchViewTouchEndEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_MOVE:
                        return 1 == nativeDispatchViewTouchMoveEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_CANCEL:
                        return 1 == nativeDispatchViewTouchCancelEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                }
                return false;
            }
        });
        return 0;
    }
}
