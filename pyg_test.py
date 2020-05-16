#!/usr/bin/env python3
import numpy
import rawcam
import random
import pygame
from hashlib import md5

DISP_WIDTH = 1024
DISP_HEIGHT = 512

WAVE_LENGTH = 8192
BUFF_SIZE = 262144
WAVES = BUFF_SIZE / WAVE_LENGTH
WAVE_X_SCALE = WAVE_LENGTH / DISP_WIDTH
WAVE_Y_SCALE = DISP_HEIGHT / 256

pygame.init()
window = pygame.display.set_mode((1024, 512))

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

        """
        arr=numpy.frombuffer(buf,dtype='uint8') # yes this is zerocopy
        #print ("average sample value %d" % (arr.sum()/len(arr)))
        #print(j)
        if (1):
                open(("rxtest/%02d.bin" % j),"wb").write(buf)
        """
        
        # plot waveforms, this is probably -very- slow but I don't have a better way right now
        pos = 0
        for n in range(WAVES):
            last_y = int(buf[xx_pos]) / WAVE_Y_SCALE
            last_x = 0
            
            for x in range(1, WAVE_LENGTH):
                this_y = int(buf[xx_pos]) / WAVE_Y_SCALE
                xa_pos = int((xx_pos + x) / WAVE_X_SCALE)
                print(((last_x, last_y), (xa_pos, this_y)))
                
                pygame.draw.line(window, (255, 255, 255), (last_x, last_y), (xa_pos, this_y))
                
                if xa_pos != last_x:
                    last_x = xa_pos
            
            xx_pos += WAVE_LENGTH
        
        rawcam.buffer_free(buf)

    pygame.display.flip()
    window.fill((0, 0, 0))
    
    time.sleep(0.05)
    