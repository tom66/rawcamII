PYTHON ?= python3.8
SWIG   ?= swig3.0

build: rawcam_wrap.c rawcam.c rawcam.h
	$(PYTHON) setup.py build_ext --inplace

rawcam_wrap.c: rawcam.i rawcam.c
	$(SWIG) -python -builtin rawcam.i

test:
	$(PYTHON) test.py

pyg_test:
	$(PYTHON) pyg_test.py

clean:
	-rm -f *.so *.o rawcam_wrap.c

rawcam_test: rawcam_test.c
	cc -g -O2 -Wall -L/opt/vc/lib -I/opt/vc/include -lmmal -lbcm_host -lvcos -lmmal_core -lmmal_util -lmmal_vc_client rawcam.c $< -o $@

.PHONY: build clean
