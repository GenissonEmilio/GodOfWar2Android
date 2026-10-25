package io.github.genissonemilio.gow2;
import android.app.*;
import android.os.*;
import android.content.*;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.concurrent.*;
public final class LauncherActivity extends Activity {
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private TextView status; private Button play,load; private boolean busy;
    private File root(){return getExternalFilesDir(null);}
    @Override public void onCreate(Bundle state){super.onCreate(state);getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);show();}
    private void show(){
        LinearLayout view=MobileUi.column(this);view.setGravity(Gravity.CENTER);
        TextView title=MobileUi.text(this,"GOD OF WAR II",38);title.setTextColor(MobileUi.GOLD);view.addView(title);
        view.addView(MobileUi.text(this,"ANDROID • DESENVOLVIMENTO",16));
        status=MobileUi.text(this,"Importe seu DVD USA v1.01 em ISO ou ZIP. A importação precisa de cerca de 18 GB livres.",16);view.addView(status);
        play=MobileUi.button(this,"ENTRAR NO OLIMPO",()->startActivity(new Intent(this,GameActivity.class)));view.addView(play);
        load=MobileUi.button(this,"IMPORTAR DVD",()->{Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,100);});view.addView(load);
        view.addView(MobileUi.button(this,"CONFIGURAÇÕES",()->MobileUi.settings(this,()->{})));
        view.addView(MobileUi.text(this,"Adaptação independente em teste. A campanha completa ainda não foi validada.",12));setContentView(view);refresh();
    }
    private void refresh(){play.setEnabled(!busy && new File(root(),"game/SCUS_974.81").isFile());load.setEnabled(!busy);}
    @Override protected void onResume(){super.onResume();if(play!=null)refresh();}
    @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request!=100 || result!=RESULT_OK || data==null)return;
        busy=true;refresh();
        android.net.Uri uri=data.getData();
        worker.execute(()->{try{if(root().getUsableSpace()<18L*1024*1024*1024)throw new IOException("São necessários cerca de 18 GB livres para importar o DVD");
            InputStream source=getContentResolver().openInputStream(uri);if(source==null)throw new IOException("Não foi possível abrir o DVD");
            DiscImporter.importStream(source,root(),message->runOnUiThread(()->status.setText(message)));
            runOnUiThread(()->status.setText("DVD validado. Pronto para iniciar."));
        }catch(Exception e){runOnUiThread(()->status.setText("Falha na importação: "+e.getMessage()));}
        finally{runOnUiThread(()->{busy=false;refresh();});}});
    }
    @Override public void onBackPressed(){if(!busy)super.onBackPressed();}
    @Override protected void onDestroy(){worker.shutdown();super.onDestroy();}
}
