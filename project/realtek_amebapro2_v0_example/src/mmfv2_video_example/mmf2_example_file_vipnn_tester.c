/******************************************************************************
*
* Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "module_video.h"
#include "avcodec.h"

#include "module_vipnn.h"
#include "module_fileloader.h"
#include "module_filesaver.h"

#include "avcodec.h"
#include <cJSON.h>

#include "model_mbnetssd.h"
#include "model_yolov3t.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG
#include "../image/3rdparty/stb/stb_image.h"

#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include "sdio_combine.h"
#include "sdio_host.h"
#include <disk_if/inc/sdcard.h>
#include "fatfs_sdcard_api.h"
static FIL      m_file;
static fatfs_sd_params_t fatfs_sd;

static char pascal_voc_name[21][20] =
{ "background",    "aeroplane",    "bicycle",    "bird",    "boat",    "bottle",    "bus",    "car",    "cat",    "chair",    "cow",    "diningtable",    "dog",    "horse",    "motorbike",    "person",    "pottedplant",    "sheep",    "sofa",    "train",    "tvmonitor" };
static char coco_name[80][20] =
{ "person",    "bicycle",    "car",    "motorbike",    "aeroplane",    "bus",    "train",    "truck",    "boat",    "traffic light",    "fire hydrant",    "stop sign",    "parking meter",    "bench",    "bird",    "cat",    "dog",    "horse",    "sheep",    "cow",    "elephant",    "bear",    "zebra",    "giraffe",    "backpack",    "umbrella",    "handbag",    "tie",    "suitcase",    "frisbee",    "skis",    "snowboard",    "sports ball",    "kite",    "baseball bat",    "baseball glove",    "skateboard",    "surfboard",    "tennis racket",    "bottle",    "wine glass",    "cup",    "fork",    "knife",    "spoon",    "bowl",    "banana",    "apple",    "sandwich",    "orange",    "broccoli",    "carrot",    "hot dog",    "pizza",    "donut",    "cake",    "chair",    "sofa",    "pottedplant",    "bed",    "diningtable",    "toilet",    "tvmonitor",    "laptop",    "mouse",    "remote",    "keyboard",    "cell phone",    "microwave",    "oven",    "toaster",    "sink",    "refrigerator",    "book",    "clock",    "vase",    "scissors",    "teddy bear",    "hair drier",    "toothbrush" };

#define MOBILENET_SSD_MODEL     1
#define YOLO_MODEL              2
#define USE_NN_MODEL            YOLO_MODEL   /* Fix me.  ( MOBILENET_SSD_MODEL or YOLO_MODEL ) */

#define TEST_IMAGE_WIDTH	416   /* Fix me */
#define TEST_IMAGE_HEIGHT	416   /* Fix me */
nn_data_param_t roi_tester = {
	.img = {
		.width = TEST_IMAGE_WIDTH,
		.height = TEST_IMAGE_HEIGHT,
		.rgb = 1,
		.roi = {
			.xmin = 0,
			.ymin = 0,
			.xmax = TEST_IMAGE_WIDTH,
			.ymax = TEST_IMAGE_HEIGHT,
		}
	}
};

static fileloader_params_t test_image_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.codec_id = AV_CODEC_ID_BMP,       /* Fix me (AV_CODEC_ID_BMP or AV_CODEC_ID_JPEG) */
	.test_data_width = TEST_IMAGE_WIDTH,
	.test_data_height = TEST_IMAGE_HEIGHT
};

static int ImageDecodeToRGB888planar_ConvertInPlace(void *pbuffer, void *pbuffer_size);
static char *nn_get_json_format(void *p, int frame_id, char *file_name);
static void nn_save_handler(char *file_name, uint32_t data_addr, uint32_t data_size);
static int sd_save_file(char *file_name, char *data_buf, int data_buf_size);

static mm_context_t *fileloader_ctx			= NULL;
static mm_context_t *filesaver_ctx			= NULL;
static mm_context_t *vipnn_ctx              = NULL;

static mm_siso_t *siso_fileloader_vipnn     = NULL;
static mm_siso_t *siso_vipnn_filesaver      = NULL;

#define TEST_FILE_PATH_PREFIX   "test_bmp/image"            /* Fix me */   // Ex: test_bmp/image-0001.bmp, test_bmp/image-0002.bmp, test_bmp/image-0003.bmp ...
#define TEST_FILE_NUM           10                          /* Fix me */

#define FILE_OUT_PATH_PREFIX    "nn_result/nn_result"       /* Fix me */

void mmf2_example_file_vipnn_tester(void)
{
	fileloader_ctx = mm_module_open(&fileloader_module);
	if (fileloader_ctx) {
		mm_module_ctrl(fileloader_ctx, CMD_FILELOADER_SET_PARAMS, (int)&test_image_params);
		mm_module_ctrl(fileloader_ctx, CMD_FILELOADER_SET_TEST_FILE_PATH, (int)TEST_FILE_PATH_PREFIX);
		mm_module_ctrl(fileloader_ctx, CMD_FILELOADER_SET_FILE_NUM, (int)TEST_FILE_NUM);
		mm_module_ctrl(fileloader_ctx, CMD_FILELOADER_SET_DECODE_PROCESS, (int)ImageDecodeToRGB888planar_ConvertInPlace);
		mm_module_ctrl(fileloader_ctx, MM_CMD_SET_QUEUE_LEN, 1);  //set to 1 when using NN file tester
		mm_module_ctrl(fileloader_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(fileloader_ctx, CMD_FILELOADER_APPLY, 0);
	} else {
		rt_printf("fileloader open fail\n\r");
		goto mmf2_example_file_vipnn_tester_fail;
	}
	rt_printf("fileloader opened\n\r");

	// VIPNN
	vipnn_ctx = mm_module_open(&vipnn_module);
	if (vipnn_ctx) {
#if USE_NN_MODEL == MOBILENET_SSD_MODEL
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&mbnetssd_fwfs);
#elif USE_NN_MODEL == YOLO_MODEL
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_MODEL, (int)&yolov3_tiny_fwfs);
#endif
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_IN_PARAMS, (int)&roi_tester);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_SET_OUTPUT, 1);  //enable module output
		mm_module_ctrl(vipnn_ctx, MM_CMD_SET_QUEUE_LEN, 1);  //set to 1 when using NN file tester
		mm_module_ctrl(vipnn_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(vipnn_ctx, CMD_VIPNN_APPLY, 0);
	} else {
		rt_printf("VIPNN open fail\n\r");
		goto mmf2_example_file_vipnn_tester_fail;
	}
	rt_printf("VIPNN opened\n\r");

	filesaver_ctx = mm_module_open(&filesaver_module);
	if (filesaver_ctx) {
		mm_module_ctrl(filesaver_ctx, CMD_FILESAVER_SET_SAVE_FILE_PATH, (int)FILE_OUT_PATH_PREFIX);
		mm_module_ctrl(filesaver_ctx, CMD_FILESAVER_SET_TYPE_HANDLER, (int)nn_save_handler);
		mm_module_ctrl(filesaver_ctx, CMD_FILESAVER_APPLY, 0);
	} else {
		rt_printf("filesaver open fail\n\r");
		goto mmf2_example_file_vipnn_tester_fail;
	}
	rt_printf("filesaver opened\n\r");

	//--------------Link---------------------------

	siso_fileloader_vipnn = siso_create();
	if (siso_fileloader_vipnn) {
		siso_ctrl(siso_fileloader_vipnn, MMIC_CMD_ADD_INPUT, (uint32_t)fileloader_ctx, 0);
		siso_ctrl(siso_fileloader_vipnn, MMIC_CMD_ADD_OUTPUT, (uint32_t)vipnn_ctx, 0);
		siso_start(siso_fileloader_vipnn);
	} else {
		rt_printf("siso_fileloader_vipnn open fail\n\r");
		goto mmf2_example_file_vipnn_tester_fail;
	}
	rt_printf("siso_fileloader_vipnn started\n\r");

	siso_vipnn_filesaver = siso_create();
	if (siso_vipnn_filesaver) {
		siso_ctrl(siso_vipnn_filesaver, MMIC_CMD_ADD_INPUT, (uint32_t)vipnn_ctx, 0);
		siso_ctrl(siso_vipnn_filesaver, MMIC_CMD_ADD_OUTPUT, (uint32_t)filesaver_ctx, 0);
		siso_start(siso_vipnn_filesaver);
	} else {
		rt_printf("siso_vipnn_filesaver open fail\n\r");
		goto mmf2_example_file_vipnn_tester_fail;
	}
	rt_printf("siso_vipnn_filesaver started\n\r");

	return;
mmf2_example_file_vipnn_tester_fail:

	return;
}

/*-----------------------------------------------------------------------------------*/

static int ImageDecodeToRGB888planar_ConvertInPlace(void *pbuffer, void *pbuffer_size)
{
	char *bmp2rgb_buffer = (char *)pbuffer;
	uint32_t *bmp2rgb_size = (uint32_t *)pbuffer_size;

	int w, h, c;
	int channels = 3;
	char *im_data = stbi_load_from_memory(bmp2rgb_buffer, *bmp2rgb_size, &w, &h, &c, channels);
	printf("\r\nimage data size: w:%d, h:%d, c:%d\r\n", w, h, c);

	int data_size = w * h * c;
	char *rgb_planar_buf = (char *)malloc(data_size);
	for (int k = 0; k < c; k++) {
		for (int j = 0; j < h; j++) {
			for (int i = 0; i < w; i++) {
				int dst_i = i + w * j + w * h * k;
				int src_i = k + c * i + c * w * j;
				rgb_planar_buf[dst_i] = im_data[src_i];
			}
		}
	}

	memcpy(bmp2rgb_buffer, rgb_planar_buf, data_size);
	*bmp2rgb_size = (uint32_t) data_size;

	free(rgb_planar_buf);
	stbi_image_free(im_data);

	return 1;
}

static char *nn_get_json_format(void *p, int frame_id, char *file_name)
{
	objdetect_res_t *res = (objdetect_res_t *)p;

	/**** cJSON ****/
	cJSON_Hooks memoryHook;
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);

	cJSON *nnJSObject = NULL, *nn_obj_JSObject = NULL;
	cJSON *nn_coor_JSObject = NULL, *nn_obj_JSArray = NULL;
	char *nn_json_string = NULL;

	nnJSObject = cJSON_CreateObject();
	cJSON_AddItemToObject(nnJSObject, "frame_id", cJSON_CreateNumber(frame_id));
	cJSON_AddItemToObject(nnJSObject, "filename", cJSON_CreateString(file_name));
	cJSON_AddItemToObject(nnJSObject, "objects", nn_obj_JSArray = cJSON_CreateArray());

	int im_w = TEST_IMAGE_WIDTH;
	int im_h = TEST_IMAGE_HEIGHT;

	printf("object num = %d\r\n", res->obj_num);
	if (res->obj_num > 0) {
		for (int i = 0; i < res->obj_num; i++) {

			int top_x = (int)(res->result[6 * i + 2] * im_w) < 0 ? 0 : (int)(res->result[6 * i + 2] * im_w);
			int top_y = (int)(res->result[6 * i + 3] * im_h) < 0 ? 0 : (int)(res->result[6 * i + 3] * im_h);
			int bottom_x = (int)(res->result[6 * i + 4] * im_w) > im_w ? im_w : (int)(res->result[6 * i + 4] * im_w);
			int bottom_y = (int)(res->result[6 * i + 5] * im_h) > im_h ? im_h : (int)(res->result[6 * i + 5] * im_h);

			printf("%d,c%d:%d %d %d %d\n\r", i, (int)(res->result[6 * i]), top_x, top_y, bottom_x, bottom_y);

			cJSON_AddItemToArray(nn_obj_JSArray, nn_obj_JSObject = cJSON_CreateObject());
			cJSON_AddItemToObject(nn_obj_JSObject, "class_id", cJSON_CreateNumber((int)res->result[6 * i ]));

#if USE_NN_MODEL == MOBILENET_SSD_MODEL
			cJSON_AddItemToObject(nn_obj_JSObject, "name", cJSON_CreateString(pascal_voc_name[(int)res->result[6 * i ]]));
#elif USE_NN_MODEL == YOLO_MODEL
			cJSON_AddItemToObject(nn_obj_JSObject, "name", cJSON_CreateString(coco_name[(int)res->result[6 * i ]]));
#endif

			cJSON_AddItemToObject(nn_obj_JSObject, "relative_coordinates", nn_coor_JSObject = cJSON_CreateObject());
			cJSON_AddItemToObject(nn_coor_JSObject, "top_x", cJSON_CreateNumber(top_x));
			cJSON_AddItemToObject(nn_coor_JSObject, "top_y", cJSON_CreateNumber(top_y));
			cJSON_AddItemToObject(nn_coor_JSObject, "bottom_x", cJSON_CreateNumber(bottom_x));
			cJSON_AddItemToObject(nn_coor_JSObject, "bottom_y", cJSON_CreateNumber(bottom_y));

			cJSON_AddItemToObject(nn_obj_JSObject, "probability", cJSON_CreateNumber((float)res->result[6 * i + 1]));
		}
	}

	nn_json_string = cJSON_Print(nnJSObject);
	cJSON_Delete(nnJSObject);
	return nn_json_string;
}

static int saver_count = 0;

static void nn_save_handler(char *file_name, uint32_t data_addr, uint32_t data_size)
{
	VIPNN_OUT_BUFFER pre_tensor_out;
	memcpy(&pre_tensor_out, data_addr, data_size);

	char nn_fn[64];
	memset(&nn_fn[0], 0x00, sizeof(nn_fn));

	/* save yolo json result */
	snprintf(nn_fn, sizeof(nn_fn), "%s_%d.json", file_name, saver_count + 1);
	char *json_format_out = nn_get_json_format(&pre_tensor_out.vipnn_res, saver_count + 1, nn_fn);
	//printf("\r\njson_format_out: %s\r\n", json_format_out);
	sd_save_file(nn_fn, json_format_out, strlen(json_format_out));

	/* save tensor */
	for (int i = 0; i < pre_tensor_out.vipnn_out_tensor_num; i++) {
		/* save raw tensor */
		memset(&nn_fn[0], 0x00, sizeof(nn_fn));
		snprintf(nn_fn, sizeof(nn_fn), "%s_out_tensor%d_uint8_%d.bin", file_name, i, saver_count + 1);
		sd_save_file(nn_fn, (char *)pre_tensor_out.vipnn_out_tensor[i], pre_tensor_out.vipnn_out_tensor_size[i]); /* raw tensor*/

#if 0
		/* save float32 tensor */
		memset(&nn_fn[0], 0x00, sizeof(nn_fn));
		snprintf(nn_fn, sizeof(nn_fn), "%s_out_tensor%d_float32_%d.bin", file_name, i, saver_count + 1);
		float *float_tensor;
		switch (pre_tensor_out.quant_format[i]) {
		case VIP_BUFFER_QUANTIZE_TF_ASYMM:   /* uint8 --> float32 */
			float_tensor = (float *)malloc(pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float));
			for (int k = 0; k < pre_tensor_out.vipnn_out_tensor_size[i]; k++) {
				float_tensor[k] = (*((uint8_t *)pre_tensor_out.vipnn_out_tensor[i] + k) - pre_tensor_out.quant_data[i].affine.zeroPoint) *
								  pre_tensor_out.quant_data[i].affine.scale;
			}
			sd_save_file(nn_fn, (char *)float_tensor, pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float));
			break;
		case VIP_BUFFER_QUANTIZE_DYNAMIC_FIXED_POINT:   /* int16 --> float32 */
			float_tensor = (float *)malloc(pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float) / sizeof(int16_t));
			for (int k = 0; k < (pre_tensor_out.vipnn_out_tensor_size[i] / sizeof(int16_t)); k++) {
				float_tensor[k] = (float)(*((int16_t *)pre_tensor_out.vipnn_out_tensor[i] + k)) / ((float)(1 << pre_tensor_out.quant_data[i].dfp.fixed_point_pos));
			}
			sd_save_file(nn_fn, (char *)float_tensor, pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float) / sizeof(int16_t));
			break;
		default:   /* float16 --> float32 */
			float_tensor = (float *)malloc(pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float) / sizeof(__fp16));
			for (int k = 0; k < (pre_tensor_out.vipnn_out_tensor_size[i] / sizeof(__fp16)); k++) {
				float_tensor[k] = (float)(*((__fp16 *)pre_tensor_out.vipnn_out_tensor[i] + k));
			}
			sd_save_file(nn_fn, (char *)float_tensor, pre_tensor_out.vipnn_out_tensor_size[i] * sizeof(float) / sizeof(__fp16));
		}
		free(float_tensor);
#endif
	}

	saver_count++;
}

static int sd_save_file(char *file_name, char *data_buf, int data_buf_size)
{
	int bw;
	FRESULT res;
	char path_all[64];

	fatfs_sd_get_param(&fatfs_sd);
	snprintf(path_all, sizeof(path_all), "%s%s", fatfs_sd.drv, file_name);

	res = f_open(&m_file, path_all, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	if (res) {
		printf("open file (%s) fail.\n", file_name);
		return 0;
	}

	printf("\r\nfile name: %s", file_name);

	char *WRBuf = (char *)malloc(data_buf_size);
	memset(WRBuf, 0x00, data_buf_size);
	memcpy(WRBuf, data_buf, data_buf_size);

	do {
		res = f_write(&m_file, WRBuf, data_buf_size, (u32 *)&bw);
		if (res) {
			f_lseek(&m_file, 0);
			printf("Write error.\n");
			return 0;
		}
		printf("\r\nWrite %d bytes.\n", bw);
	} while (bw < data_buf_size);

	free(WRBuf);

	f_lseek(&m_file, 0);
	if (f_close(&m_file)) {
		printf("close file (%s) fail.\n", file_name);
		return 0;
	}
	printf("\r\close file (%s) done.\n\n\r", file_name);

	return 1;
}

/*-----------------------------------------------------------------------------------*/
