"""The second DVD layer must be extracted with its own relative LBA base."""
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/desktop'))
import import_disc
import iso9660

def record(name,lba,size,directory=False):
    name=name.encode('ascii');n=33+len(name)+(len(name)%2==0)
    out=bytearray(n);out[0]=n;struct.pack_into('<I',out,2,lba);struct.pack_into('<I',out,10,size)
    out[25]=2 if directory else 0;out[32]=len(name);out[33:33+len(name)]=name
    return out

def fixture(path):
    disc=bytearray(40*2048)
    def write(sector,data):disc[sector*2048:sector*2048+len(data)]=data
    for pvd,root,volume in ((16,20,32),(32,20,24)):
        descriptor=bytearray(2048);descriptor[0]=1;descriptor[1:6]=b'CD001'
        struct.pack_into('<I',descriptor,80,volume)
        descriptor[156:190]=record('x',root,2048,True)
        write(pvd,descriptor)
    write(20,record('SCUS_974.81;1',21,4)+record('PART1.PAK;1',22,4))
    write(36,record('PART2.PAK;1',21,4))
    write(21,b'BOOT');write(22,b'ONE!');write(37,b'TWO!')
    path.write_bytes(disc)

class DualLayerTests(unittest.TestCase):
    def test_imports_both_layers_and_original_sector_layout(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder=Path(temporary);image=folder/'disc.iso';fixture(image)
            with patch.dict(import_disc.EXPECTED,{'SCUS_974.81':(4,hashlib.sha256(b'BOOT').hexdigest())},clear=True),patch.dict(import_disc.PAKS,{'PART1.PAK':4,'PART2.PAK':4},clear=True):
                info=import_disc.inspect_disc(image);self.assertEqual(info['layer1'],16)
                import_disc.extract_iso(image,folder/'disc')
            self.assertEqual((folder/'disc/PART1.PAK').read_bytes(),b'ONE!')
            self.assertEqual((folder/'disc/PART2.PAK').read_bytes(),b'TWO!')
            self.assertIn('file 37 4 PART2.PAK',(folder/'disc/.disc_layout').read_text())
            self.assertTrue((folder/'disc/.disc_meta').stat().st_size>0)
    def test_refuses_missing_second_layer_and_wrong_revision(self):
        with tempfile.TemporaryDirectory() as temporary:
            image=Path(temporary)/'disc.iso';fixture(image)
            with self.assertRaises(import_disc.ImportError_):import_disc.inspect_disc(image)
            data=bytearray(image.read_bytes());data[32*2048:33*2048]=bytes(2048);image.write_bytes(data)
            with patch.dict(import_disc.EXPECTED,{'SCUS_974.81':(4,hashlib.sha256(b'BOOT').hexdigest())},clear=True),self.assertRaises(import_disc.ImportError_):import_disc.inspect_disc(image)
    def test_refuses_directory_cycle(self):
        with tempfile.TemporaryDirectory() as temporary:
            image=Path(temporary)/'disc.iso';fixture(image)
            data=bytearray(image.read_bytes());entry=record('cycle',20,2048,True)
            data[20*2048:20*2048+len(entry)]=entry;image.write_bytes(data)
            with image.open('rb') as stream,self.assertRaises(ValueError):iso9660.scan(stream)

if __name__=='__main__':unittest.main()
