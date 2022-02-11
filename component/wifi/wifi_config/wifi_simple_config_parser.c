
#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B) || defined(CONFIG_PLATFORM_8721D) || defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8710C) || defined(CONFIG_PLATFORM_AMEBAD2) || defined(CONFIG_PLATFORM_AMEBALITE)
#include "basic_types.h"
#endif
#include "wifi_simple_config_parser.h"
#include "wifi_structures.h"
#include <drv_types.h>


#include "rom_md5.h"
#include "rom_sha1.h"
#include "rom_aes.h"

#if defined(CONFIG_PLATFORM_8721D)
#include "ameba_soc.h"
#endif
#if defined(CONFIG_PLATFORM_AMEBAD2)
#include "ameba_soc.h"
#endif
#if defined(CONFIG_PLATFORM_AMEBALITE)
#include "ameba_soc.h"
#endif
#if defined(CONFIG_PLATFORM_8710C)
#include "crypto_api.h"
#endif
#if defined(CONFIG_PLATFORM_8735B)
#include "crypto_api.h"
#endif

#define BSSID_CHECK_SUPPORT 0 //Simple config use Bssid to check packet's da_mac and length

#if (CONFIG_INCLUDE_SIMPLE_CONFIG)

u8 g_bssid[6];
u8 g_security_mode = 0xff;
u8 get_channel_flag = 0;

//#define	DISABLE (0)
#define	INFO 	(1)
#define	DEBUG 	(2)
#define	DETAIL 	(3)
#define BIT_(__n)       (1<<(__n))

#ifndef BIT
#define BIT(__n)       (1<<(__n))
#endif

#ifndef NULL
#define NULL 0
#endif


#undef DBG_871X
#undef printf
#undef memset
#undef memcpy
#undef strlen
#undef strcpy
#undef free
#undef zmalloc
#undef malloc
#undef memcmp
#undef md5_init
#undef md5_append
#undef md5_final
#undef hmac_sha1
#undef _ntohl
#undef AES_UnWRAP

#define memset sc_api_fun.memset_fn
#define memcpy sc_api_fun.memcpy_fn
#define strlen sc_api_fun.strlen_fn
#define strcpy sc_api_fun.strcpy_fn
#define free sc_api_fun.free_fn
#define zmalloc sc_api_fun.zmalloc_fn
#define malloc sc_api_fun.malloc_fn
#define memcmp sc_api_fun.memcmp_fn
#define _ntohl sc_api_fun.ntohl_fn
#define printf sc_api_fun.printf_fn


#if SIMPLE_CONFIG_PLATFORM_LIB
/* use simple config crypto dependency in wifi_simple_config_crypto.c  */
extern void sc_rt_md5_init(md5_ctx *context);
extern void sc_rt_md5_append(md5_ctx *context, u8 *input, u32 inputLen);
extern void sc_rt_md5_final(u8 digest[16], md5_ctx *context);
extern void sc_rt_hmac_sha1(unsigned char *text, int text_len, unsigned char *key,
							int key_len, unsigned char *digest);
extern void sc_AES_UnWRAP(unsigned char *cipher, int cipher_len,
						  unsigned char *kek,	int kek_len,
						  unsigned char *plain);
#define md5_init sc_rt_md5_init
#define md5_append sc_rt_md5_append
#define md5_final sc_rt_md5_final
#define hmac_sha1 sc_rt_hmac_sha1
#define AES_UnWRAP sc_AES_UnWRAP
#else
/* use simple config crypto dependency in rom */
#define md5_init rt_md5_init
#define md5_append rt_md5_append
#define md5_final rt_md5_final
#define hmac_sha1 rt_hmac_sha1
#define AES_UnWRAP AES_UnWRAP
#endif



#define DBG_871X(format, ...) \
	do {	\
		printf(format, ##__VA_ARGS__);	\
		printf("\n");	\
	} while(0)



#define SIMPLE_CONFIG_DEBUG_LEVEL	0 //

/*only for debug */
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
#define get_addr1(ip_addr) ((u16)(_ntohl(ip_addr) >> 24) & 0xff)
#define get_addr2(ip_addr) ((u16)(_ntohl(ip_addr) >> 16) & 0xff)
#define get_addr3(ip_addr) ((u16)(_ntohl(ip_addr) >>  8) & 0xff)
#define get_addr4(ip_addr) ((u16)(_ntohl(ip_addr) >>  0) & 0xff)
#endif

//#define BIT(x)  					(1 << (x))
#define PATTERN_VALID                  BIT(1)
#define PATTERN_USING_UDP_SOCKET       BIT(3)

#define MAX_PATTERN_NUM                (5)
#define MAX_KEY_BUF_LEN                (32)
#define MAX_PROFILE_BUF_LEN            (256)

#define SC_NAME_LEN                    (32)

/* Profile Tag */
#define TAG_RESERVD_SC                 (0)
#define TAG_SSID_SC                    (1)
#define TAG_PASSWORD_SC                (2)
#define TAG_IP_SC                      (3)

/* Pattern 2 related index */
#define P2_SEQ_IDX                     (3)
#define P2_DATA_IDX                    (5)
#define P2_PATTERN_ID_IDX              (5)
#define P2_SYNC_PKT_NUM                (9)

#define P2_MAGIC_IDX0                  (3)
#define P2_MAGIC_IDX1                  (4)
#define P2_MAGIC_IDX2                  (5)
#define P2_MAGIC_IDX3                  (5)
#define P2_MAGIC_IDX4                  (5)

#define RADOM_VALUE_LEN                (4)

// added for softAP mode
void promisc_issue_probersp(unsigned char *da);
#if !defined(CONFIG_PLATFORM_8710C) //to suppress compiling error for GCC
#ifndef CONFIG_INIC_IPC_TODO
extern int rtl_cryptoEngine_init(void);
extern int rtl_crypto_sha1(IN const u8 *message, IN const u32 msglen, OUT u8 *pDigest);
#endif
#endif
extern int rtw_get_random_bytes(void *dst, u32 size);

/* record user define */
static u8 *custom_pin;
static const u8 sc_device_name[] = "simple_config_client";
/* nunce stream (64B) */
static const u8 mcast_udp_buffer[] = "8CmT/ J(3_aE R_UFR}`mtwF=)Qfjtn^S_1/ffg<_C7yw's}?'_'n&2~Blm&_k?6";
/* default pin which use in configuration without pin */
static const u8 default_pin[] = "57289961";
/* randaom value get from encoding packet*/
static u8 radom_value[RADOM_VALUE_LEN] = {0};
/* simple link state machine */
s32 simple_config_status;
static const s32 sc_device_type = 1;
static s32 sync_pkt_index = 0;
static s32 profile_pkt_index = 0;
static u8 fix_sa;
static u32 g_sc_pin_len;
static u8 use_ios7_mac = 0;
static u8 g_ios_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static u8 magic_num[] = {42, 38};//sa+da+ip_type+udp_len+ip_len 6 +6+2+8+20
static struct pattern_ops *pp;
static const u8 default_key_iv[] = {0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6};
static struct rtk_sc *g_sc_ctx = NULL;
struct pattern_ops;
struct rtk_sc;
struct simple_config_lib_config sc_api_fun;

static u8 ubss_channel[2];
static u8 ubss_hidden[1];
static u8 mac_bssid[6];
static int sub_tlv = 0;
extern int ssid_hidden;
static unsigned char dissoc_ssid[33];
static unsigned char dissoc_length = 0;

typedef s32(*sc_check_pattern_call_back)(struct pattern_ops  *pp, struct rtk_sc *pSc);
typedef s32(*sc_get_cipher_info_call_back)(struct pattern_ops  *pp, struct rtk_sc *pSc);
typedef s32(*sc_generate_key_call_back)(struct pattern_ops   *pp, struct rtk_sc *pSc);
typedef s32(*sc_decode_profile_call_back)(struct pattern_ops  *pp, struct rtk_sc *pSc);
typedef s32(*sc_get_tlv_info_call_back)(struct pattern_ops  *pp, struct rtk_sc *pSc);

#pragma pack(1)
struct pattern_ops {
	u32 index;
	u32 flag;					// ??? so how is the flag size ?
	u8 name[SC_NAME_LEN]; 		// 32 B
	sc_check_pattern_call_back check_pattern;
	sc_get_cipher_info_call_back get_cipher_info;
	sc_generate_key_call_back generate_key;
	sc_decode_profile_call_back decode_profile;
	sc_get_tlv_info_call_back get_tlv_info;
};

/**the new structure to adopt bcast and RS code**/
#pragma pack(1)
struct rtk_sc {

	u8      pattern_type;
	u8      smac[6];			//the source mac of the profile packet, it maybe the Phone MAC
	u8      ssid[33];
	u8      password[64];

	u32	    ip_addr;
	u8	    sync_pkt[P2_SYNC_PKT_NUM][6];
	u8	    profile_pkt[MAX_PROFILE_BUF_LEN][6];
	u32	    profile_pkt_len;
	u8 	    plain_buf[MAX_PROFILE_BUF_LEN];
	u32     plain_len;
	u8 	    key_buf[MAX_KEY_BUF_LEN];	//kek
	u32 	key_len;
	u8 	    crypt_buf[MAX_PROFILE_BUF_LEN];
	u32 	crypt_len;
	struct pattern_ops 	*pattern[MAX_PATTERN_NUM];	/*pattern array*/
	u8 	    max_pattern_num; 		/*total register pattern num*/
	u8	    pin[65];
	u8	    default_pin[65];
	u8 	    have_pin;
	u16	    device_type;
	u8	    device_name[64];

	u8	    bcast_crypt_buf[MAX_PROFILE_BUF_LEN];
};

/* no link error if wifi hybrid config do not present */
__weak void whc_fix_channel()
{

}
__weak void whc_unfix_channel()
{

}

void simple_config_lib_init(struct simple_config_lib_config *config)
{
	sc_api_fun = *config;
}

void simple_config_lib_deinit(void)
{
	simple_config_memset_fn local_memset;
	local_memset = memset;
	local_memset(&sc_api_fun, 0, sizeof(sc_api_fun));
}


static s32 bytecopy(u8 *src, u8 *dst, u32 len)
{
	s32 i = 0;
	for (i = 0; i < len; i++) {
		dst[i] = src[i];
	}
	return 0;
}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
static s32 bytedump(u8 *buffer, u32 len)
{
	s32 i = 0;
	for (i = 0; i < len; i++) {
		if ((i % 6 == 0) && (i > 0)) {
			DBG_871X("    ");
		}
		printf("%02x ", buffer[i]);
	}
	DBG_871X("    ");
	return 0;
}
#endif
static s32 rtk_sc_register_pattern(struct pattern_ops *pp)
{
	if (g_sc_ctx->pattern[g_sc_ctx->max_pattern_num] == NULL) {
		g_sc_ctx->pattern[g_sc_ctx->max_pattern_num] = pp;
	}
	g_sc_ctx->max_pattern_num++;
	return 0;
}
static s32 rtk_sc_check_pattern(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	s32 ret = 0;

	if (pp->check_pattern) {
		ret = pp->check_pattern(pp, pSc);
	}
	return ret;
}

static s32 rtk_sc_generate_key(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	s32 ret = 0;

	if (pp->generate_key) {
		ret = pp->generate_key(pp, pSc);
	}
	return ret;
}

static s32 rtk_sc_get_cipher_info(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	s32 ret = 0;

	if (pp->get_cipher_info) {
		ret = pp->get_cipher_info(pp, pSc);
	}
	return ret;
}

static s32 rtk_sc_decode_profile(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	s32 ret = 0;

	if (pp->decode_profile) {
		ret = pp->decode_profile(pp, pSc);
	}
	return ret;
}

static s32 rtk_sc_get_tlv_info(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	s32 ret = 0;

	if (pp->get_tlv_info) {
		ret = pp->get_tlv_info(pp, pSc);
	}
	return ret;
}

static u8 *sub_test(u8 *p_info)
{
	u8 *p_buf;
	p_buf = p_info;
	if (p_buf[0] == 0x2C) {
		//printf("\nInside subtlv\n");
		p_buf++;
		sub_tlv = 1;
		return p_buf;
	} else if ((p_buf[0] == 0x00) && (p_buf[1] == 0x0B)) {
		//printf("\nInside subtlv\n");
		p_buf = p_buf + 2;
		sub_tlv = 1;
		return p_buf;
	} else {
		return p_buf;
	}

}

static s32 sub_test2(struct rtk_sc *test_pSc, u8 test_len)
{
	if (sub_tlv == 1 && test_len == 9) {
		memcpy(mac_bssid, test_pSc->ssid,  6);
		memcpy(ubss_channel, test_pSc->ssid + 6,  2);
		memcpy(ubss_hidden, test_pSc->ssid + 8,  1);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
		DBG_871X("\nchn: %d\n", ubss_channel[1]);
		bytedump(ubss_channel, 2);
		DBG_871X("\nhidden: %d\n", ubss_hidden[0]);
		bytedump(ubss_hidden, 1);
#endif
		return 0;
	} else {
		return -1;
	}

}

static u8 *sub_test3(u8 *p_info, int *p_len)
{
	u8 *p_buf;
	p_buf = p_info;
	if (sub_tlv == 1) {
		p_buf++;
		*p_len = *p_buf;
		return p_buf;
	} else {
		*p_len = (*p_buf) >> 2;
		return p_buf;
	}
}

static s32 parse_tlv_info(struct rtk_sc *pSc, u8 *plain_info, u8 len)
{

	s32 length, current_length;
	u8 *p = plain_info;
	s32 ret = 0;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>parse_tlv_info()");
#endif

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("===>parse_tlv_info()");
	DBG_871X("The plain info is");
	bytedump(plain_info, len);
#endif

	p = sub_test(p);

	current_length = 0;
	while (current_length < len) {
		if (*p == TAG_SSID_SC) {
			// move to length field
			p++;
			// get length value
			length = *p;
			if (length > 32) {
				return -1;
			}
			// move to SSID string start address
			p++;
			// copy ssid string to pSc->ssid
			memset(pSc->ssid, 0, 33);
			bytecopy(p, pSc->ssid, length);

			if (sub_test2(pSc, length) == 0) {
				current_length = 3;
			}

			// move to next item Tag
			p += length;
			// this 2 Bytes means the SSID Tag ID(1B) + Length(1B)
			// get current total length(include current TLV element)
			current_length = 2 + length;
		} else if (*p == TAG_PASSWORD_SC) {
			p++;
			length = *p;
			p++;
			bytecopy(p, pSc->password, length);
			p += length;
			current_length = 2 + length;
		} else if (*p == TAG_IP_SC) {
			p++;
			length = *p;
			p++;
			bytecopy(p, (u8 *) & (pSc->ip_addr), length);
			p += length;
			current_length = 2 + length;
		} else if (*p == TAG_RESERVD_SC) {
			DBG_871X("get the reservd tag");
			break;
		} else {
			DBG_871X("tag(%d) is not supported", *p);
			// skip this tlv instead of stop parsing to support new tag
			// ret = -1;
			p++;
			length = *p;
			p++;
			p += length;
			current_length = 2 + length;
		}

	}

	if (pSc->ip_addr == 0) {
		DBG_871X("the profile must include IP");
		ret = -1;
	}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("the SSID is %s, the PASSWORD is %s, the IP is %x",
			 pSc->ssid, pSc->password, pSc->ip_addr);
	DBG_871X("the IP is %d.%d.%d.%d", get_addr1(pSc->ip_addr),
			 get_addr2(pSc->ip_addr), get_addr3(pSc->ip_addr),
			 get_addr4(pSc->ip_addr));
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===parse_tlv_info()");
#endif
	return ret;
}

static s32 mcast_udp_get_cipher_info(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	/* To avoid gcc warnings */
	(void) pp;

	s32 i = 0;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>mcast_udp_get_cipher_info()");
#endif
	// copy the encoding data in data packet to pSc->crypt_buf
	for (i = 0; i < pSc->profile_pkt_len; i++) {
		pSc->crypt_buf[i] = pSc->profile_pkt[i][P2_DATA_IDX];
	}
	pSc->crypt_len = pSc->profile_pkt_len;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("The crypt info is");
	bytedump(pSc->crypt_buf, pSc->crypt_len);
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===mcast_udp_get_cipher_info()");
#endif
	return 0;
}

static s32 mcast_udp_generate_key(struct pattern_ops  *pp, struct rtk_sc *pSc)
{

	u8 buffer[256];
	u8 md5_digest[16];
	s32 len = 0;
	md5_ctx md5;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>mcast_udp_generate_key()");
#endif
	memset(buffer, 0x0, 256);

	/* Get thired party(i.e mobile phone or tablet) SA */
	//DBG_871X("Get thired party(i.e mobile phone or tablet) SA");
	if (use_ios7_mac) {
		memcpy(buffer, g_ios_mac, 6);
	} else {
		memcpy(buffer, pSc->smac, 6);
	}
	len += 6;

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("Add SA MAC, buffer len = %d, expect = 6", len);
	DBG_871X("sa:%02x:%02x:%02x:%02x:%02x:%02x",
			 buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
	DBG_871X("Use User pin = %d", pSc->have_pin);
#endif
	/* PIN */
	if (pSc->have_pin) {
		memcpy(buffer + len, pSc->pin, strlen((const s8 *)pSc->pin));
	} else {
		// default pin = "57289961" , guess QR code get is the same
		memcpy(buffer + len, pSc->default_pin, g_sc_pin_len);
	}
	//14(6+ 8)	// 8
	len += g_sc_pin_len;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("Add PIN, buffer len = %d, expect = 14", len);
#endif
	/* Nonce Stream (64) */				//	64
	memcpy(buffer + len, mcast_udp_buffer, strlen((const s8 *)mcast_udp_buffer));
	// 14 + 64 = 78
	len += strlen((const s8 *)mcast_udp_buffer);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("Add nunce stream, buffer len = %d, expect = 78", len);
#endif

	/* radom value (4) */
	//DBG_871X("radom value = 0x%x:0x%x:0x%x:0x%x",radom_value[0],radom_value[1],radom_value[2],radom_value[3]);
	memcpy(buffer + len, radom_value, strlen((const s8 *)radom_value));
	// 78 + 4 = 82
	len += 4;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("Add random value, buffer len = %d, expect = 82", len);
	DBG_871X("the buf for MD5 is:");
	bytedump(buffer, len);
#endif
	/* generate Key of hmac_sha1 */
	//DBG_871X("generate Key of hmac_sha1");
	md5_init(&md5);
	md5_append(&md5, buffer, len);
	md5_final(md5_digest, &md5);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("after MD5, the buffer is ");
	bytedump(buffer, len);
#endif
	/* pattern name */
	// "sc_mcast_udp"			12
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("pattern name = %s", pp->name);
#endif
	memcpy(buffer + len, pp->name, strlen((const s8 *)pp->name));
	len += strlen((const s8 *)pp->name);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("Add pattern %s, buffer len = %d, expect = 94", pp->name, len);
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("before SHA1, the buffer is:");
	bytedump(buffer, len);
#endif
	/* generate key of AES. digest is 20 bytes */
	//DBG_871X("generate key of AES. digest is 20 bytes");
	hmac_sha1(buffer, len, md5_digest, 16, pSc->key_buf);
	pSc->key_len = 16;

	/* only 128 bits is used for AES */
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("only 128 bits is used for AES");
	DBG_871X("key_buf[32] content =");
	DBG_871X("0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x", pSc->key_buf[0], pSc->key_buf[1], pSc->key_buf[2], pSc->key_buf[3], pSc->key_buf[4], pSc->key_buf[5],
			 pSc->key_buf[6], pSc->key_buf[7]);
	DBG_871X("0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x", pSc->key_buf[8], pSc->key_buf[9], pSc->key_buf[10], pSc->key_buf[11], pSc->key_buf[12], pSc->key_buf[13],
			 pSc->key_buf[14], pSc->key_buf[15]);
	DBG_871X("0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x", pSc->key_buf[16], pSc->key_buf[17], pSc->key_buf[18], pSc->key_buf[19], pSc->key_buf[20], pSc->key_buf[21],
			 pSc->key_buf[22], pSc->key_buf[23]);
	DBG_871X("0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x", pSc->key_buf[24], pSc->key_buf[25], pSc->key_buf[26], pSc->key_buf[27], pSc->key_buf[28], pSc->key_buf[29],
			 pSc->key_buf[30], pSc->key_buf[31]);
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("After SHA1, the buffer is:");
	bytedump(buffer, len);
#endif

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===mcast_udp_generate_key()");
#endif
	return 0;
}


static s32 mcast_udp_get_pattern(struct pattern_ops *pp, struct rtk_sc *pSc)
{
	/* To avoid gcc warnings */
	(void) pp;

	s32 pattern_index, random_num_index;
	u8 magic_num;
	// get pattern_type
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>mcast_udp_get_pattern()");
#endif
	pattern_index = (pSc->sync_pkt[2][P2_PATTERN_ID_IDX]) +
					((pSc->sync_pkt[1][P2_PATTERN_ID_IDX]) << 8) +
					((pSc->sync_pkt[0][P2_PATTERN_ID_IDX]) << 16);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("the pattern type is %d ", pattern_index);
#endif
	if (pattern_index == 2) {
		if (custom_pin != NULL) {
			simple_config_status = -1;
			DBG_871X("This APP must enable pin!\n");
			return -1;
		}
		// get magic_num
		magic_num = (pSc->sync_pkt[0][P2_MAGIC_IDX0]) + (pSc->sync_pkt[1][P2_MAGIC_IDX1]) + (pSc->sync_pkt[2][P2_MAGIC_IDX2]) + (pSc->sync_pkt[3][P2_MAGIC_IDX3]);
		// check magic num
		if (magic_num == pSc->sync_pkt[4][P2_MAGIC_IDX4]) {
			pSc->pattern_type = 2;
			// get the total data packet counts which after sync packets
			pSc->profile_pkt_len = pSc->sync_pkt[3][P2_DATA_IDX] - P2_SYNC_PKT_NUM;
			// use default pin
			memcpy(pSc->default_pin, default_pin, strlen((const s8 *)default_pin));
			g_sc_pin_len = strlen((const s8 *)default_pin);
			pSc->default_pin[g_sc_pin_len] = '\0';
		}
	} else if (pattern_index == 3) {
		// get magic_num
		magic_num = (pSc->sync_pkt[0][P2_MAGIC_IDX0]) + (pSc->sync_pkt[1][P2_MAGIC_IDX1]) + (pSc->sync_pkt[2][P2_MAGIC_IDX2]) + (pSc->sync_pkt[3][P2_MAGIC_IDX3]);
		// check magic num
		if (magic_num == pSc->sync_pkt[4][P2_MAGIC_IDX4]) {
			pSc->pattern_type = 3;
			pSc->profile_pkt_len = pSc->sync_pkt[3][P2_DATA_IDX] - P2_SYNC_PKT_NUM;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
			DBG_871X("profile_pkt_len = 0x%x", pSc->profile_pkt_len);
#endif
			/* TODO: for now, we not support the user enter pin,
						so use the default_pin replace it */
			if (custom_pin) {
				g_sc_pin_len = strlen((const char *)custom_pin);
				if (g_sc_pin_len) {
					memcpy(pSc->pin, custom_pin, strlen((const char *)custom_pin));
				}
			} else {
				simple_config_status = -1;
				DBG_871X("This client do not have pin!\n");
				return -1;
			}
			// use user define pin
			pSc->have_pin = 1;
			pSc->pin[g_sc_pin_len] = '\0';
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
			DBG_871X("The pin code is %s, the pin length is %d",
					 pSc->pin, g_sc_pin_len);
#endif
		}
	} else {
		DBG_871X("It is not multicast UDP pattern");
		return -1;
	}
	// get random num[4] from B55 B65 B75 B85 byte copy
	if (P2_SYNC_PKT_NUM == 9) {
		for (random_num_index = 5;
			 random_num_index < P2_SYNC_PKT_NUM; random_num_index++) {
			radom_value[random_num_index - 5] =
				pSc->sync_pkt[random_num_index][P2_PATTERN_ID_IDX];
		}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
		DBG_871X("The radom value is ");
		bytedump(radom_value, 4);
#endif
	}
	/* TODO: To add the device_type, although it not important to FreeRTOS */
	pSc->device_type = sc_device_type;
	strcpy((s8 *)pSc->device_name, (s8 *)sc_device_name);
	DBG_871X("The device_name = %s", pSc->device_name);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===mcast_udp_get_pattern()");
#endif
	return 0;
}

static s32 bcast_udp_get_pattern(struct pattern_ops   *pp, struct rtk_sc *pSc)
{
	/* To avoid gcc warnings */
	(void) pp;

	int sc_pin_enabled;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>bcast_udp_get_pattern()");
#endif
	pSc->pattern_type = 5;
	if (custom_pin) {
		sc_pin_enabled = 1;
		g_sc_pin_len = strlen((const s8 *)custom_pin);
	} else {
		sc_pin_enabled = 0;
		g_sc_pin_len = 0;
	}

	pSc->have_pin = sc_pin_enabled;

	if (sc_pin_enabled == 0) {
		memcpy(pSc->pin, default_pin, strlen((const s8 *)default_pin));
		memcpy(pSc->default_pin, default_pin, strlen((const s8 *)default_pin));
		g_sc_pin_len = strlen((const s8 *)default_pin);
		pSc->default_pin[g_sc_pin_len] = '\0';
	} else {
		memcpy(pSc->pin, custom_pin, g_sc_pin_len);
	}

	pSc->pin[g_sc_pin_len] = '\0';

	radom_value[0] = 50;
	radom_value[1] = 51;
	radom_value[2] = 52;
	radom_value[3] = 53;

	pSc->device_type = sc_device_type;
	strcpy((s8 *)pSc->device_name, (s8 *)sc_device_name);

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===bcast_udp_get_pattern()");
#endif
	return 0;
}

static s32 bcast_udp_get_cipher_info(struct pattern_ops   *pp, struct rtk_sc *pSc)
{
	/* To avoid gcc warnings */
	(void) pp;

	int i = 0;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>bcast_udp_get_cipher_info()");
#endif
	memset(pSc->crypt_buf, 0, pSc->profile_pkt_len);
	for (i = 0; i < pSc->profile_pkt_len; i++) {
		if ((i & 0x01) == 1) {
			pSc->crypt_buf[i >> 1] |= (pSc->bcast_crypt_buf[i] & 0xf);
		} else {
			pSc->crypt_buf[i >> 1] |= (pSc->bcast_crypt_buf[i] << 4 & 0xf0);
		}
	}
	pSc->crypt_len = pSc->profile_pkt_len >> 1;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("The crypt length is %d, profile pkt len is %d", pSc->crypt_len, pSc->profile_pkt_len);
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===bcast_udp_get_cipher_info()");
#endif
	return 0;
}

/**only for bcast use**/
int parse_tlv_info_bcast(struct rtk_sc *pSc, unsigned char *plain_info, unsigned char len)
{
	int length, current_length;
	unsigned char *p = plain_info;
	int rcv_ssid = 0, rcv_pwd = 0, rcv_ip = 0;

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("===>parse_tlv_info_bcast()");
	DBG_871X("The plain info is");
	bytedump(plain_info, len);
#endif

	p = sub_test(p);

	current_length = 0;

	while (current_length < len) {
		if ((*p & 0x03) == TAG_SSID_SC) {
			if (rcv_ssid == 0) {
				rcv_ssid = 1;
			} else {
				return -1;
			}

			//length = (*p)>>2;
			p = sub_test3(p, &length);

			if (length > 32) {
				return -1;
			}
			p++;
			memset(pSc->ssid, 0, 33);
			bytecopy(p, pSc->ssid, length);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
			bytedump(pSc->ssid, length);
			DBG_871X("\nlength: %d\n", length);
#endif

			if (sub_test2(pSc, length) == 0) {
				current_length = 3;
			}

			p += length;
			current_length += 1 + length;
		} else if ((*p & 0x03) == TAG_PASSWORD_SC) {
			if (rcv_pwd == 0) {
				rcv_pwd = 1;
			} else {
				return -1;
			}

			length = (*p) >> 2;
			if (length == 0) {
				length = 64;
			}
			p++;
			memset(pSc->password, 0, 64);
			bytecopy(p, pSc->password, length);
			p += length;
			current_length += 1 + length;
		} else if ((*p & 0x03) == TAG_IP_SC) {
			if (rcv_ip == 0) {
				rcv_ip = 1;
			} else {
				return -1;
			}

			length = (*p) >> 2;
			if (length != 4) {
				return -1;
			}
			p++;
			bytecopy(p, (unsigned char *) & (pSc->ip_addr), length);
			p += length;
			current_length += 1 + length;
		} else if (*p == TAG_RESERVD_SC) {
			break;
		} else {
			return -1;
		}

		if (current_length > len) {
			return -1;
		}
	}

	while (current_length < len) {
		if (plain_info[current_length++] != 0x00) {
			return -1;
		}
	}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("The SSID is %s, the PASSWORD is %s",
			 pSc->ssid, pSc->password);
	DBG_871X("The IP is %d.%d.%d.%d", get_addr1(pSc->ip_addr),
			 get_addr2(pSc->ip_addr), get_addr3(pSc->ip_addr),
			 get_addr4(pSc->ip_addr));
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===parse_tlv_info_bcast()");
#endif

	if (pSc->ip_addr == 0) {
		return -1;
	} else {
		return 0;
	}
}


static s32 mcast_udp_get_profile(struct pattern_ops  *pp, struct rtk_sc *pSc)
{
	u8 *plain_info = pSc->plain_buf;
	s32 len = pSc->plain_len - 8;
	s32 ret = 0;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>mcast_udp_get_profile()");
#endif

	// Check profile content
	if (memcmp(pp->name, "sc_mcast_udp", 12) == 0) {
		if (memcmp(pSc->plain_buf, default_key_iv, 8)) {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
			DBG_871X("key iv is 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x ", pSc->plain_buf[0], pSc->plain_buf[1], pSc->plain_buf[2], pSc->plain_buf[3],
					 pSc->plain_buf[4], pSc->plain_buf[5], pSc->plain_buf[6], pSc->plain_buf[7]);
#endif
			return -1;
		}

		ret = parse_tlv_info(pSc, (plain_info + 8), len);
	} else { //For bcast
		len = pSc->plain_len;
		ret = parse_tlv_info_bcast(pSc, plain_info, len);
		if (ret < 0) {
			printf("get tlv error\n");
		}
	}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===mcast_udp_get_profile()");
#endif
	return ret;
}
/*
extern void AES_UnWRAP(u8 *cipher, s32 cipher_len,
		u8 *kek, s32 kek_len, u8 *plain);
*/
static s32 mcast_udp_decode_profile(struct pattern_ops   *pp, struct rtk_sc *pSc)
{
	/* To avoid gcc warnings */
	(void) pp;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>mcast_udp_decode_profile()");
#endif

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("before AES:");
	bytedump(pSc->crypt_buf, pSc->crypt_len);
	DBG_871X("the key is:");
	bytedump(pSc->key_buf, pSc->key_len);
#endif
	// decode AES encry_text to plain_text and put the content in pSc->plain_buf
	AES_UnWRAP(pSc->crypt_buf, pSc->crypt_len, pSc->key_buf, pSc->key_len, pSc->plain_buf);
	pSc->plain_len = pSc->crypt_len;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
	DBG_871X("the plain buf is:");
	bytedump(pSc->plain_buf, pSc->plain_len);
#endif
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===mcast_udp_decode_profile()");
#endif
	return 0;
}

struct pattern_ops udp_mcast = {
	2,
	(PATTERN_USING_UDP_SOCKET | PATTERN_VALID),
	"sc_mcast_udp",
	mcast_udp_get_pattern,		// 1
	mcast_udp_get_cipher_info,	// 2 or 3
	mcast_udp_generate_key,		// 3 or 2
	mcast_udp_decode_profile,	// 4
	mcast_udp_get_profile,		// 5
	//mcast_udp_send_ack,		// 6
};

struct pattern_ops udp_mcast_pin = {
	3,
	(PATTERN_USING_UDP_SOCKET | PATTERN_VALID),
	"sc_mcast_udp",
	mcast_udp_get_pattern,
	mcast_udp_get_cipher_info,
	mcast_udp_generate_key,
	mcast_udp_decode_profile,
	mcast_udp_get_profile,
	//mcast_udp_send_ack,
};

/***add broadcast pattern***/
struct pattern_ops udp_bcast = {
	4,
	(PATTERN_USING_UDP_SOCKET | PATTERN_VALID),
	"sc_bcast_udp",
	bcast_udp_get_pattern,
	bcast_udp_get_cipher_info,
	mcast_udp_generate_key,
	mcast_udp_decode_profile,
	mcast_udp_get_profile,
};

struct pattern_ops udp_bcast_pin = {
	5,
	(PATTERN_USING_UDP_SOCKET | PATTERN_VALID),
	"sc_bcast_udp",
	bcast_udp_get_pattern,
	bcast_udp_get_cipher_info,
	mcast_udp_generate_key,
	mcast_udp_decode_profile,
	mcast_udp_get_profile,
};

u8 *rtk_sc_get_ie(u8 *pbuf, sint index, u32 *len, sint limit)
{
	sint tmp, i;
	u8 *p;
	_func_enter_;
	if (limit < 1) {
		_func_exit_;
		return NULL;
	}

	p = pbuf;
	i = 0;
	*len = 0;
	while (1) {
		if (*p == index) {
			*len = *(p + 1);
			return (p);
		} else {
			tmp = *(p + 1);
			p += (tmp + 2);
			i += (tmp + 2);
		}
		if (i >= limit) {
			break;
		}
	}
	return NULL;
}

static s32 rtk_sc_get_packet_length(u8  index)
{
#if BSSID_CHECK_SUPPORT
	u8 length = (index * 16807) % 32;
	if (length == 0) {
		length = 7;
	}
	return	length;
#else
	return 	index;
#endif
}

static s32 rtk_sc_check_packet(u8 *da, u8 *bssid, s32 length)
{
	/* To avoid gcc warnings */
	(void) bssid;

	s32 i;
	u8 check_sum = 0;
	for (i = 0; i < 6; i ++) {
#if BSSID_CHECK_SUPPORT
		check_sum += da[i] + bssid[i];
#else
		check_sum += da[i];
#endif
	}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>rtk_sc_check_packet()");
	DBG_871X("Checksum is %d", check_sum);
#endif
	//Add for aes/tkip mixed encrypt and dlink855 does not forward multicast packet
#if ((SIMPLE_CONFIG_DEBUG_LEVEL <= DEBUG))
	DBG_871X("Input frame da: %02X %02X %02X %02X %02X %02X, data length: %02X",
			 da[0], da[1], da[2], da[3], da[4], da[5], (length - 42));

	//DBG_871X("Input frame length: %d\r\n", length);
#endif
	if ((check_sum == 0) && ((length == rtk_sc_get_packet_length(da[P2_SEQ_IDX]) + magic_num[0]) ||
							 (length == rtk_sc_get_packet_length(da[P2_SEQ_IDX]) + magic_num[0] - 4))) {
		return 1;
	} else {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
		DBG_871X("PACKET length %d or checksum %d is invalid", length, check_sum);
#endif
		return 0;
	}
}

static s32 rtk_clean_profile_value(void)
{

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>rtk_clean_profile_value()");
#endif

	fix_sa = 0;
	sync_pkt_index = 0;
	profile_pkt_index = 0;

	memset((void *)g_sc_ctx, 0, sizeof(struct rtk_sc));
	rtk_sc_register_pattern(&udp_mcast);
	rtk_sc_register_pattern(&udp_mcast_pin);
	rtk_sc_register_pattern(&udp_bcast);
	rtk_sc_register_pattern(&udp_bcast_pin);

	return 0;
}

void rtk_restart_simple_config(void)
{
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>rtk_restart_simple_config()");
#endif
	rtk_clean_profile_value();
	memset(g_bssid, 0, 6);
	get_channel_flag = 0;
	simple_config_status = 1;
	*(sc_api_fun.is_promisc_callback_unlock) = 1;
	whc_unfix_channel();
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===rtk_restart_simple_config()");
#endif
}

void rtk_stop_simple_config(void)
{
	rtk_clean_profile_value();
	simple_config_status = 0;
}

static void rtk_start_simple_config(void)
{
	rtk_clean_profile_value();
	//if (simple_config_status == 0)
	memset(g_bssid, 0, 6);
	get_channel_flag = 0;
	simple_config_status = 1;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("Start simple config now!");
#endif
}


s32 rtk_sc_init(char *custom_pin_code, struct simple_config_lib_config *lib_config)
{
	simple_config_lib_init(lib_config);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("===>rtk_sc_init()");
#endif
	/* init and clear */
	if (g_sc_ctx != NULL) {
		free((void *)g_sc_ctx, 0);
		g_sc_ctx = NULL;
		DBG_871X("free custom g_sc_ctx");
	}

	g_sc_ctx = (struct rtk_sc *)zmalloc(sizeof(struct rtk_sc));
	if (!g_sc_ctx) {
		DBG_871X("rtk_sc_parser_init() fail!");
		return -1;
	}

	if (custom_pin != NULL) {
		free(custom_pin, 0);
		custom_pin = NULL;
		DBG_871X("free custom pin code");
	}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
	DBG_871X("Register Realtek Simple Config");
#endif
	if (custom_pin_code != NULL) {

		if (strlen(custom_pin_code) < 8) {
			s32 pad_start;
			custom_pin = (u8 *)malloc(9);
			memcpy(custom_pin, custom_pin_code, strlen(custom_pin_code));
			for (pad_start = strlen(custom_pin_code); pad_start < 8; pad_start++) {
				custom_pin[pad_start] = 0x30;
			}
			custom_pin[8] = '\0';
			DBG_871X("strlen(custom_pin_code) = %d < 8, padding to 8", strlen(custom_pin_code));
		} else if (8 <= strlen(custom_pin_code) && strlen(custom_pin_code) < 65) {
			custom_pin = (u8 *)malloc(strlen(custom_pin_code) + 1);
			memcpy(custom_pin, custom_pin_code, strlen(custom_pin_code));
			custom_pin[strlen(custom_pin_code)] = '\0';
			DBG_871X("strlen(custom_pin_code) = %d", strlen(custom_pin_code));
		} else {
			custom_pin = (u8 *)malloc(65);
			memcpy(custom_pin, custom_pin_code, 65);
			custom_pin[64] = '\0';
			DBG_871X("strlen(custom_pin_code) = %d > 32,only use the first 32-digit numeric as input", strlen(custom_pin_code));
		}
	}
#ifndef CONFIG_INIC_IPC_TODO
	if (rtl_cryptoEngine_init() != 0) {
		DBG_871X("crypto engine init failed\r\n");
		return -1;
	}
#endif
	rtk_start_simple_config();

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===rtk_sc_init()");
#endif
	return 0;
}

void rtk_sc_deinit()
{
	if (custom_pin != NULL) {
		free(custom_pin, 0);
		custom_pin = NULL;
	}
	if (g_sc_ctx != NULL) {
		free((void *)g_sc_ctx, 0);
		g_sc_ctx = NULL;
	}

	simple_config_lib_deinit();
}
/*for bcast*/
int rtk_sc_check_profile(struct pattern_ops *pp, struct rtk_sc *pSc, void *backup_sc_ctx)
{
	int ret = 0;
	struct rtk_test_sc *user_sc = backup_sc_ctx;

	use_ios7_mac = 0;
	rtk_sc_get_cipher_info(pp, pSc);

	ret = rtk_sc_generate_key(pp, pSc);
	if (ret == 0) {
		ret = rtk_sc_decode_profile(pp, pSc);
		if (ret == 0) {
			ret = rtk_sc_get_tlv_info(pp, pSc);
		}
	}
	if (ret != 0) {
		use_ios7_mac = 1;
		ret = rtk_sc_generate_key(pp, pSc);
		if (ret == 0) {
			ret = rtk_sc_decode_profile(pp, pSc);
			if (ret == 0) {
				ret = rtk_sc_get_tlv_info(pp, pSc);
			}
		}
		use_ios7_mac = 0;
		if (ret != 0) {
			if (pp->index == 5) {
				return -1;
			}
			rtk_restart_simple_config();
			return -1;
		}
	}
	/***already decoded, assign AP profile***/
	memset(user_sc->ssid, 0, 33);
	memcpy(user_sc->ssid, pSc->ssid, sizeof(pSc->ssid));
	memcpy(user_sc->password, pSc->password, sizeof(pSc->password));
	user_sc->ip_addr = pSc->ip_addr;
	return 0;
}

// use nonceA to generate nonceB
static void softAP_get_nonceB(SC_softAP_decode_ctx *pSoftAP_ctx)
{
// assue cryptoEngine is already inited
// sha1 input len: 6(bssid) + 16(nonceA) + 27("Realtek Simple Config v3 m2") = 49
	unsigned char *sha1_input, *sha1_output;
	unsigned char randomB[16];

	sha1_input = (unsigned char *)malloc(49);
	sha1_output = (unsigned char *)malloc(20);
	memcpy(sha1_input, pSoftAP_ctx -> mac, 6);
	memcpy(sha1_input + 6, pSoftAP_ctx -> nonceA, 16);
	memcpy(sha1_input + 22, "Realtek Simple Config v3 m2", 27);

	// nonceB: 16byte(output of sha1) + 16 (randomB)
#ifdef CONFIG_INIC_IPC_TODO
	mbedtls_sha1(sha1_input, 49, sha1_output);
#else
	if (SUCCESS != rtl_crypto_sha1(sha1_input, 49, sha1_output)) {
		printf("sha1 error in generate nonceB");
	}
#endif

	memcpy(pSoftAP_ctx -> nonceB, sha1_output, 16);

	rtw_get_random_bytes(randomB, 16);
	memcpy(pSoftAP_ctx -> nonceB + 16, randomB, 16);

	free((void *)sha1_input, 0);
	free((void *)sha1_output, 0);
	return;
}

// check the validity of recv pkt
static void softAP_check_pkt(unsigned char *buf, SC_softAP_decode_ctx *pSoftAP_ctx, u8 *ret)
{
	unsigned char *sha1_input, *sha1_output;

	sha1_input = (unsigned char *)malloc(49);
	sha1_output = (unsigned char *)malloc(20);

	// sha1 input: 6byte(bssid) + 16byte(randomB) + 27byte("Realtek Simple Config v3 m3")
	memcpy(sha1_input, pSoftAP_ctx -> mac, 6);
	memcpy(sha1_input + 6, pSoftAP_ctx -> nonceB + 16, 16);
	memcpy(sha1_input + 22, "Realtek Simple Config v3 m3", 27);

#ifdef CONFIG_INIC_IPC_TODO
	mbedtls_sha1(sha1_input, 49, sha1_output);
#else
	if (SUCCESS != rtl_crypto_sha1(sha1_input, 49, sha1_output)) {
		printf("sha1 error in generate nonceB");
	}
#endif

	if (memcmp(sha1_output, buf, 16)) {
		printf("check sha1 error in recv pkt\n");
		*ret = 0; //check FAIL
	} else {
		*ret = 1;    //check PASS
	}

	free((void *)sha1_input, 0);
	free((void *)sha1_output, 0);

	return;
}
// return val: 1 succeed, -1 fail, 0 continue.
SC_softAP_status softAP_simpleConfig_parse(unsigned char *buf, int len, void *backup_sc_ctx, void *psoftAP_ctx)
{
	int profile_len = 0;
	unsigned char controller_ip[4];
	struct rtk_test_sc *user_sc = NULL;
	SC_softAP_decode_ctx *pSoftAP_ctx = (SC_softAP_decode_ctx *)psoftAP_ctx;

	if (len == 5 && memcmp(buf, "ERROR", 5) == 0) {
		printf("softAP received error pkt from Phone\n");
		goto softAP_error;
	}

	if (len < 16) {
		printf("softAP received pkt len is too small\n");
		goto softAP_error;
	}

	// 4-way handshake state machine of softAP mode
	switch (pSoftAP_ctx -> softAP_decode_status) {
	case SOFTAP_INIT:
		memcpy(pSoftAP_ctx -> nonceA, buf, 16);
		pSoftAP_ctx -> softAP_decode_status = SOFTAP_RECV_A;
		softAP_get_nonceB(pSoftAP_ctx);
		return SOFTAP_RECV_A;
	case SOFTAP_RECV_A: {
		u8 ret = 0;
		softAP_check_pkt(buf, pSoftAP_ctx, &ret);
		if (ret == 0) {
			printf("[softAP_step2]: pkt is invalid\n");
			goto softAP_error;
		} else {
			pSoftAP_ctx -> softAP_decode_status = SOFTAP_HANDSHAKE_DONE;
			return SOFTAP_HANDSHAKE_DONE;
		}
	}
	case SOFTAP_HANDSHAKE_DONE:
		break;
	default:
		printf("unknown status, return");
		goto softAP_error;
	}

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	printf("\n\nThe softAP buf value\n\n");
	bytedump(buf, len);
#endif

	if (buf[0] == 0x08 && buf[1] == 0x25 && buf[2] == 0x07 && buf[3] == 0x01) {
		profile_len = buf[4];
		memcpy(g_sc_ctx->smac, buf + 6, 6);
		memcpy(g_sc_ctx->bcast_crypt_buf, buf + 16, profile_len);
		g_sc_ctx->profile_pkt_len = profile_len;

		memcpy(controller_ip, buf + 12, 4);

		pp = g_sc_ctx->pattern[2];
		rtk_sc_check_pattern(pp, g_sc_ctx);
		if (pp && (pp->flag & PATTERN_VALID)) {
			if (rtk_sc_check_profile(pp, g_sc_ctx, backup_sc_ctx) == -1) {
				printf("check profile error\n");
				goto softAP_error;
			}

			// use the prefix ip_addr when in softAP mode
			user_sc = (struct rtk_test_sc *)backup_sc_ctx;
			user_sc->ip_addr = buf[12] | buf[13] << 8 | buf[14] << 16 | buf[15] << 24;
			rtk_stop_simple_config();
			*(sc_api_fun.is_promisc_callback_unlock) = 0;
			return SOFTAP_DECODE_SUCCESS;
		} else {
			goto softAP_error;
		}
	} else {
		printf("header check fail, header: 0x%02x 0x%02x 0x%02x 0x%02x\n",
			   buf[0], buf[1], buf[2], buf[3]);

		printf("profile length: %d\n", buf[4]);
		printf("configure device number: %d\n", buf[5]);
		printf("source mac: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			   buf[6], buf[7], buf[8],
			   buf[9], buf[10], buf[11]);
		printf("controller IP: %d.%d.%d.%d\n", buf[12], buf[13], buf[14], buf[15]);

		goto softAP_error;
	}

softAP_error: {
		memset(pSoftAP_ctx -> nonceA, 0, sizeof(pSoftAP_ctx -> nonceA));
		memset(pSoftAP_ctx -> nonceB, 0, sizeof(pSoftAP_ctx -> nonceB));
		pSoftAP_ctx -> softAP_decode_status = SOFTAP_INIT;
		return SOFTAP_ERROR;
	}

}

int get_sc_profile_fmt(void)
{
	return sub_tlv;
}

int get_sc_profile_info(void *fmt_info_t)
{
	struct fmt_info *user_info = fmt_info_t;
	rtw_memcpy(user_info->fmt_channel, ubss_channel, 2);
	rtw_memcpy(user_info->fmt_hidden, ubss_hidden, 1);
	rtw_memcpy(user_info->fmt_bssid, mac_bssid,  6);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("\nchn: %d\n", user_info->fmt_channel[1]);
	DBG_871X("\nHidden: %d\n", user_info->fmt_hidden[0]);
	bytedump(user_info->fmt_bssid, 6);
#endif
}

int get_sc_dsoc_info(void *dsoc_info_t)
{
	struct dsoc_info *dsoc_prof_info = dsoc_info_t;
	dsoc_prof_info->dsoc_length =  dissoc_length;
	rtw_memcpy(dsoc_prof_info->dsoc_ssid, dissoc_ssid, dsoc_prof_info->dsoc_length);
}

int rtl_dsoc_parse(u8 *mac_addr, u8 *buf, void *userdata, unsigned int *len)
{

	ieee80211_frame_info_t *promisc_info = (ieee80211_frame_info_t *)userdata;
	int ret = -1;
	int ssid_ie_len = 0;
	unsigned char *p;
	switch (*(unsigned short *)buf & 0xfc) {
	case /*WIFI_ASSOCREQ*/0x00: {
		if (memcmp(promisc_info->bssid, mac_addr, 6) == 0) {
			p = rtk_sc_get_ie((buf + WLAN_HDR_A3_LEN + _ASOCREQ_IE_OFFSET_), _SSID_IE_, &ssid_ie_len, (*len - WLAN_HDR_A3_LEN - _ASOCREQ_IE_OFFSET_));
			if (p && ssid_ie_len > 0) {
				memset(dissoc_ssid, 0, sizeof(dissoc_ssid));
				rtw_memcpy(dissoc_ssid, (p + 2), ssid_ie_len);
				dissoc_length = ssid_ie_len;
				if (dissoc_length != 0) {
					ret = 0;
				}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
				DBG_871X("\nassoc request\n");
				DBG_871X("\nlength: %d\n", dissoc_length);
				bytedump(dissoc_ssid, dissoc_length);
#endif
			}
		}
		break;
	}
	case /*WIFI_PROBERES*/0x50: {
		if (memcmp(promisc_info->bssid, mac_addr, 6) == 0) {
			p = rtk_sc_get_ie((buf + WLAN_HDR_A3_LEN + _PROBERSP_IE_OFFSET_), _SSID_IE_, &ssid_ie_len, (*len - WLAN_HDR_A3_LEN - _PROBERSP_IE_OFFSET_));
			if (p && ssid_ie_len > 0) {
				memset(dissoc_ssid, 0, sizeof(dissoc_ssid));
				rtw_memcpy(dissoc_ssid, (p + 2), ssid_ie_len);
				dissoc_length = ssid_ie_len;
				if (dissoc_length != 0) {
					ret = 0;
				}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
				DBG_871X("\nprobe response\n");
				DBG_871X("\nlength: %d\n", dissoc_length);
				bytedump(dissoc_ssid, dissoc_length);
#endif
			}
		}
		break;
	}
	case /*BEACON*/0x80: {
		if ((memcmp(promisc_info->bssid, mac_addr, 6) == 0) && (ssid_hidden == 0)) {
			p = rtk_sc_get_ie((buf + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_), _SSID_IE_, &ssid_ie_len, (*len - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_));
			if (p && ssid_ie_len > 0) {
				memset(dissoc_ssid, 0, sizeof(dissoc_ssid));
				rtw_memcpy(dissoc_ssid, (p + 2), ssid_ie_len);
				dissoc_length = ssid_ie_len;
				if (dissoc_length != 0) {
					ret = 0;
				}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
				DBG_871X("\nbeacon\n");
				DBG_871X("\nlength: %d\n", dissoc_length);
				bytedump(dissoc_ssid, dissoc_length);
#endif
			}
		}
		break;
	}
	default:
		break;
	}
	return ret;
}

int rtl_pre_parse(u8 *mac_addr, u8 *buf, void *userdata, u8 **da, u8 **sa, unsigned int *len)
{
	ieee80211_frame_info_t *promisc_info = (ieee80211_frame_info_t *)userdata;
	unsigned to_ds = 0, fr_ds = 0;
	to_ds = ((*(unsigned short *)buf & (u16)(1 << 8)) << 1) ? 1 : 0;
	fr_ds = (*(unsigned short *)buf & (u16)(1 << 9)) ? 1 : 0;
	unsigned char to_fr_ds = (to_ds << 1) | fr_ds;
	int ret = -1;

	switch (to_fr_ds) {
	case 0x0:
		*da = promisc_info->i_addr1;
		*sa = promisc_info->i_addr2;
		break;
	case 0x01:	// ToDs=0, FromDs=1
		*da = promisc_info->i_addr1;
		*sa = promisc_info->i_addr3;
		break;
	case 0x02:	// ToDs=1, FromDs=0
		*da = promisc_info->i_addr3;
		*sa = promisc_info->i_addr2;
		break;
	default:	// ToDs=1, FromDs=1
		return -1;
	}

	switch (*(unsigned short *)buf & 0xfc) {
	case /*WIFI_AUTH*/0xb0:
		if (memcmp(*da, mac_addr, 6) == 0) {
			ret = 1;
		}
		return ret;
	case /*WIFI_PROBEREQ*/0x40: {
		promisc_issue_probersp(*sa);
		return ret;
	}
	default:
		break;
	}

	if (promisc_info->encrypt == 0xff) {
		return ret;
	}
	/*wlan header to ether header, 26-6-2-8-8+6+6+2 = 26-10*/
	if (promisc_info->i_fc & (1 << 7)) { //QoS packet
		*len -= 20;    //(-26 - 8 + 6 + 6 + 2);  //MAC header length + SNAP size
	} else {
		*len -= 18;    //(-24 - 8 +6 + 6 + 2);
	}

	switch (promisc_info->encrypt) {
	case RTW_ENCRYPTION_OPEN:
		break;
	case RTW_ENCRYPTION_WEP40:
	case RTW_ENCRYPTION_WEP104:
		*len -= 8;
		break;
	case RTW_ENCRYPTION_WPA_TKIP:
	case RTW_ENCRYPTION_WPA2_TKIP:
		*len -= 12 + 8; // 12: iv = 8,icv = 4; 8: mic
		break;
	case RTW_ENCRYPTION_WPA_AES:
	case RTW_ENCRYPTION_WPA2_AES:
	case RTW_ENCRYPTION_WPA2_MIXED:
		*len -= 16;
		break;
	default:	//default AES
		*len -= 16;
		break;
	}

	ret = 0;
	return ret;
}

s32 rtk_start_parse_packet(u8 *da, u8 *sa, s32 len, void *user_data, void *backup_sc_ctx)
{
	int j = 0;
	s32 pattern_cunt = 0;
	s32 ret = -1;
	s32 pkt_seq_num = 0;
	ieee80211_frame_info_t *promisc_info;

	// only parse multicast mode pkt
	if (!(da[0] == 0x01 && da[1] == 0x00 && da[2] == 0x5e)) {
		return 0;
	}

	promisc_info = user_data;

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("==>rtk_start_parse_packet()");
	DBG_871X("Simple_config_status = %d", simple_config_status);
#endif

	if (simple_config_status == 1 || simple_config_status == 2) {
		{
			if (!rtk_sc_check_packet(da, promisc_info->bssid, len)) {
				return 0;
			}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
			DBG_871X("fix_sa: %d", fix_sa);
			DBG_871X("Previous MAC: %02X %02X %02X %02X %02X %02X",
					 g_sc_ctx->smac[0], g_sc_ctx->smac[1], g_sc_ctx->smac[2], g_sc_ctx->smac[3], g_sc_ctx->smac[4], g_sc_ctx->smac[5]);
			DBG_871X("Input frame da: %02X %02X %02X %02X %02X %02X",
					 da[0], da[1], da[2], da[3], da[4], da[5]);

			DBG_871X("Input frame sa: %02X %02X %02X %02X %02X %02X",
					 sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#endif
			//Make sure the input frame is from the same MAC with the previous
			if ((fix_sa == 1) && (memcmp(g_sc_ctx->smac, sa, 6) == 0)) {
				// get each frame sequence num (0 ~ 8)
				pkt_seq_num = (da[P2_SEQ_IDX]);
				if (g_sc_ctx->sync_pkt[pkt_seq_num][0] != 0x00) {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
					DBG_871X("g_sc_ctx->sync_pkt[%d][0] != 0x00", pkt_seq_num);
#endif
					return 0;
				} else if (pkt_seq_num > (P2_SYNC_PKT_NUM - 1)) { //It's a profile packet
					if (g_sc_ctx->profile_pkt[pkt_seq_num - P2_SYNC_PKT_NUM][0] != 0x00) {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
						DBG_871X("Already got this sync packet");
#endif
						return 0;
					} else {
						for (j = 0; j < 6; j++) {
							g_sc_ctx->profile_pkt[pkt_seq_num - P2_SYNC_PKT_NUM][j] = da[j];
						}
						profile_pkt_index++;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
						DBG_871X("get profile info from packets");
						DBG_871X("data_pkt_seq_num is %d, profile_pkt_index is %d",
								 pkt_seq_num, profile_pkt_index);
#endif
					}
				} else {
					for (j = 0; j < 6; j++) {
						g_sc_ctx->sync_pkt[pkt_seq_num][j] = (da)[j];
					}
					sync_pkt_index |= 1 << pkt_seq_num;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
					DBG_871X("Current stored sync packet is");
					bytedump(&(g_sc_ctx->sync_pkt[0][0]), 6 * P2_SYNC_PKT_NUM);
					DBG_871X("pkt_seq_num is %d, sync_pkt_index is 0x%03x, (sync_pkt_index|0x1ff) is 0x%d", pkt_seq_num, sync_pkt_index, (sync_pkt_index | 0x1ff));
#endif
				}
				// guess syn packet count(1 bit/packet)
				if ((sync_pkt_index & 0x1ff) == 0x1ff) {
					//		6 * P2_SYNC_PKT_NUM);
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
					DBG_871X("the stored sync packet is");
					bytedump(&(g_sc_ctx->sync_pkt[0][0]),
							 6 * P2_SYNC_PKT_NUM);
					DBG_871X("g_sc_ctx->max_pattern_num = %d", g_sc_ctx->max_pattern_num);
#endif

					for (pattern_cunt = 0; pattern_cunt < g_sc_ctx->max_pattern_num; pattern_cunt++) {
						pp = g_sc_ctx->pattern[pattern_cunt];
						if (pp && (pp->flag & PATTERN_VALID)) {
							ret = rtk_sc_check_pattern(pp, g_sc_ctx);
							if (ret == 0) {
								/*****change the stauts to let the state machine go to decode***/
								simple_config_status = 3;
								sync_pkt_index = 0;
								return 0;
							} else if (ret == -1) {
								return ret;
							}
						}
					}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
					DBG_871X("Sync packet don't match any patterns");
					DBG_871X("Restart simple config");
#endif
					fix_sa = 0;
					rtk_restart_simple_config();
				} //end of sync_pkt_index
			} else if (fix_sa == 0) {
				memcpy(g_sc_ctx->smac, sa, 6);
				memcpy(g_bssid, promisc_info->bssid, 6);
				get_channel_flag = 1;
				g_security_mode = promisc_info->encrypt;
				simple_config_status = 2;
				sync_pkt_index = 0;
				fix_sa = 1;
				whc_fix_channel();
			} /*****when fix_sa is 0, initialize****/

		} /**end of multicast mode*/
	}

	else if (simple_config_status == 3) {
		if (!rtk_sc_check_packet(da, promisc_info->bssid, len) || !(memcmp(g_sc_ctx->smac, sa, 6) == 0)) {
			return 0;
		}

		pkt_seq_num = da[3];

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
		DBG_871X("pkt_seq_num is %d", pkt_seq_num);
#endif
		if (pkt_seq_num < P2_SYNC_PKT_NUM) {//Already get all sync pkt
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
			DBG_871X("Already got this sync packet");
#endif
			return 0;
		}
		if (!(profile_pkt_index == g_sc_ctx->profile_pkt_len)) {
			if (g_sc_ctx->profile_pkt[pkt_seq_num - P2_SYNC_PKT_NUM][0] != 0x00) {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
				DBG_871X("Already got this profile packet");
#endif
				return 0;
			} else {
				for (j = 0; j < 6; j++) {
					g_sc_ctx->profile_pkt[pkt_seq_num - P2_SYNC_PKT_NUM][j] = da[j];
				}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
				DBG_871X("Stored the profile %d", pkt_seq_num);
#endif
				profile_pkt_index++;
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DEBUG))
				DBG_871X("get profile info from packets");
				DBG_871X("data_pkt_seq_num is %d, profile_pkt_index is %d",
						 pkt_seq_num, profile_pkt_index);
#endif
			}
		}
		if (profile_pkt_index == g_sc_ctx->profile_pkt_len) {
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
			DBG_871X("collect %d encoding data packets",
					 profile_pkt_index);
#endif
			use_ios7_mac = 0;
			rtk_sc_get_cipher_info(pp, g_sc_ctx);
			ret = rtk_sc_generate_key(pp, g_sc_ctx);
			if (ret == 0) {
				ret = rtk_sc_decode_profile(pp, g_sc_ctx);
				if (ret == 0) {
					ret = rtk_sc_get_tlv_info(pp, g_sc_ctx);
				}
			}
			/* start iOS decode profile */
			if (ret != 0) {
				use_ios7_mac = 1;
				ret = rtk_sc_generate_key(pp, g_sc_ctx);
				if (ret == 0) {
					ret = rtk_sc_decode_profile(pp, g_sc_ctx);
					if (ret == 0) {
						ret = rtk_sc_get_tlv_info(pp, g_sc_ctx);
					}
				}
				use_ios7_mac = 0;
			}
			/* */
			if (ret == 0) {
				/* return the result to user */
				struct rtk_test_sc *user_sc = backup_sc_ctx;
				memcpy(user_sc->ssid, g_sc_ctx->ssid, sizeof(g_sc_ctx->ssid));
				memcpy(user_sc->password, g_sc_ctx->password, sizeof(g_sc_ctx->password));
				user_sc->ip_addr = g_sc_ctx->ip_addr;
			}
			profile_pkt_index = 0;
			rtk_stop_simple_config();
			if (ret == -1) {
				DBG_871X("Config App wrong!!Restart config correctly");
				printf("%s", "\r\nConfig App wrong!!Restart config correctly,simple_config_status = -1");
				return -1;
			}
			*(sc_api_fun.is_promisc_callback_unlock) = 0;

#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= INFO))
			DBG_871X("Get the profile info now");
#endif
			return 1;
		}
	} else {
		DBG_871X("Simple_config_status = %d", simple_config_status);
		rtk_restart_simple_config();
	}
#if ((SIMPLE_CONFIG_DEBUG_LEVEL >= DETAIL))
	DBG_871X("<===rtk_start_parse_packet()");
#endif
	return 0;
}
#endif

#ifdef __cplusplus
}
#endif
