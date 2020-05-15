#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_connection.h"
#include <stdbool.h>

struct rawcam_interface {
  MMAL_PARAMETER_CAMERA_RX_CONFIG_T rx_cfg;
  MMAL_PARAMETER_CAMERA_RX_TIMING_T rx_timing;
  int camera_num;
  int buffer_num;
  int buffer_size;
  int running;
  int eventfd;
};

struct rawcam_interface *rawcam_init (void);
bool rawcam_start(void);
MMAL_BUFFER_HEADER_T *rawcam_buffer_get(void);
unsigned int rawcam_buffer_count(void);
void rawcam_buffer_free(MMAL_BUFFER_HEADER_T *buffer);
void rawcam_stop(void);
void rawcam_free(void);

unsigned int rawcam_buffer_count(void);
void rawcam_buffer_free(MMAL_BUFFER_HEADER_T *buffer);
void rawcam_set_buffer_size(int buffer_size);
int rawcam_get_buffer_size();
void rawcam_set_buffer_num(int buffer_num);
int rawcam_get_buffer_num();
void rawcam_set_image_id(uint8_t image_id);
uint8_t rawcam_get_image_id();
void rawcam_set_data_lanes(uint8_t n_lanes);
int rawcam_get_data_lanes();
void rawcam_set_buffer_dimensions(int width, int height);
void rawcam_set_decode_mode(int decode_mode);
int rawcam_get_decode_mode();
void rawcam_set_encode_mode(int encode_mode);
int rawcam_get_encode_mode();
void rawcam_set_unpack_mode(int unpack_mode);
int rawcam_get_unpack_mode();
void rawcam_set_pack_mode(int pack_mode);
int rawcam_get_pack_mode();
void rawcam_set_encode_block_length(int encode);
int rawcam_get_encode_block_length();
void rawcam_set_embedded_data_lines(int data_lines);
int rawcam_get_embedded_data_lines();
void rawcam_set_timing(int t1, int t2, int t3, int t4, int t5, int term1, int term2);
void rawcam_set_encoding_fourcc(char a, char b, char c, char d);
void rawcam_get_encoding_int();
void rawcam_set_camera_num(int num);
int rawcam_get_camera_num();
int rawcam_get_buffer_size_recommended();
int rawcam_get_buffer_num_recommended();
void rawcam_set_zero_copy(int zero_copy);
int rawcam_get_zero_copy();

bool rawcam_format_commit();
void rawcam_debug();
int rawcam_get_eventfd();
