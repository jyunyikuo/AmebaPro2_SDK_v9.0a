/**************************************************************************//**
 * @file     hal_video_common.h
 * @brief    The HAL API implementation for the Video device.
 * @version  V1.00
 * @date     2021-01-14
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/


#ifndef _HAL_VIDEO_COMMON_H_
#define _HAL_VIDEO_COMMON_H_

#define IS_HEVC(a)  (a==VCENC_VIDEO_CODEC_HEVC)
#define IS_H264(a)  (a==VCENC_VIDEO_CODEC_H264)

#define MAX(a, b) ((a) > (b) ?  (a) : (b))
#define CLIP3(x, y, z)  ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))



typedef const void *VCEncInst;

/* Function return values */
typedef enum {
	VCENC_OK = 0,
	VCENC_FRAME_READY = 1,
	VCENC_FRAME_ENQUEUE = 2,

	VCENC_ERROR = -1,
	VCENC_NULL_ARGUMENT = -2,
	VCENC_INVALID_ARGUMENT = -3,
	VCENC_MEMORY_ERROR = -4,
	VCENC_EWL_ERROR = -5,
	VCENC_EWL_MEMORY_ERROR = -6,
	VCENC_INVALID_STATUS = -7,
	VCENC_OUTPUT_BUFFER_OVERFLOW = -8,
	VCENC_HW_BUS_ERROR = -9,
	VCENC_HW_DATA_ERROR = -10,
	VCENC_HW_TIMEOUT = -11,
	VCENC_HW_RESERVED = -12,
	VCENC_SYSTEM_ERROR = -13,
	VCENC_INSTANCE_ERROR = -14,
	VCENC_HRD_ERROR = -15,
	VCENC_HW_RESET = -16
} VCEncRet;

/* Video Codec Format */
typedef enum {
	VCENC_VIDEO_CODEC_HEVC = 0,
	VCENC_VIDEO_CODEC_H264 = 1,
	VCENC_VIDEO_CODEC_JPEG = 2,
	VCENC_VIDEO_CODEC_NV12 = 3,
	VCENC_VIDEO_CODEC_RGB = 4,
	VCENC_VIDEO_CODEC_NV16 = 5

} VCEncVideoCodecFormat;

/* Stream type for initialization */
typedef enum {
	VCENC_BYTE_STREAM = 0,    /* NAL unit starts with hex bytes '00 00 00 01' */
	VCENC_NAL_UNIT_STREAM = 1 /* Plain NAL units without startcode */
} VCEncStreamType;

/* Level for initialization */
typedef enum {
	VCENC_HEVC_LEVEL_1 = 30,
	VCENC_HEVC_LEVEL_2 = 60,
	VCENC_HEVC_LEVEL_2_1 = 63,
	VCENC_HEVC_LEVEL_3 = 90,
	VCENC_HEVC_LEVEL_3_1 = 93,
	VCENC_HEVC_LEVEL_4 = 120,
	VCENC_HEVC_LEVEL_4_1 = 123,
	VCENC_HEVC_LEVEL_5 = 150,
	VCENC_HEVC_LEVEL_5_1 = 153,
	VCENC_HEVC_LEVEL_5_2 = 156,
	VCENC_HEVC_LEVEL_6 = 180,
	VCENC_HEVC_LEVEL_6_1 = 183,
	VCENC_HEVC_LEVEL_6_2 = 186,

	/* H264 Defination*/
	VCENC_H264_LEVEL_1 = 10,
	VCENC_H264_LEVEL_1_b = 99,
	VCENC_H264_LEVEL_1_1 = 11,
	VCENC_H264_LEVEL_1_2 = 12,
	VCENC_H264_LEVEL_1_3 = 13,
	VCENC_H264_LEVEL_2 = 20,
	VCENC_H264_LEVEL_2_1 = 21,
	VCENC_H264_LEVEL_2_2 = 22,
	VCENC_H264_LEVEL_3 = 30,
	VCENC_H264_LEVEL_3_1 = 31,
	VCENC_H264_LEVEL_3_2 = 32,
	VCENC_H264_LEVEL_4 = 40,
	VCENC_H264_LEVEL_4_1 = 41,
	VCENC_H264_LEVEL_4_2 = 42,
	VCENC_H264_LEVEL_5 = 50,
	VCENC_H264_LEVEL_5_1 = 51,
	VCENC_H264_LEVEL_5_2 = 52,
	VCENC_H264_LEVEL_6 = 60,
	VCENC_H264_LEVEL_6_1 = 61,
	VCENC_H264_LEVEL_6_2 = 62
} VCEncLevel;

/* Profile for initialization */
typedef enum {
	VCENC_HEVC_MAIN_PROFILE = 0,
	VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE = 1,
	VCENC_HEVC_MAIN_10_PROFILE = 2,
	VCENC_HEVC_MAINREXT = 3,
	/* H264 Defination*/
	VCENC_H264_BASE_PROFILE = 9,
	VCENC_H264_MAIN_PROFILE = 10,
	VCENC_H264_HIGH_PROFILE = 11,
	VCENC_H264_HIGH_10_PROFILE = 12,

} VCEncProfile;

/* Tier for initialization */
typedef enum {
	VCENC_HEVC_MAIN_TIER = 0,
	VCENC_HEVC_HIGH_TIER = 1,
} VCEncTier;

/* Picture YUV type for initialization */
typedef enum {
	VCENC_YUV420_PLANAR = 0,                  /* YYYY... UUUU... VVVV...  */
	VCENC_YUV420_SEMIPLANAR = 1,              /* YYYY... UVUVUV...        */
	VCENC_YUV420_SEMIPLANAR_VU = 2,           /* YYYY... VUVUVU...        */
	VCENC_YUV422_INTERLEAVED_YUYV = 3,        /* YUYVYUYV...              */
	VCENC_YUV422_INTERLEAVED_UYVY = 4,        /* UYVYUYVY...              */
	VCENC_RGB565 = 5,                         /* 16-bit RGB 16bpp         */
	VCENC_BGR565 = 6,                         /* 16-bit RGB 16bpp         */
	VCENC_RGB555 = 7,                         /* 15-bit RGB 16bpp         */
	VCENC_BGR555 = 8,                         /* 15-bit RGB 16bpp         */
	VCENC_RGB444 = 9,                         /* 12-bit RGB 16bpp         */
	VCENC_BGR444 = 10,                         /* 12-bit RGB 16bpp         */
	VCENC_RGB888 = 11,                         /* 24-bit RGB 32bpp         */
	VCENC_BGR888 = 12,                         /* 24-bit RGB 32bpp         */

	VCENC_FORMAT_MAX

} VCEncPictureType;

/* Picture rotation for pre-processing */
typedef enum {
	VCENC_ROTATE_0 = 0,
	VCENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
	VCENC_ROTATE_90L = 2,  /* Rotate 90 degrees counter-clockwise */
	VCENC_ROTATE_180R = 3  /* Rotate 180 degrees clockwise */
} VCEncPictureRotation;
#if 0
/* Picture mirror for pre-processing */

typedef enum {
	VCENC_MIRROR_NO = 0, /* no mirror */
	VCENC_MIRROR_YES = 1 /* mirror */
} VCEncPictureMirror;
#endif

/* Picture color space conversion (RGB input) for pre-processing */
typedef enum {
	VCENC_RGBTOYUV_BT601 = 0, /* Color conversion of limited range[16,235] according to BT.601 */
	VCENC_RGBTOYUV_BT709 = 1, /* Color conversion of limited range[16,235] according to BT.709 */
	VCENC_RGBTOYUV_USER_DEFINED = 2,   /* User defined color conversion */
	VCENC_RGBTOYUV_BT2020 = 3, /* Color conversion according to BT.2020 */
	VCENC_RGBTOYUV_BT601_FULL_RANGE = 4, /* Color conversion of full range[0,255] according to BT.601*/
	VCENC_RGBTOYUV_BT601_LIMITED_RANGE = 5, /* Color conversion of limited range[0,219] according to BT.601*/
	VCENC_RGBTOYUV_BT709_FULL_RANGE = 6 /* Color conversion of full range[0,255] according to BT.709*/
} VCEncColorConversionType;
/* Picture type for encoding */
typedef enum {
	VCENC_RESERVED = -255,
	VCENC_INTRA_FRAME = 0,
	VCENC_PREDICTED_FRAME = 1,
	VCENC_BIDIR_PREDICTED_FRAME = 2,
	VCENC_NOTCODED_FRAME  /* Used just as a return value */
} VCEncPictureCodingType;


typedef enum {
	VCENC_CHROMA_IDC_400 = 0,
	VCENC_CHROMA_IDC_420 = 1,
	VCENC_CHROMA_IDC_422 = 2,
} VCEncChromaIdcType;

/************** for AV1 ******************/
typedef enum {
	KF_UPDATE = 0,                 // update Key REF
	LF_UPDATE = 1,                 // update LAST/LAST2/LAST3 REF, after encoded , -----> LAST
	GF_UPDATE = 2,                 // update GOLDREF & LAST,       after encoded , -----> LAST/GOLD REF
	ARF_UPDATE = 3,                // update ALTREF,               after encoded , -----> ARF
	ARF2_UPDATE = 4,               // update ALTREF2,              after encoded , -----> ARF2
	BRF_UPDATE = 5,                // update BWD,                  after encoded , -----> BWD
	COMMON_NO_UPDATE = 6,          // common Frame, not update any reference idx
	BWD_TO_LAST_UPDATE = 7,        // Last Frame just before BWD in the poc order, after encoded , BWD------> LAST
	ARF_TO_LAST_UPDATE =  8,       // Last Frame just before ARF in the poc order, after encoded , ARF------> LAST
	ARF2_TO_LAST_UPDATE = 9,       // Last Frame just before ARF2 in the poc order,after encoded , ARF2-----> LAST
	FRAME_UPDATE_TYPES = 10
} VCENC_FrameUpdateType;

/** @} */ /* End of group hal_enc */


#endif  // end of "#define _HAL_VIDEO_COMMON_H_"

