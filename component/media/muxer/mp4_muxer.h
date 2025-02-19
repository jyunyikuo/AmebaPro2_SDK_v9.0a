#ifndef _MP4_MUXER_H_
#define _MP4_MUXER_H_

#include "basic_types.h"
#include "osdep_service.h"
#include "fatfs_sdcard_api.h"

#include "device.h"
#include "diag.h"
#include "dma_api.h"

#define MAX_BOX_CHILD 8
#define RBUF_SIZE 1024

typedef struct _mp4root {
	unsigned int video_len;
	unsigned int audio_len;
	unsigned int total;
	unsigned int keyindex;
} MP4root, *PMP4root;

#define STORAGE_IDLE            0
#define STORAGE_INIT            1
#define STORAGE_START           2
#define STORAGE_WRITE_ALL       3
#define STORAGE_WRITE_VIDEO     4
#define STORAGE_WRITE_AUDIO     5
#define STORAGE_STOP            6
#define STORAGE_END             7
#define STORAGE_ERROR           8

#define STORAGE_ALL     0
#define STORAGE_VIDEO   1
#define STORAGE_AUDIO   2

typedef struct _mp4_payload {
	unsigned char *addr;
	unsigned int  len;
} mp4_payload, pmp4_payload;

#define MP4_DMA_BLOCK_LENGTH 4092
#define MP4_DMA_BLOCK_NUM 16
#define MP4_DMA_SIZE (MP4_DMA_BLOCK_LENGTH*MP4_DMA_BLOCK_NUM)


struct BlockInfo {
	u32 SrcAddr;
	u32 DstAddr;
	u32 BlockLength;
	u32 SrcOffset;
	u32 DstOffset;
};

//SDRAM_DATA_SECTION struct BlockInfo block_info[MP4_DMA_BLOCK_NUM];
#define AVCC_TYPE 0X00
#define HVCC_TYPE 0X01

#define AUDIO_AAC  0X00
#define AUDIO_ULAW 0X01
#define AUDIO_ALAW 0X02
typedef struct _mp4_context {
	FATFS       m_fs;
	int         drv_num;
	char        _drv[4];
	FIL         m_file;
	char        filename[128];
	int         Fatfs_ok;
	int         filecount;
	int         width;
	int         height;
	int         sample_rate;
	int         channel_count;
	int         frame_rate;
	int			gop;
	u32         start_time;//rtw_get_current_time()
	u32         file_name_index;
	u32         storage_state;
	MP4root     root;
	int         period_time;
	int         file_total;
	int         sps_start;
	int         type;
	int         vps_len;
	int         sps_len;
	int         pps_len;
	int         sps_pps_mark;
	unsigned    int *video_buffer_index;
	unsigned    int *video_buffer_size;
	unsigned    int video_size;
	unsigned    int *key_frame;
	unsigned    int key_frame_size;
	unsigned    int *audio_buffer_index;
	unsigned    int *audio_buffer_size;
	unsigned    int audio_size;
	unsigned    char *moov_box;
	unsigned    int moov_box_size;
	unsigned    char *fatfs_buf;
	unsigned    int fatfs_buf_size;
	unsigned    int fatfs_buf_pos;
	mp4_payload payload;
	int         write_status;
	int         nal_len;
	int         h264_extra_len;
	char        vps_str[64];
	char        sps_str[64];
	char        pps_str[64];
	unsigned    char ftyp_box[24];
	unsigned    char mdat_box[8];
	int         audio_clock_rate;
	int         video_clock_rate;
	unsigned    int  video_old_depend_clock_tick;
	unsigned    int  audio_old_depend_clock_tick;
	unsigned    int  video_timestamp;
	unsigned    int  audio_timestamp;
	unsigned    int  video_timestamp_first;
	unsigned    int  audio_timestamp_first;
	unsigned    int  *video_timestamp_buffer;
	unsigned    int  *audio_timestamp_buffer;
	int (*cb_start)(void *);
	int (*cb_stop)(void *);
	int (*cb_end)(void *);
	int (*cb_error)(void *);
	int (*cb_fwrite)(void *, void *, unsigned int, unsigned int *); //Start
	int (*cb_fseek)(void *, unsigned int);
	int (*cb_fopen)(void *, const char *, char mode);
	int (*cb_fclose)(void *);
	int (* sd_card_is_mounted_cb)(void);//Provide the sd card detection, 1:The sd card is alive 0:The sd card is removed
	int loop_mode;
	int remove_append_name;//1. without .mp4 0.default
	int moov_size;
	gdma_t gdma;
	int audio_appear_first;
	int video_appear_first;
	int audio_start;
	int video_start ;
	_sema	MP4_DMA_DONE;
	struct BlockInfo block_info[MP4_DMA_BLOCK_NUM];
	void *fp_handle;
	int mp4_user_callback;
	int encoder_type;
	int audio_format;
	int audio_duration;//For g711
} mp4_context, *pmp4_context;

void mp4_muxer_init(pmp4_context mp4_ctx);
void mp4_muxer_close(pmp4_context mp4_ctx);
void set_mp4_fatfs_param(pmp4_context mp4_ctx, fatfs_sd_params_t *fatfs_param);

u8 mp4_is_recording(pmp4_context mp4_ctx);
int mp4_start_record(pmp4_context mp4_ctx, int file_num);
int mp4_stop_record(pmp4_context mp4_ctx);
int mp4_stop_record_immediately(pmp4_context mp4_ctx);
void mp4_muxer_handle(pmp4_context mp4_ctx, unsigned char *buf, unsigned int size, int type, unsigned int timestamp);

#endif //_MP4_MUXER_H_
