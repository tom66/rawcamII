%module rawcam

%include "stdint.i"

%rename("%(strip:[rawcam_])s") "";
%{
#define SWIG_FILE_WITH_INIT
#include "rawcam.h"
%}

%typemap(out) MMAL_BUFFER_HEADER_T *{
  Py_buffer *buf=malloc(sizeof *buf);
  PyBuffer_FillInfo(buf, NULL,  $1->data, $1->length, true /*ro*/, PyBUF_ND);
  buf->internal = $1;
  $result = PyMemoryView_FromBuffer(buf);
}

%typemap(in) MMAL_BUFFER_HEADER_T *{
  //fprintf(stderr,"buf in %p\n",$input);
  Py_buffer *buf=PyMemoryView_GET_BUFFER ($input);
  //fprintf(stderr,"buf2 is %p\n", buf);
  //fprintf(stderr,"buf->internal is %p\n", buf->internal);
  $1 = buf->internal;
}

%include "rawcam.h"
