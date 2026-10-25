package io.github.genissonemilio.gow2;
import java.io.*;
import java.nio.file.*;
import java.util.zip.*;
public final class DiscImporterTest {
    static int checks;
    static void rejects(byte[] bytes,File root) throws Exception {
        try { DiscImporter.importStream(new ByteArrayInputStream(bytes),root,s->{}); throw new AssertionError("Invalid input accepted"); }
        catch(IOException expected) { checks++; }
        if(!new File(root,"game/save-marker").isFile())throw new AssertionError("Existing data lost");
        for(File f:root.listFiles())if(f.getName().startsWith(".import-"))throw new AssertionError("Temporary import leaked");
    }
    public static void main(String[] args) throws Exception {
        File root=Files.createTempDirectory("gow2-import-test").toFile();new File(root,"game").mkdir();Files.write(new File(root,"game/save-marker").toPath(),new byte[]{42});
        try {
            rejects(new byte[100],root);
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();
            try(ZipOutputStream z=new ZipOutputStream(bytes)){z.putNextEntry(new ZipEntry("../../escape.iso"));z.write(new byte[100]);z.closeEntry();}
            rejects(bytes.toByteArray(),root);
            bytes.reset();try(ZipOutputStream z=new ZipOutputStream(bytes)){z.putNextEntry(new ZipEntry("unrelated.txt"));z.write(1);z.closeEntry();}rejects(bytes.toByteArray(),root);
            bytes.reset();try(ZipOutputStream z=new ZipOutputStream(bytes)){for(int i=0;i<2;i++){z.putNextEntry(new ZipEntry("disc"+i+".iso"));z.write(new byte[100]);z.closeEntry();}}rejects(bytes.toByteArray(),root);
            if(new File(root.getParentFile(),"escape.iso").exists())throw new AssertionError("Archive path escaped");
            System.out.println(checks+" invalid imports rejected; existing data preserved and temporary files removed");
        } finally { DiscImporter.remove(root); }
    }
}
