import sys
import os
import argparse
from intelhex import IntelHex



if __name__ == '__main__':
    ih = IntelHex()
    print('main')
    
    #parser = argparse.ArgumentParser()
    #parser.add_argument("-i",help="input binary file")
    #parser.add_argument("-o",help="output header file")
    #args = parser.parse_args()
    
    if os.path.exists(sys.argv[1]):
        ih.loadbin(sys.argv[1])
        bin = ih.todict()
        pad = (256-(len(ih) % 256)) % 256
   
        bin = ih.tobinarray()
      
        for i in range(0,pad,1):
            bin.append(0xFF)
            
        f = open("rom.h","w")
        f.write("#ifndef __ROM_BIN_H__\n")
        f.write("#define __ROM_BIN_H__\n\n")
        f.write("#define ROM_LEN {0}\n\n".format(len(bin)))

        f.write("const unsigned char rom_bin[ROM_LEN]={")
        for i in range(0,len(bin),1):
            if 0==(i & 0x0f):
                f.write('\n')
            f.write("0x{0:02X}".format(bin[i]))
            if i < len(bin)-1:
                f.write(",")
        f.write("};\n")
        f.write("#endif\n")
        f.close()

    else:
        print('can not open this file')
