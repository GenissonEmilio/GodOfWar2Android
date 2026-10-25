package io.github.genissonemilio.gow2;
import android.content.*;
import android.graphics.*;
import android.view.*;
import java.util.*;
import org.json.*;
/** Stable pointer ownership supports simultaneous movement, attacks and shoulder buttons. */
final class TouchOverlay extends View {
    static final class Control { String name;int mask;float x,y,r=1;Control(String n,int m,float px,float py){name=n;mask=m;x=px;y=py;} }
    private final GameActivity game;private final Paint p=new Paint(3);
    private final List<Control> controls=new ArrayList<>();
    private final Map<Integer,Control> owners=new HashMap<>();
    private final Map<Integer,PointF> points=new HashMap<>();
    float gameFps; boolean movie;
    boolean editing;private boolean controller;private Control selected; private int presetApplied=-1;
    TouchOverlay(GameActivity activity){super(activity);game=activity;setContentDescription("Controle touch de God of War II");setFocusable(true);resetLayout();}
    void settingsChanged(){if(presetApplied!=game.prefs().getInt("preset",0))resetLayout();invalidate();}
    void setController(boolean c){controller=c;invalidate();}
    void clear(){owners.clear();points.clear();invalidate();}
    void resetLayout(){clear();controls.clear();presetApplied=game.prefs().getInt("preset",0);
        controls.add(new Control("MOVER",0,.14f,.73f));controls.add(new Control("ESQUIVA",0,.72f,.76f));
        controls.add(new Control("□",0x8000,.84f,.67f));controls.add(new Control("△",0x1000,.90f,.51f));controls.add(new Control("○",0x2000,.96f,.67f));controls.add(new Control("✕",0x4000,.90f,.83f));
        controls.add(new Control("L1",0x400,.09f,.18f));controls.add(new Control("L2",0x100,.20f,.18f));controls.add(new Control("R1",0x800,.80f,.18f));controls.add(new Control("R2",0x200,.91f,.18f));
        controls.add(new Control("↑",16,.35f,.68f));controls.add(new Control("←",128,.31f,.79f));controls.add(new Control("→",32,.39f,.79f));controls.add(new Control("↓",64,.35f,.90f));
        controls.add(new Control("START",8,.56f,.90f));controls.add(new Control("SELECT",1,.46f,.90f));controls.add(new Control("L3",2,.08f,.46f));controls.add(new Control("R3",4,.69f,.46f));
        if(presetApplied>=1){controls.get(2).x=.07f;controls.get(2).y=.37f;controls.get(8).x=.78f;controls.get(8).y=.12f;}
        if(presetApplied==2){controls.get(3).x=.93f;controls.get(3).y=.36f;controls.get(6).x=.07f;controls.get(6).y=.12f;controls.get(7).x=.19f;controls.get(7).y=.12f;}
        try{JSONArray saved=new JSONArray(game.prefs().getString("layout","[]"));for(int i=0;i<Math.min(saved.length(),controls.size());i++){JSONArray a=saved.getJSONArray(i);Control c=controls.get(i);c.x=clamp((float)a.getDouble(0),.03f,.97f);c.y=clamp((float)a.getDouble(1),.05f,.95f);c.r=clamp((float)a.getDouble(2),.5f,2f);}}catch(JSONException ignored){}invalidate();
    }
    void saveLayout(){JSONArray a=new JSONArray();for(Control c:controls){JSONArray row=new JSONArray();try{row.put(c.x);row.put(c.y);row.put(c.r);}catch(JSONException ignored){}a.put(row);}game.prefs().edit().putString("layout",a.toString()).apply();}
    private static float clamp(float v,float lo,float hi){return Math.max(lo,Math.min(hi,v));}
    private float radius(Control c){return getHeight()*(c.name.equals("MOVER")||c.name.equals("ESQUIVA")?.12f:.065f)*game.prefs().getInt("size",100)/100f*c.r;}
    @Override protected void onDraw(Canvas canvas){super.onDraw(canvas);float w=getWidth(),h=getHeight();
        p.setStyle(Paint.Style.FILL);p.setColor(0xAA191313);canvas.drawRoundRect(w/2-45,12,w/2+45,64,16,16,p);p.setColor(MobileUi.GOLD);p.setTextSize(30);p.setTextAlign(Paint.Align.CENTER);canvas.drawText("Ⅱ",w/2,49,p);
        p.setColor(MobileUi.GOLD);p.setTextSize(20);canvas.drawText(movie && gameFps<.5f?"Vídeo":String.format(java.util.Locale.ROOT,"%s: %.1f FPS",movie?"Vídeo":"Jogo",gameFps),getWidth()-105,35,p);
        if(controller && game.prefs().getBoolean("autohide",true) && !editing)return;
        int alpha=game.prefs().getInt("opacity",65)*255/100;
        for(Control c:controls){if(movie && !editing && c.mask!=8)continue;float x=c.x*w,y=c.y*h,r=radius(c);p.setColor((alpha<<24)|0x221718);p.setStyle(Paint.Style.FILL);canvas.drawCircle(x,y,r,p);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(2);p.setColor(c==selected&&editing?Color.WHITE:(alpha<<24)|0xDCB870);canvas.drawCircle(x,y,r,p);p.setStyle(Paint.Style.FILL);p.setTextSize(r*(c.name.length()>2?.34f:.75f));canvas.drawText(c.name,x,y+r*.23f,p);
            if(c.mask==0)for(Map.Entry<Integer,Control> owner:owners.entrySet())if(owner.getValue()==c){PointF point=points.get(owner.getKey());if(point!=null){float dx=point.x-x,dy=point.y-y,len=(float)Math.hypot(dx,dy),k=len>r?r/len:1;p.setColor((alpha<<24)|0xC2A271);canvas.drawCircle(x+dx*k,y+dy*k,r*.35f,p);}}
        }
        if(editing){p.setColor(MobileUi.GOLD);p.setTextSize(24);canvas.drawText("Arraste os controles • − / + redimensiona • Voltar salva",w/2,100,p);canvas.drawText("−",w/2-70,160,p);canvas.drawText("+",w/2+70,160,p);}
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        int action=e.getActionMasked(),index=e.getActionIndex(),id=e.getPointerId(index);float x=e.getX(index),y=e.getY(index);
        if(action==MotionEvent.ACTION_DOWN || action==MotionEvent.ACTION_POINTER_DOWN){
            if(y<75 && Math.abs(x-getWidth()/2f)<65){game.pauseMenu();return true;}
            if(editing && y>110 && y<190 && selected!=null && Math.abs(x-getWidth()/2f)<120){selected.r=clamp(selected.r+(x<getWidth()/2f?-.1f:.1f),.5f,2);invalidate();return true;}
            Control best=null;float distance=Float.MAX_VALUE;
            for(Control c:controls){if(movie && !editing && c.mask!=8)continue;float d=(float)Math.hypot(x-c.x*getWidth(),y-c.y*getHeight());if(d<radius(c)*1.15f && d<distance && !owners.containsValue(c)){best=c;distance=d;}}
            if(best!=null){owners.put(id,best);points.put(id,new PointF(x,y));selected=best;}
        } else if(action==MotionEvent.ACTION_MOVE){
            for(int i=0;i<e.getPointerCount();i++){int pointer=e.getPointerId(i);Control c=owners.get(pointer);if(c==null)continue;float px=e.getX(i),py=e.getY(i);points.put(pointer,new PointF(px,py));if(editing){c.x=clamp(px/getWidth(),.03f,.97f);c.y=clamp(py/getHeight(),.05f,.95f);}}
        } else if(action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP){owners.remove(id);points.remove(id);if(editing)saveLayout();}
        else if(action==MotionEvent.ACTION_CANCEL){clear();}
        if(!editing){int buttons=0;float lx=0,ly=0,rx=0,ry=0;
            for(Map.Entry<Integer,Control> owner:owners.entrySet()){Control c=owner.getValue();PointF point=points.get(owner.getKey());if(point==null)continue;if(c.mask!=0)buttons|=c.mask;else {float dx=(point.x-c.x*getWidth())/radius(c),dy=(point.y-c.y*getHeight())/radius(c);float len=(float)Math.hypot(dx,dy);if(len>1){dx/=len;dy/=len;}if(c.name.equals("MOVER")){lx=dx;ly=dy;}else{rx=dx;ry=dy;}}}
            game.sendTouch(buttons,lx,ly,rx,ry);
        }
        invalidate();return true;
    }
}
