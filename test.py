#!/usr/bin/env python3
import numpy
import rawcam
import random
from hashlib import md5

rc = rawcam.init() # initializes camera interface, returns config object
#rc.pack = rawcam.Pack.NONE
#rc.unpack = rawcam.Unpack.NONE

#rawcam.set_timing(0, 0, 0, 0, 0, 0, 0)
rawcam.set_data_lanes(2)
rawcam.set_image_id(0x2a)
rawcam.set_buffer_size(2048*128)
rawcam.set_buffer_num(8)
rawcam.set_buffer_dimensions(2048, 128)
rawcam.set_pack_mode(0)
rawcam.set_unpack_mode(0)
rawcam.set_unpack_mode(0)
rawcam.set_encoding_fourcc(ord('G'), ord('R'), ord('B'), ord('G'))
#rawcam.set_encode_block_length(32)
#rawcam.set_embedded_data_lines(32)
rawcam.set_zero_copy(1)

rawcam.set_camera_num(1)

print("debug after init params")
rawcam.debug()

#rawcam.format_commit()
#print("debug after format_commit")
#rawcam.debug()

print("start rawcam")
rawcam.start()

print("debug after start")
rawcam.debug()

j = 0
while True: # FIXME
    #print("iter")
    #print(rawcam.buffer_count())
    
    for i in range(rawcam.buffer_count()):
        j+=1
        buf = rawcam.buffer_get()
        #print(dir(buf))
        print ("[%4d] got buf %s, len=%d, hash=%s" % (j,buf,len(buf),md5(buf).hexdigest()))

        arr=numpy.frombuffer(buf,dtype='uint8') # yes this is zerocopy
        #print ("average sample value %d" % (arr.sum()/len(arr)))
        #print(j)
        if (1):
                open(("rxtest/%02d.bin" % j),"wb").write(buf)

        # do other stuff with buffer contents

        rawcam.buffer_free(buf)
