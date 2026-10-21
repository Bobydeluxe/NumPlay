"""Private ELF name compaction must preserve imports and public symbols."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec=importlib.util.spec_from_file_location('packing',Path(__file__).resolve().parents[1]/'tools/compact_nwa.py')
packing=importlib.util.module_from_spec(spec);spec.loader.exec_module(packing)

def elf(symbols):
    strings=bytearray(b'\0');entries=bytearray(16)
    for name,binding,kind,visibility,index in symbols:
        at=len(strings);strings+=name.encode()+b'\0'
        entries+=struct.pack('<IIIBBH',at,0,0,binding<<4|kind,visibility,index)
    data=bytearray(52+3*40);data[:6]=b'\x7fELF\x01\x01'
    struct.pack_into('<I',data,32,52);struct.pack_into('<HH',data,46,40,3)
    struct.pack_into('<10I',data,52+40,0,2,0,0,len(data),len(entries),2,0,4,16)
    struct.pack_into('<10I',data,52+80,0,3,0,0,len(data)+len(entries),len(strings),0,0,1,0)
    return data+entries+strings

class Symbols(unittest.TestCase):
    def test_only_private_definitions_are_renamed(self):
        entries=[('main',0,2,0,1),('eadk_app_icon',0,1,0,1),
                 ('eadk_keyboard_scan',1,2,0,0),('another_import',1,2,0,0),
                 ('.text',0,3,0,1),('public_api',1,2,0,1),
                 ('local_fun',0,2,0,1),('hidden_fun',1,2,2,1),
                 ('secret',1,1,1,1),('mixed',0,2,0,1),('mixed',1,2,0,1),('_0',0,1,0,1)]
        aliases=packing.private_symbol_aliases(elf(entries))
        self.assertEqual(set(aliases),{'local_fun','hidden_fun','secret','_0'})
        self.assertEqual(len(set(aliases.values())),len(aliases))
        self.assertFalse(set(aliases.values())&{item[0] for item in entries})

    def test_rejects_other_formats(self):
        with self.assertRaises(ValueError):packing.private_symbol_aliases(b'bad')

if __name__=='__main__':unittest.main()
