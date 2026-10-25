package io.github.genissonemilio.gow2;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.*;
import java.util.*;
import java.util.zip.*;

/** Bounded ISO9660/DVD9 importer. Archive paths are never used as output paths. */
public final class DiscImporter {
    static final long LIMIT = 10L * 1024 * 1024 * 1024;
    static final String BOOT_HASH = "fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6";
    public interface Progress { void update(String message); }
    static final class Entry {
        String name; long lba, size;
        Entry(String n,long l,long s) { name=n;lba=l;size=s; }
    }
    static long u32(byte[] b,int p) { return (b[p]&255L)|((b[p+1]&255L)<<8)|((b[p+2]&255L)<<16)|((b[p+3]&255L)<<24); }
    static byte[] read(RandomAccessFile f,long pos,int count) throws IOException {
        if(pos<0 || count<0 || pos>f.length()-count) throw new IOException("DVD truncado");
        byte[] b=new byte[count]; f.seek(pos); f.readFully(b); return b;
    }
    static void directory(RandomAccessFile f,long lba,long size,String path,long base,List<Entry> files,Set<Long> visited) throws IOException {
        if(path.split("/").length>8 || visited.size()>4096 || size>16*1024*1024 || !visited.add(base+lba)) throw new IOException("Diretório ISO inválido ou cíclico");
        byte[] b=read(f,(base+lba)*2048,(int)size);
        for(int at=0;at<b.length;) {
            int len=b[at]&255;
            if(len==0) { at=(at/2048+1)*2048; continue; }
            if(len<34 || at+len>b.length || (b[at+32]&255)>len-33) throw new IOException("Registro ISO inválido");
            int nameLen=b[at+32]&255; long start=u32(b,at+2), bytes=u32(b,at+10);
            String name=new String(b,at+33,nameLen,StandardCharsets.US_ASCII).split(";",2)[0];
            boolean dir=(b[at+25]&2)!=0; at+=len;
            if(name.equals("\0") || name.equals("\1")) continue;
            if(name.isEmpty() || name.equals(".") || name.equals("..") || name.contains("/") || name.contains("\\") || name.contains(":") || name.indexOf(0)>=0) throw new IOException("Nome ISO inseguro");
            if((base+start)*2048>f.length()-bytes) throw new IOException("Arquivo fora do DVD");
            String full=path.isEmpty()?name:path+"/"+name;
            if(dir) directory(f,start,bytes,full,base,files,visited);
            else { if(files.size()>=4096) throw new IOException("Excesso de arquivos"); files.add(new Entry(full,base+start,bytes)); }
        }
    }
    static boolean pvd(byte[] b) { return b.length==2048 && b[0]==1 && new String(b,1,5,StandardCharsets.US_ASCII).equals("CD001"); }
    static String hash(byte[] bytes) throws IOException {
        try { byte[] digest=MessageDigest.getInstance("SHA-256").digest(bytes); StringBuilder s=new StringBuilder(); for(byte b:digest)s.append(String.format(Locale.ROOT,"%02x",b&255)); return s.toString(); }
        catch(NoSuchAlgorithmException e) { throw new IOException(e); }
    }
    static void copy(InputStream in,OutputStream out,long limit) throws IOException {
        byte[] buffer=new byte[1024*1024]; long count=0; int n;
        while((n=in.read(buffer))!=-1) { count+=n; if(count>limit)throw new IOException("Arquivo excede o limite de 10 GiB"); out.write(buffer,0,n); }
    }
    static void remove(File file) throws IOException {
        if(file.isDirectory()) { File[] children=file.listFiles(); if(children!=null)for(File child:children) remove(child); }
        Files.deleteIfExists(file.toPath());
    }
    public static void importStream(InputStream source,File root,Progress progress) throws IOException {
        root.mkdirs();
        File work=Files.createTempDirectory(root.toPath(),".import-").toFile();
        File image=new File(work,"disc.iso"), stage=new File(work,"game");
        try {
            progress.update("Preparando DVD local…");
            try(BufferedInputStream input=new BufferedInputStream(source)) {
                input.mark(4); int a=input.read(),b=input.read(); input.reset();
                if(a=='P' && b=='K') {
                    int found=0,entries=0;
                    try(ZipInputStream zip=new ZipInputStream(input)) {
                        ZipEntry entry;
                        while((entry=zip.getNextEntry())!=null) {
                            if(++entries>128) throw new IOException("ZIP contém arquivos demais");
                            if(!entry.isDirectory() && entry.getName().toLowerCase(Locale.ROOT).endsWith(".iso")) {
                                if(++found!=1)throw new IOException("Escolha um ZIP com exatamente um DVD ISO");
                                try(OutputStream out=new FileOutputStream(image)) { copy(zip,out,LIMIT); }
                            } else throw new IOException("O ZIP deve conter somente um DVD ISO");
                            zip.closeEntry();
                        }
                    }
                    if(found!=1) throw new IOException("DVD ISO não encontrado no ZIP");
                } else try(OutputStream out=new FileOutputStream(image)) { copy(input,out,LIMIT); }
            }
            extract(image,stage,progress);
            File target=new File(root,"game"), old=new File(root,".previous-game");
            if(old.exists())throw new IOException("Importação anterior pendente; os dados foram preservados");
            if(target.exists() && !target.renameTo(old))throw new IOException("Não foi possível preservar os dados anteriores");
            if(!stage.renameTo(target)) { if(old.exists())old.renameTo(target); throw new IOException("Falha ao instalar o DVD"); }
            if(old.exists()) remove(old);
        } finally { remove(work); }
    }
    static void extract(File image,File stage,Progress progress) throws IOException {
        List<Entry> files=new ArrayList<>(); Set<Long> visited=new HashSet<>();
        try(RandomAccessFile f=new RandomAccessFile(image,"r")) {
            byte[] header=read(f,16*2048,2048);
            if(!pvd(header))throw new IOException("Imagem ISO9660 inválida");
            directory(f,u32(header,158),u32(header,166),"",0,files,visited);
            long layer0=u32(header,80),layer1=layer0-16;
            byte[] second=read(f,layer0*2048,2048);
            if(!pvd(second))throw new IOException("God of War II exige as duas camadas do DVD");
            directory(f,u32(second,158),u32(second,166),"",layer1,files,visited);
            Map<String,Entry> names=new HashMap<>();
            for(Entry e:files)if(names.put(e.name.toUpperCase(Locale.ROOT),e)!=null)throw new IOException("Arquivos ISO duplicados");
            Entry boot=names.get("SCUS_974.81"),p1=names.get("PART1.PAK"),p2=names.get("PART2.PAK");
            if(boot==null || boot.size!=2125688 || !hash(read(f,boot.lba*2048,(int)boot.size)).equals(BOOT_HASH))throw new IOException("Use God of War II USA SCUS-97481 v1.01 sem modificações");
            if(p1==null || p1.size!=4264382464L || p2==null || p2.size!=4265340928L)throw new IOException("DVD9 incompleto");
            stage.mkdirs(); byte[] buffer=new byte[1024*1024]; int count=0;
            for(Entry e:files) {
                progress.update("Extraindo "+(++count)+"/"+files.size()+" — "+e.name);
                File target=new File(stage,e.name); target.getParentFile().mkdirs();
                f.seek(e.lba*2048);
                try(OutputStream out=new FileOutputStream(target)) { long remaining=e.size; while(remaining>0) { int n=f.read(buffer,0,(int)Math.min(buffer.length,remaining)); if(n<0)throw new EOFException();out.write(buffer,0,n);remaining-=n; } }
            }
            files.sort(Comparator.comparingLong(e->e.lba));
            try(Writer layout=new OutputStreamWriter(new FileOutputStream(new File(stage,".disc_layout")),StandardCharsets.UTF_8);
                DataOutputStream meta=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(new File(stage,".disc_meta"))))) {
                long total=f.length()/2048, cursor=0;
                layout.write("sectors "+total+"\nlayer1 "+layer1+"\n");
                List<Entry> ranges=new ArrayList<>(files); ranges.add(new Entry("",total,0));
                byte[] sector=new byte[2048];
                for(Entry e:ranges) {
                    for(long lba=cursor;lba<e.lba;lba++) { f.seek(lba*2048);f.readFully(sector);boolean nonzero=false;for(byte v:sector)if(v!=0){nonzero=true;break;} if(nonzero){meta.writeInt(Integer.reverseBytes((int)lba));meta.write(sector);} }
                    cursor=Math.max(cursor,e.lba+(e.size+2047)/2048);
                    if(!e.name.isEmpty())layout.write("file "+e.lba+" "+e.size+" "+e.name+"\n");
                }
            }
            File iop=new File(stage,"IOP_MOD");iop.mkdirs();
            for(Entry e:files)if(e.name.endsWith(".IRX") && !e.name.contains("/")) Files.copy(new File(stage,e.name).toPath(),new File(iop,e.name).toPath());
        }
    }
}
