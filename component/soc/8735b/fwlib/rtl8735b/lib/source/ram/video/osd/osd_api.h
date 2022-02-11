#ifndef OSD_API_H
#define OSD_API_H

#include "hal_osd_util.h"
#include "hal_video.h"

#define MAX_OSD_W 1920
#define MAX_OSD_H 1080

struct osd_rect_info_st {
	int format;
	unsigned char line_width;
	unsigned char rect_clr_r;
	unsigned char rect_clr_g;
	unsigned char rect_clr_b;
	int rect_w;
	int rect_h;
	unsigned char type;
	unsigned char txt_clr_r;
	unsigned char txt_clr_g;
	unsigned char txt_clr_b;
} ;

enum rts_osd2_blk_type {
	rts_osd2_type_none = 0,
	rts_osd2_type_date,
	rts_osd2_type_time,
	rts_osd2_type_pict,
	rts_osd2_type_text
};

enum rts_osd2_err_code {
	rts_osd2_err_success = 0,
	rts_osd2_err_fail,
	rts_osd2_err_wrong_bitmap_range,
	rts_osd2_err_wrong_date,
	rts_osd2_err_wrong_time,
	rts_osd2_err_malloc_fail,
	rts_osd2_err_reserved
};


#define OSD_TEXT_FONT_BG_ENABLE		0
#define OSD_TEXT_FONT_BG_COLOR		RGB_Green
#define OSD_TEXT_FONT_CH_COLOR		RGB_White
#define OSD_TEXT_FONT_BLOCK_ALPHA	10	// 0~15
#define OSD_TEXT_FONT_H_GAP			0
#define OSD_TEXT_FONT_V_GAP			0
#define OSD_TEXT_START_X			10
#define OSD_TEXT_START_Y			10
#define OSD_TEXT_ROTATE				RT_ROTATE_0
#define OSD_TEXT_BMP_BUF_SZ			(80*0x00000400)


struct osd_info {
	void *info;
	char osd_type;
	char update_flag;
};

enum rts_osd2_err_code rts_osd_set_info(int osd_type, void *osd_info);
void rts_osd_init(int char_resize_w, int char_resize_h);
int rts_osd_text2bmp(char *str, BITMAP_S *pbmp, int ch);
enum rts_osd2_err_code rts_osd_rect_gen_with_txt(BITMAP_S *txt_bitmap, void *buf, int pic_w, int pic_h, struct osd_rect_info_st rect_info, int update_rect);
void rts_osd_bitmap_update(int ch, rt_osd2_info_st *posd2_pic, BOOL ready2update);
void rts_osd_hide_bitmap(int ch, rt_osd2_info_st *posd2_pic);
void rts_osd_deinit();

#endif	// OSD_API_H
