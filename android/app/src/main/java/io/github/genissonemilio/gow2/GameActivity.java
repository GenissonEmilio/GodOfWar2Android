package io.github.genissonemilio.gow2;
import android.app.*;
import android.content.*;
import android.os.*;
import android.view.*;
import android.widget.*;
public final class GameActivity extends NativeActivity {
    static { System.loadLibrary("ps2EntryRunner"); }
    static native void nativeConfigure(String directory,int scale);
    static native void nativeInput(int buttons,float lx,float ly,float rx,float ry);
    static native void nativePause(boolean paused);
    static native long nativeFrames();
    private final Handler meter=new Handler(android.os.Looper.getMainLooper());
    private long lastFrames,lastTime;
    private final Runnable sample=new Runnable(){public void run(){long now=android.os.SystemClock.elapsedRealtime(),frames=nativeFrames();if(touch!=null && lastTime>0){touch.gameFps=(frames-lastFrames)*1000f/(now-lastTime);touch.invalidate();}lastTime=now;lastFrames=frames;meter.postDelayed(this,1000);}};
    static native void nativeSettings(int volume,int fps,float deadzone,float sensitivity);
    private TouchOverlay touch; private boolean menu,background,childMenu;
    private int padButtons; private float lx,ly,rx,ry; private boolean controller;
    android.content.SharedPreferences prefs(){return getSharedPreferences("mobile",0);}
    @Override public void onCreate(Bundle state){
        nativeConfigure(getExternalFilesDir(null).getAbsolutePath(),prefs().getInt("scale",1));
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        meter.postDelayed(sample,1000);
        touch=new TouchOverlay(this);addContentView(touch,new android.view.ViewGroup.LayoutParams(-1,-1));applySettings();
    }
    private void applySettings(){nativeSettings(prefs().getInt("volume",80),prefs().getInt("fps",60),prefs().getInt("deadzone",14)/100f,prefs().getInt("sensitivity",100)/100f);if(touch!=null)touch.invalidate();}
    void sendTouch(int buttons,float x,float y,float dx,float dy){controller=false;padButtons=0;nativeInput(buttons,x,y,dx,dy);touch.setController(false);}
    void pauseMenu(){if(menu)return;menu=true;nativePause(true);if(touch!=null)touch.clear();
        String[] actions={"Continuar","Touch, controle e desempenho","Editar posição dos controles","Restaurar layout","Voltar ao início"};
        new AlertDialog.Builder(this).setTitle("GOD OF WAR II • PAUSA").setItems(actions,(d,index)->{
            if(index==1){childMenu=true;MobileUi.settings(this,this::applySettings).setOnDismissListener(x->{childMenu=false;nativePause(background || (touch!=null && touch.editing));});}
            else if(index==2){touch.editing=true;touch.invalidate();}
            else if(index==3){prefs().edit().remove("layout").apply();touch.resetLayout();}
            else if(index==4){finish();}
        }).setOnDismissListener(d->{menu=false;if(!background && !childMenu && !touch.editing)nativePause(false);}).show();
    }
    @Override public void onBackPressed(){if(touch!=null && touch.editing){touch.editing=false;touch.saveLayout();touch.invalidate();nativePause(background || menu);return;}pauseMenu();}
    @Override protected void onPause(){background=true;nativePause(true);if(touch!=null)touch.clear();super.onPause();}
    @Override protected void onResume(){super.onResume();background=false;if(!menu && !childMenu && (touch==null || !touch.editing))nativePause(false);}
    @Override protected void onDestroy(){meter.removeCallbacks(sample);nativePause(false);super.onDestroy();}
    private int bit(int key){switch(key){case KeyEvent.KEYCODE_BUTTON_A:return 0x4000;case KeyEvent.KEYCODE_BUTTON_B:return 0x2000;case KeyEvent.KEYCODE_BUTTON_X:return 0x8000;case KeyEvent.KEYCODE_BUTTON_Y:return 0x1000;case KeyEvent.KEYCODE_BUTTON_L1:return 0x400;case KeyEvent.KEYCODE_BUTTON_R1:return 0x800;case KeyEvent.KEYCODE_BUTTON_L2:return 0x100;case KeyEvent.KEYCODE_BUTTON_R2:return 0x200;case KeyEvent.KEYCODE_BUTTON_START:return 8;case KeyEvent.KEYCODE_BUTTON_SELECT:return 1;case KeyEvent.KEYCODE_BUTTON_THUMBL:return 2;case KeyEvent.KEYCODE_BUTTON_THUMBR:return 4;case KeyEvent.KEYCODE_DPAD_UP:return 16;case KeyEvent.KEYCODE_DPAD_RIGHT:return 32;case KeyEvent.KEYCODE_DPAD_DOWN:return 64;case KeyEvent.KEYCODE_DPAD_LEFT:return 128;default:return 0;}}
    @Override public boolean dispatchKeyEvent(KeyEvent e){
        if(e.getKeyCode()==KeyEvent.KEYCODE_BACK){if(e.getAction()==KeyEvent.ACTION_UP)onBackPressed();return true;}
        int mask=bit(e.getKeyCode());if(mask!=0 && ((e.getSource()&InputDevice.SOURCE_GAMEPAD)==InputDevice.SOURCE_GAMEPAD || (e.getSource()&InputDevice.SOURCE_JOYSTICK)==InputDevice.SOURCE_JOYSTICK)){
            controller=true;if(e.getAction()==KeyEvent.ACTION_DOWN)padButtons|=mask;else padButtons&=~mask;
            nativeInput(padButtons,lx,ly,rx,ry);if(touch!=null)touch.setController(true);return true;}
        return super.dispatchKeyEvent(e);
    }
    @Override public boolean onGenericMotionEvent(android.view.MotionEvent e){
        if((e.getSource()&InputDevice.SOURCE_JOYSTICK)!=0 && e.getAction()==MotionEvent.ACTION_MOVE){
            controller=true;lx=e.getAxisValue(MotionEvent.AXIS_X);ly=e.getAxisValue(MotionEvent.AXIS_Y);rx=e.getAxisValue(MotionEvent.AXIS_Z);ry=e.getAxisValue(MotionEvent.AXIS_RZ);
            padButtons&=~(16|32|64|128|0x100|0x200);
            float hx=e.getAxisValue(MotionEvent.AXIS_HAT_X),hy=e.getAxisValue(MotionEvent.AXIS_HAT_Y);
            if(hx<-.5)padButtons|=128;if(hx>.5)padButtons|=32;if(hy<-.5)padButtons|=16;if(hy>.5)padButtons|=64;
            if(e.getAxisValue(MotionEvent.AXIS_LTRIGGER)>.5)padButtons|=0x100;if(e.getAxisValue(MotionEvent.AXIS_RTRIGGER)>.5)padButtons|=0x200;
            nativeInput(padButtons,lx,ly,rx,ry);if(touch!=null)touch.setController(true);return true;}
        return super.onGenericMotionEvent(e);
    }
}
