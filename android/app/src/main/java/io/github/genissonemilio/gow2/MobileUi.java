package io.github.genissonemilio.gow2;
import android.app.*;
import android.content.*;
import android.graphics.Color;
import android.view.*;
import android.widget.*;
final class MobileUi {
    static final int BG=Color.rgb(18,16,16), GOLD=Color.rgb(220,184,112), RED=Color.rgb(125,31,33), TEXT=Color.rgb(241,231,213);
    static int dp(Context c,float v){return Math.round(v*c.getResources().getDisplayMetrics().density);}
    static TextView text(Context c,String s,int size){TextView t=new TextView(c);t.setText(s);t.setTextSize(size);t.setTextColor(TEXT);t.setPadding(12,8,12,8);return t;}
    static Button button(Context c,String s,Runnable action){Button b=new Button(c);b.setText(s);b.setTextColor(TEXT);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(RED));b.setOnClickListener(v->action.run());return b;}
    static LinearLayout column(Context c){LinearLayout l=new LinearLayout(c);l.setOrientation(LinearLayout.VERTICAL);l.setPadding(dp(c,20),dp(c,10),dp(c,20),dp(c,10));l.setBackgroundColor(BG);return l;}
    static void slider(LinearLayout parent,android.content.SharedPreferences prefs,String key,String label,int min,int max,int fallback,Runnable changed){
        Context c=parent.getContext();TextView t=text(c,label+": "+prefs.getInt(key,fallback),16);parent.addView(t);
        SeekBar bar=new SeekBar(c);bar.setMax(max-min);bar.setProgress(prefs.getInt(key,fallback)-min);bar.setContentDescription(label);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}public void onProgressChanged(SeekBar b,int value,boolean user){if(user){int n=value+min;prefs.edit().putInt(key,n).apply();t.setText(label+": "+n);changed.run();}}});parent.addView(bar);
    }
    static AlertDialog settings(Activity a,Runnable changed){
        android.content.SharedPreferences p=a.getSharedPreferences("mobile",0);
        LinearLayout body=column(a);body.addView(text(a,"AJUSTES DO OLIMPO",24));
        body.addView(text(a,"TOUCH E CONTROLE",18));
        TextView presetLabel=text(a,"Layout base",16);body.addView(presetLabel);
        Spinner preset=new Spinner(a);
        String[] presets={"Padrão — 2 dedos","Combate — 3 dedos","Combate — 4 dedos"};
        ArrayAdapter<String> adapter=new ArrayAdapter<>(a,android.R.layout.simple_spinner_dropdown_item,presets);preset.setAdapter(adapter);preset.setSelection(p.getInt("preset",0));
        preset.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){public void onNothingSelected(android.widget.AdapterView<?> v){}public void onItemSelected(android.widget.AdapterView<?> v,View row,int pos,long id){if(pos!=p.getInt("preset",0)){p.edit().putInt("preset",pos).remove("layout").apply();changed.run();}}});body.addView(preset);
        slider(body,p,"opacity","Opacidade (%)",15,100,65,changed);
        slider(body,p,"size","Tamanho dos controles (%)",60,160,100,changed);
        slider(body,p,"deadzone","Zona morta (%)",0,50,14,changed);
        slider(body,p,"sensitivity","Sensibilidade (%)",10,300,100,changed);
        Switch auto=new Switch(a);auto.setText("Ocultar touch com controle físico");auto.setTextColor(TEXT);auto.setChecked(p.getBoolean("autohide",true));auto.setOnCheckedChangeListener((v,b)->{p.edit().putBoolean("autohide",b).apply();changed.run();});body.addView(auto);
        body.addView(text(a,"IMAGEM E DESEMPENHO",18));
        body.addView(text(a,"Resolução interna: 1×, 2× ou 3×. A mudança vale na próxima abertura do jogo.",14));
        slider(body,p,"scale","Resolução interna",1,3,1,changed);
        Switch fps=new Switch(a);fps.setText("Limitar apresentação a 30 FPS (o jogo mantém seu relógio)");fps.setTextColor(TEXT);fps.setChecked(p.getInt("fps",60)==30);fps.setOnCheckedChangeListener((v,b)->{p.edit().putInt("fps",b?30:60).apply();changed.run();});body.addView(fps);
        Switch pipeline=new Switch(a);pipeline.setText("Renderização assíncrona (experimental)");pipeline.setTextColor(TEXT);pipeline.setChecked(p.getBoolean("graphicsAsync",false));pipeline.setOnCheckedChangeListener((v,b)->{p.edit().putBoolean("graphicsAsync",b).apply();changed.run();});body.addView(pipeline);
        slider(body,p,"volume","Volume (%)",0,100,80,changed);
        body.addView(text(a,"Os saves ficam no aparelho. Evite limpar os dados do aplicativo.",14));
        ScrollView scroll=new ScrollView(a);scroll.addView(body);
        return new AlertDialog.Builder(a).setView(scroll).setPositiveButton("Concluir",(d,w)->changed.run()).show();
    }
}
