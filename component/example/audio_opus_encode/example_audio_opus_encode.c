#include "platform_opts.h"

#include <platform_stdlib.h>
#include "opusenc.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "ff.h"
#include "sdio_combine.h"
#include "sdio_host.h"
#include "fatfs_sdcard_api.h"
#define READ_SIZE 256

char logical_drv[4];

FATFS   m_fs;
FIL     m_file;
fatfs_sd_params_t fatfs_sd;

void opus_audio_opus_encode(u8 *source_file, u8 *dest_file, opus_int32 sample_rate, int channels, int complexity, int bitrate)
{

	printf("enter opus encode\n\r");

	char abs_path[32];
	OggOpusEnc *enc;
	OggOpusComments *comments;
	int error;
	FRESULT res;

	snprintf(abs_path, sizeof(abs_path), "%s%s", fatfs_sd.drv, source_file);

	//Open source file
	res = f_open(&m_file, abs_path, FA_OPEN_EXISTING | FA_READ); // open read only file
	comments = ope_comments_create();

	//add artist name and title name to the converted opus file
	ope_comments_add(comments, "ARTIST", "Someone");
	ope_comments_add(comments, "TITLE", "Some track");
	//specify the sampling rate and channels of source file
	enc = ope_encoder_create_file(dest_file, comments, sample_rate, channels, 0, &error);
	//configure the complexity and bitrate of encoded file
	ope_encoder_ctl(enc, OPUS_SET_COMPLEXITY_REQUEST, complexity);
	ope_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, bitrate);
	u32 tim_total1 = 0;
	//change data transfer strategy according to channel number
	if (channels == 2) {
		while (1) {
			short buf[2 * READ_SIZE];
			int byte_count;
			u32 tim1, tim2;
			FRESULT read_res;
			read_res = f_read(&m_file, (void *)buf, 2 * sizeof(short) * READ_SIZE, &byte_count);
			if (byte_count < 2 * sizeof(short)*READ_SIZE) {
				break;
			}
			if (read_res == FR_OK) {
				tim1 = xTaskGetTickCount();
				ope_encoder_write(enc, buf, byte_count / (2 * sizeof(short)));
				tim2 = xTaskGetTickCount();
				tim_total1 += (tim2 - tim1) * 1;
			} else {
				break;
			}
		}
	}

	else {
		while (1) {
			short buf[READ_SIZE];
			int byte_count;
			u32 tim1, tim2;
			FRESULT read_res;
			read_res = f_read(&m_file, (void *)buf, sizeof(short) * READ_SIZE, &byte_count);
			if (byte_count < sizeof(short)*READ_SIZE) {
				break;
			}
			if (read_res == FR_OK) {
				tim1 = xTaskGetTickCount();
				ope_encoder_write(enc, buf, byte_count / (sizeof(short)));
				tim2 = xTaskGetTickCount();
				tim_total1 += (tim2 - tim1) * 1;
			} else {
				break;
			}
		}
	}

	printf("conversion finished\n\r");
	printf("tim_total is %d\n\r", tim_total1);
	ope_encoder_drain(enc);
	ope_encoder_destroy(enc);
	ope_comments_destroy(comments);
	f_close(&m_file);

}

void opus_audio_opus_encode_thread(void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_PLATFORM_8735B)
	rtw_create_secure_context(configMINIMAL_SECURE_STACK_SIZE);
#endif
	//source_file is stored in SD card, dest_file is the one converted from source_file
	u8 *source_file[] = {"1000Hz_48k_stereo.wav", "1000Hz_48k_mono.wav", "1000Hz_16k_mono.wav"};
	u8 *dest_file[] = {"48_stereo_256_10.opus", "48_stereo_256_3.opus", "48_stereo_256_0.opus", "48_mono_256_3.opus", "16_mono_48_3.opus", "16_mono_20_3.opus"};
	printf("\r\n============start opus_audio_opus_encode==================\r\n");
	FRESULT res;
	res = fatfs_sd_init();
	if (res < 0) {
		printf("fatfs_sd_init fail (%d)\n\r", res);
		goto fail;
	}
	fatfs_sd_get_param(&fatfs_sd);
	opus_audio_opus_encode(source_file[0], dest_file[0], 48000, 2, 10, 256000);
	opus_audio_opus_encode(source_file[0], dest_file[1], 48000, 2, 3, 256000);
	opus_audio_opus_encode(source_file[0], dest_file[2], 48000, 2, 0, 256000);
	opus_audio_opus_encode(source_file[1], dest_file[3], 48000, 1, 3, 256000);
	opus_audio_opus_encode(source_file[2], dest_file[4], 16000, 1, 3, 48000);
	opus_audio_opus_encode(source_file[2], dest_file[5], 16000, 1, 3, 20000);
fail:
	fatfs_sd_close();
	vTaskDelete(NULL);
}

void example_audio_opus_encode(void)
{
	xTaskCreate(opus_audio_opus_encode_thread, ((const char *)"opus_audio_opus_encode_thread"), 20000, NULL, tskIDLE_PRIORITY + 1, NULL);
	//printf("\n\r%s xTaskCreate(opus_audio_opus_encode_thread) failed", __FUNCTION__);
}
