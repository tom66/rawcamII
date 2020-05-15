#include "rawcam.h"
#include <stdio.h>

static void print_rx_config (MMAL_PARAMETER_CAMERA_RX_CONFIG_T *r) {
#define NAME(n,names) ((n>=0 && n<(sizeof names/sizeof names[0]) ? names[n] : "???"))
	const char *decode[] = {
		"MMAL_CAMERA_RX_CONFIG_DECODE_NONE",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM8TO10",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM7TO10",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM6TO10",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM8TO12",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM7TO12",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM6TO12",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM10TO14",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM8TO14",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM12TO16",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM10TO16",
		"MMAL_CAMERA_RX_CONFIG_DECODE_DPCM8TO16",
	}, *encode[] = {
		"MMAL_CAMERA_RX_CONFIG_ENCODE_NONE",
		"MMAL_CAMERA_RX_CONFIG_ENCODE_DPCM10TO8",
		"MMAL_CAMERA_RX_CONFIG_ENCODE_DPCM12TO8",
		"MMAL_CAMERA_RX_CONFIG_ENCODE_DPCM14TO8",
	}, *unpack[] = {
		"MMAL_CAMERA_RX_CONFIG_UNPACK_NONE",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_6",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_7",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_8",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_10",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_12",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_14",
		"MMAL_CAMERA_RX_CONFIG_UNPACK_16",
	}, *pack[] = {
		"MMAL_CAMERA_RX_CONFIG_PACK_NONE",
		"MMAL_CAMERA_RX_CONFIG_PACK_8",
		"MMAL_CAMERA_RX_CONFIG_PACK_10",
		"MMAL_CAMERA_RX_CONFIG_PACK_12",
		"MMAL_CAMERA_RX_CONFIG_PACK_14",
		"MMAL_CAMERA_RX_CONFIG_PACK_16",
		"MMAL_CAMERA_RX_CONFIG_PACK_RAW10",
		"MMAL_CAMERA_RX_CONFIG_PACK_RAW12",
	};
	printf ("rx config:\n"
		"decode: %s\n"
		"encode: %s\n"
		"pack: %s\n"
		"unpack: %s\n"
		"data_lanes: %u\n"
		"encode_block_length: %u\n"
		"embedded_data_lines: %u\n"
		"image_id: %u\n",
		NAME(r->decode, decode),
		NAME(r->encode, encode),
		NAME(r->pack, pack),
		NAME(r->unpack, unpack),
		r->data_lanes,
		r->encode_block_length,
		r->embedded_data_lines,
		r->image_id);
}

void print_rx_timing(MMAL_PARAMETER_CAMERA_RX_TIMING_T *t) {
	printf("rx_timings: %u %u %u %u %u / %u %u / %u %u\n",
		t->timing1, t->timing2, t->timing3, t->timing4, t->timing5,
		t->term1, t->term2, t->cpi_timing1, t->cpi_timing2);
}

int main(void) {
	struct rawcam_interface *r;
	if (!(r = rawcam_init()))
		return 1;
	print_rx_config(&r->rx_cfg);
	print_rx_timing(&r->rx_timing);
	fprintf (stderr, "%d buffers of size %d\n", r->buffer_num, r->buffer_size);
	/* tweak config here */
	//r->buffer_size = 262144;
	
	if (!rawcam_start())
		return 1;
	r->running = 1;
	do {
		uint64_t u;
		read (r->eventfd, &u, sizeof u);
		fprintf (stderr, "efd got %lld, queue len %u\n", u, rawcam_buffer_count());
		if (r->running) {
			while (u--) {
				MMAL_BUFFER_HEADER_T *buffer = rawcam_buffer_get();
				fprintf (stderr, "buffer %p, size %d\n", buffer, buffer->length);
				write(3, buffer->data, buffer->length);
				rawcam_buffer_free(buffer);
			}
		} else {
			printf ("aborted\n");
		}
	} while (r->running);

	return 0;
}
