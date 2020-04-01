# -*- coding: UTF-8 -*-

try:
    import chardet
except :
    import os
    os.system('pip install chardet')
    import chardet

import glob
import codecs
import os

def utf8_converter(file_path, universal_endline=True):
    '''
    Convert any type of file to UTF-8 without BOM
    and using universal endline by default.

    Parameters
    ----------
    file_path : string, file path.
    universal_endline : boolean (True),
                        by default convert endlines to universal format.
    '''

    # Fix file path
    file_path = os.path.realpath(os.path.expanduser(file_path))

    # Read from file
    file_open = open(file_path, 'rb')
    raw = file_open.read()
    file_open.close()

    # Decode
    encoding = chardet.detect(raw)['encoding']
    if encoding == None or encoding == 'utf-8':
        return 0
    
    print('convert ' + file_path)
    raw = raw.decode(encoding)
    # Remove windows end line
    if universal_endline:
        raw = raw.replace('\r\n', '\n')
    # Encode to UTF-8
    raw = raw.encode('utf8')
    # Remove BOM
    if raw.startswith(codecs.BOM_UTF8):
        raw = raw.replace(codecs.BOM_UTF8, '', 1)

    # Write to file
    file_open = open(file_path, 'wb')
    file_open.write(bytes(raw))
    file_open.close()
    return 0

def convert_dir_ext(path, ext):
    files = glob.glob(path + "/**/*." + ext, recursive = True)
    for path in files:
        utf8_converter(path, False)

convert_dir_ext("include", "h")
convert_dir_ext("wsCore", "cpp")
convert_dir_ext("wsDatabase", "cpp")
convert_dir_ext("wsNetwork", "cpp")
convert_dir_ext("test", "cpp")

os.system('pause')
