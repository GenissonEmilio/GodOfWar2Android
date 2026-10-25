package io.github.genissonemilio.gow2.inputprobe;

import android.app.Instrumentation;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;

/** Optional on-device input smoke test. Run only with a disposable gameplay session open. */
public final class TouchProbe extends Instrumentation {
    private long down;
    private void event(int action,int count,float[] x,float[] y) {
        MotionEvent.PointerProperties[] properties=new MotionEvent.PointerProperties[count];
        MotionEvent.PointerCoords[] coords=new MotionEvent.PointerCoords[count];
        for(int i=0;i<count;i++) {
            properties[i]=new MotionEvent.PointerProperties();properties[i].id=i;properties[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
            coords[i]=new MotionEvent.PointerCoords();coords[i].x=x[i];coords[i].y=y[i];coords[i].pressure=1;coords[i].size=.1f;
        }
        MotionEvent e=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,count,properties,coords,0,0,1,1,0,0,InputDevice.SOURCE_TOUCHSCREEN,0);
        try {if(!getUiAutomation().injectInputEvent(e,true))throw new IllegalStateException("Input rejected");}finally{e.recycle();}
    }
    @Override public void onCreate(Bundle args) {super.onCreate(args);start();}
    @Override public void onStart() {
        Bundle result=new Bundle();
        try {
            down=SystemClock.uptimeMillis();
            event(MotionEvent.ACTION_DOWN,1,new float[]{367},new float[]{934});
            event(MotionEvent.ACTION_MOVE,1,new float[]{470},new float[]{900});
            for(int i=0;i<8;i++) {
                event(MotionEvent.ACTION_POINTER_DOWN|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),2,new float[]{470,2200},new float[]{900,858});
                SystemClock.sleep(350);
                event(MotionEvent.ACTION_POINTER_UP|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),2,new float[]{470,2200},new float[]{900,858});
                SystemClock.sleep(250);
            }
            event(MotionEvent.ACTION_UP,1,new float[]{470},new float[]{900});
            result.putString("stream","Movement held while eight independent attack touches were injected. Inspect gameplay response.\n");
            finish(-1,result);
        } catch(Throwable t) {result.putString("stream",t.toString());finish(0,result);}
    }
}
