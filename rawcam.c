#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
//#include <libgen.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>

#include "interface/vcos/vcos.h"
#include "bcm_host.h"

//#include "scope.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_connection.h"

#include <sys/ioctl.h>
#include <stdbool.h>
//#include "raw_header.h"
#include "rawcam.h"

struct rawcam_interface_private {
	MMAL_PARAMETER_CAMERA_RX_CONFIG_T rx_cfg;
	MMAL_PARAMETER_CAMERA_RX_TIMING_T rx_timing;
	int camera_num;
	int buffer_num;
	int buffer_size;
	int buffer_width;
	int buffer_height;
	int running;
	int eventfd;
	MMAL_PORT_T *output;
	MMAL_POOL_T *pool;
	MMAL_COMPONENT_T *rawcam, *isp;
	MMAL_CONNECTION_T *rawcam_isp;
	MMAL_QUEUE_T *queue;
	MMAL_BOOL_T zero_copy;
} r = {
	.camera_num = 1,
	.running = 0,
	.eventfd = -1,
	.output = NULL,
	.rawcam = NULL,
	.isp = NULL,
	.rawcam_isp = NULL,
	.queue = NULL
};

enum teardown { NONE=0, PORT, POOL, C1, C2 };

#define WARN(x) fprintf(stderr, x "\n");
#if 0
#define DBG(x) WARN("[" x "]");
#else
#define DBG(x)
#endif

/* abort if (!= MMAL_SUCCESS) */
#define TRY(f,td) do {			      \
		mmal_ret_status = (f); \
		if (mmal_ret_status != MMAL_SUCCESS) { \
			fprintf(stderr, "mmal error: %d during " #f "... dying (%s:%d)\n", mmal_ret_status, __FILE__, __LINE__); \
			teardown(td);	      \
			return false;	      \
		}} while(0)

#define RAWCAM_VERSION 	"v0.0.1"

int mmal_ret_status = 0;
int fi_counter = 0;

static void poke_efd(uint64_t u) {
	//fprintf(stderr,"poking...");
	write(r.eventfd, &u, sizeof u);
	//fprintf(stderr,"poked\n");
}

void signal_abort(int i) {
	r.running = 0;
	write(2, "aborting: signal_abort()\n", 9);
	poke_efd (1);
}

static void teardown(int what) {
	switch (what) {
	case PORT:
	  //if (cfg.capture)
	  mmal_port_disable(r.output);
	/* fall-through */
	case POOL:
	if (r.pool)
		mmal_port_pool_destroy(r.output, r.pool);
	if (r.rawcam_isp)	{
		mmal_connection_disable(r.rawcam_isp);
		mmal_connection_destroy(r.rawcam_isp);
	}
	/* fall-through */
	case C1:
		mmal_component_disable(r.isp);
		mmal_component_disable(r.rawcam);
	/* fall-through */
	case C2:
	if (r.rawcam)
		mmal_component_destroy(r.rawcam);
	if (r.isp)
		mmal_component_destroy(r.isp);
	}
}

void rawcam_stop (void) {
	r.running = 0;
	teardown(PORT);
}

static void callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
	//fprintf(stderr, "callback()\n");
	char fn_buffer[32];
	sprintf(&fn_buffer, "rxtest_c/%d.bin", fi_counter++);
	//FILE *fp = fopen(buffer, "wb");

	assert (r.running);
	if (!(buffer->flags&MMAL_BUFFER_HEADER_FLAG_CODECSIDEINFO)) {
		fprintf(stderr,"queueing buffer %p (data %p, len %d, flags %02x)\n", buffer, buffer->data, buffer->length, buffer->flags);
		mmal_queue_put(r.queue, buffer);
		FILE *fp = fopen(fn_buffer, "wb");
		fwrite(buffer->data, buffer->length, 1, fp);
		fclose(fp);
		poke_efd (1);
	} else {
		rawcam_buffer_free(buffer);
	}

	//fclose(fp);
}

MMAL_BUFFER_HEADER_T *rawcam_buffer_get(void) {
	MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(r.queue);
	fprintf(stderr,"dequeueing buffer %p (data %p, len %d)\n", buffer, buffer->data, buffer->length);
	return buffer;
}

unsigned int rawcam_buffer_count(void) {
	return mmal_queue_length (r.queue);
}

void rawcam_buffer_free(MMAL_BUFFER_HEADER_T *buffer) {
	//fprintf(stderr, "buffer_free called with %p\n", buffer);
	buffer->length = 0;
	mmal_port_send_buffer(r.output, buffer);
	mmal_buffer_header_release(buffer);
}

void rawcam_set_buffer_size(int buffer_size) {
	assert(buffer_size > 0);
	r.buffer_size = buffer_size;
	r.output->buffer_size = buffer_size;
}

int rawcam_get_buffer_size() {
	return r.buffer_size;
}

void rawcam_set_buffer_num(int buffer_num) {
	assert(buffer_num > 0);
	r.buffer_num = buffer_num;
	r.output->buffer_num = buffer_num;
}

int rawcam_get_buffer_num() {
	return r.buffer_num;
}

int rawcam_get_eventfd() {
	return r.eventfd;
}

void rawcam_set_image_id(uint8_t image_id) {
	assert(image_id >= 0x03);
	r.rx_cfg.image_id = image_id;
}

uint8_t rawcam_get_image_id() {
	return r.rx_cfg.image_id;
}

void rawcam_set_data_lanes(uint8_t n_lanes) {
	assert(n_lanes >= 1 && n_lanes <= 4);
	r.rx_cfg.data_lanes = n_lanes;
}

int rawcam_get_data_lanes() {
	return r.rx_cfg.data_lanes;
}

void rawcam_set_buffer_dimensions(int width, int height) {
	assert(width > 0 && height > 0);
	r.buffer_width = width;
	r.buffer_height = height;
}

void rawcam_set_decode_mode(int decode_mode) {
	r.rx_cfg.decode = decode_mode;
}

int rawcam_get_decode_mode() {
	return r.rx_cfg.decode;
}

void rawcam_set_encode_mode(int encode_mode) {
	r.rx_cfg.encode = encode_mode;
}

int rawcam_get_encode_mode() {
	return r.rx_cfg.encode;
}

void rawcam_set_unpack_mode(int unpack_mode) {
	r.rx_cfg.unpack = unpack_mode;
}

int rawcam_get_unpack_mode() {
	return r.rx_cfg.unpack;
}

void rawcam_set_pack_mode(int pack_mode) {
	r.rx_cfg.pack = pack_mode;
}

int rawcam_get_pack_mode() {
	return r.rx_cfg.pack;
}

void rawcam_set_encode_block_length(int encode) {
	r.rx_cfg.encode_block_length = encode;
}

int rawcam_get_encode_block_length() {
	return r.rx_cfg.encode_block_length;
}

void rawcam_set_embedded_data_lines(int data_lines) {
	r.rx_cfg.embedded_data_lines = data_lines;
}

int rawcam_get_embedded_data_lines() {
	return r.rx_cfg.embedded_data_lines;
}

void rawcam_set_timing(int t1, int t2, int t3, int t4, int t5, int term1, int term2) {
	r.rx_timing.timing1 = t1;
	r.rx_timing.timing2 = t2;
	r.rx_timing.timing3 = t3;
	r.rx_timing.timing4 = t4;
	r.rx_timing.timing5 = t5;
	r.rx_timing.timing5 = t5;
	r.rx_timing.term1 = term1;
	r.rx_timing.term2 = term2;
}

void rawcam_set_encoding_fourcc(char a, char b, char c, char d) {
	fprintf(stderr, "rawcam-csi: request: encoding=0x%08x (FOURCC:%c%c%c%c)\n", \
		MMAL_FOURCC(a, b, c, d), a, b, c, d);
		
	r.output->format->encoding = MMAL_FOURCC(a, b, c, d);
}

void rawcam_get_encoding_int() {
	return r.output->format->encoding;
}

void rawcam_set_camera_num(int num) {
	assert(num == 0 || num == 1);
	r.camera_num = num;
}

int rawcam_get_camera_num() {
	//int num = 0;
	//mmal_port_parameter_get_int32(r.output, MMAL_PARAMETER_CAMERA_NUM, &num);
	return r.camera_num;
}

int rawcam_get_buffer_size_recommended() {
	return r.output->buffer_size_recommended;
}

int rawcam_get_buffer_num_recommended() {
	return r.output->buffer_num_recommended;
}

void rawcam_set_zero_copy(int zero_copy) {
	if(zero_copy) {
		r.zero_copy = MMAL_TRUE;
	} else {
		r.zero_copy = MMAL_FALSE;
	}
}

int rawcam_get_zero_copy() {
	MMAL_BOOL_T  res;
	mmal_port_parameter_get_boolean(r.output, MMAL_PARAMETER_ZERO_COPY, &res);
	return res == MMAL_TRUE;
}

bool rawcam_format_commit() {
	return true;
}

void rawcam_debug() {
	fprintf(stderr, "rawcam-csi (" RAWCAM_VERSION "): unpack=%d, pack=%d, image_id=0x%02x, data_lanes=%d, timing=(%d,%d,%d,%d,%d)\n", \
		r.rx_cfg.unpack, r.rx_cfg.pack, r.rx_cfg.image_id, r.rx_cfg.data_lanes, \
		r.rx_timing.timing1, r.rx_timing.timing2, r.rx_timing.timing3, r.rx_timing.timing4, r.rx_timing.timing5);
		
	fprintf(stderr, "rawcam-csi (" RAWCAM_VERSION "): term=(%d,%d), frame_dimensions=crop:(%d x %d) es-video:(%d x %d), camera_num=%d\n", \
		r.rx_timing.term1, r.rx_timing.term2, \
		r.output->format->es->video.crop.width, r.output->format->es->video.crop.height, \
		r.output->format->es->video.width, r.output->format->es->video.height, \
		rawcam_get_camera_num());
		
	fprintf(stderr, "rawcam-csi (" RAWCAM_VERSION "): buffers=set:(%d x %d) out:(%d x %d) recommended:(%d x %d) minimum:(%d x %d)\n", \
		r.buffer_size, r.buffer_num, r.output->buffer_size, r.output->buffer_num, \
		r.output->buffer_size_recommended, r.output->buffer_num_recommended, \
		r.output->buffer_size_min, r.output->buffer_num_min);
		
	fprintf(stderr, "rawcam-csi (" RAWCAM_VERSION "): encoding=0x%08x (FOURCC:%c%c%c%c)\n", r.output->format->encoding, \
		(r.output->format->encoding & 0x000000ff),       (r.output->format->encoding & 0x0000ff00) >>  8, \
		(r.output->format->encoding & 0x00ff0000) >> 16, (r.output->format->encoding & 0xff000000) >> 24);
}

bool do_init(void) {
	fprintf(stderr, "rawcam-csi: rawcam.do_init()\n");
	
	TRY (!(r.eventfd = eventfd(0, 0)), NONE);
	
	bcm_host_init();
	
	TRY (mmal_component_create("vc.ril.rawcam", &r.rawcam), NONE);
	TRY (mmal_component_create("vc.ril.isp", &r.isp), C2);
	
	r.output = r.rawcam->output[0];
	
	r.rx_cfg.hdr = (MMAL_PARAMETER_HEADER_T){ .id=MMAL_PARAMETER_CAMERA_RX_CONFIG, .size=sizeof r.rx_cfg };
	r.rx_timing.hdr = (MMAL_PARAMETER_HEADER_T){ .id=MMAL_PARAMETER_CAMERA_RX_TIMING, .size=sizeof r.rx_timing };
	
	TRY(mmal_port_parameter_get(r.output, &r.rx_cfg.hdr), C2);
	TRY(mmal_port_parameter_get(r.output, &r.rx_timing.hdr), C2);
	r.buffer_num =  r.output->buffer_num;
        r.buffer_size = r.output->buffer_size;

	return true;
}

struct rawcam_interface *rawcam_init (void) {
	fprintf(stderr, "rawcam-csi: rawcam_init()\n");
	
	return (do_init() ? (struct rawcam_interface *)&r : NULL);
}

bool rawcam_start(void) {
	fprintf(stderr, "rawcam-csi: rawcam_start()\n");
	
	r.queue = mmal_queue_create();
	//r.output->buffer_size = r.buffer_size;
	//r.output->buffer_num = r.buffer_num;	
	TRY (mmal_port_parameter_set_int32(r.output, MMAL_PARAMETER_CAMERA_NUM, r.camera_num), C2);
	TRY (mmal_port_parameter_set(r.output, &r.rx_cfg.hdr), C2);
	TRY (mmal_port_parameter_set(r.output, &r.rx_timing.hdr), C2);
	TRY (mmal_port_parameter_set_boolean(r.output, MMAL_PARAMETER_ZERO_COPY, r.zero_copy), C2);
	r.output->format->es->video.crop.width = r.buffer_width;
	r.output->format->es->video.crop.height = r.buffer_height;
	r.output->format->es->video.width = VCOS_ALIGN_UP(r.buffer_width, 16);
	r.output->format->es->video.height = VCOS_ALIGN_UP(r.buffer_height, 16);
	TRY (mmal_port_format_commit(r.output), C2);

	TRY (mmal_component_enable(r.rawcam), C2);
	TRY (mmal_component_enable(r.isp), C2);
	

        TRY (!(r.pool = mmal_port_pool_create(r.output, r.buffer_num, r.buffer_size)), C1);

	fprintf(stderr, "rawcam-csi: enabling port - callback: %p - num buffers %d,%d\n", callback, r.buffer_num, r.output->buffer_num);
	TRY (mmal_port_enable(r.output, callback), POOL);

	for(int i = 0; i < r.buffer_num; i++) {
		MMAL_BUFFER_HEADER_T *buffer;
                TRY (!(buffer = mmal_queue_get(r.pool->queue)), PORT);
                TRY (mmal_port_send_buffer(r.output, buffer), PORT);
	}
	
	atexit (rawcam_stop);

	return true;
}

void rawcam_free(void) {
	/* FIXME */
}
