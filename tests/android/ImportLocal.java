package io.github.genissonemilio.gow2;
import java.io.*;
public final class ImportLocal {
    public static void main(String[] args) throws Exception {
        if(args.length!=2)throw new IllegalArgumentException("ZIP_OR_ISO OUTPUT_DIRECTORY");
        DiscImporter.importStream(new FileInputStream(args[0]),new File(args[1]),System.out::println);
        System.out.println("Import complete");
    }
}
