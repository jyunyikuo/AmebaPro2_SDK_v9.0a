/*
 * Get firmware from flash
 * only support read operation
 */
#include <stdio.h>
#include <stdint.h>
#include "snand_api.h"
#include "device_lock.h"
#include "log_service.h"

// firmware prepend manifest before raw binary and align to 4K
typedef struct manifest_s {
	uint8_t  lbl[8];
	uint16_t size;
	uint16_t vrf_alg;
	uint32_t enc_rmp_base_addr;
	uint8_t  resv2[16];
	uint8_t  sec_enc_record[8][32];
	// -- TLV format start from here
	uint8_t  tlv_start[0];
	uint8_t  pk[388];
	uint8_t  type_id[8];
	uint8_t  version[36];
	uint8_t  timestamp[12];
	uint8_t  imgsz[8];
	uint8_t  encalg[8];
	uint8_t  enckn[36];
	uint8_t  encks[36];
	uint8_t  enciv[20];
	uint8_t  hshalg[8];
	uint8_t  hshkn[36];
	uint8_t  hshks[36];
	uint8_t  ie_resv[36];
	uint8_t  hash[36];
	uint8_t  tlv_end[0];
	// -- TLV format end here
	uint8_t  signature[384];
} manifest_t;

enum tlv_id {
	ID_PK = 1,
	ID_VERSION,
	ID_IMGSZ,
	ID_TYPE_ID,
	ID_ENCALG,
	ID_ENCKN,
	ID_ENCKS,
	ID_ENCIV,
	ID_HSHALG,
	ID_HSHKN,
	ID_HSHKS,
	ID_HASH,
	ID_TIMST,
	ID_VID,
	ID_PID,
	ID_IMGLST,
	ID_DEP,
	ID_RMATK,
	ID_BATLV,
	ID_ACPW,
	ID_IE_RESV
};

static char *manifest_valid_label = "RTL8735B";
#define FWFS_MANIFEST_SIZE	4096

typedef struct tlv_s {
	uint32_t id_size;
	uint8_t data[384];	// because max data is 384 bytes, valid data length depend on id_size
} tlv_t;

typedef struct img_hdr_s {
	uint32_t imglen;
	uint32_t nxtoffset;
	uint16_t type_id;
	uint16_t nxt_type_id;
	uint8_t  sec_enc_ctrl;
	uint8_t  resv234[3 + 4 + 4];
	uint32_t str_tbl;
	uint8_t  resv5678[4 + 32 + 32 + 32];
} img_hdr_t;

int tlv_get_value(uint8_t *tlv, uint8_t *tlv_end, int id, uint8_t *value)
{
	uint8_t *curr = tlv;

	int found = -1;
	while (curr < tlv_end) {
		tlv_t *ie = (tlv_t *)curr;
		int cid = ie->id_size & 0xff;
		int csize = (ie->id_size >> 8) & 0xffffff;
		//printf("TLV search cid %d csize %d, tid %d tsize %d", cid, csize, id, len);
		if (cid == id) {
			found = 0;
			memcpy(value, ie->data, csize);
			break;
		} else {
			curr = curr + ((csize + 3) & (~3)) + 4;    // tlv header size (4) + data size (align to 4)
		}
	}

	if (found == -1) {
		printf("TLV not found");
	}
	return found;
}


typedef struct nand_fci_s {
	uint32_t blk_cnt;
	uint32_t page_per_blk;
	uint32_t page_size;
	uint32_t spare_size;
	uint8_t  resv1[0x10];
	uint32_t part_tbl_start;
	uint32_t part_tbl_dup_cnt;
	uint32_t vrf_alg;
	uint8_t  resv2[0x2];
	uint32_t bbt_start;
	uint32_t bbt_dup_cnt;
} nand_fci_t;

typedef struct nand_part_rec_s {
	uint32_t magic_num;
	uint16_t type_id;
	uint16_t blk_cnt;
	uint8_t  resv[0x18];
	uint16_t vmap[48];
} nand_part_rec_t;

struct nand_spare_v0_s {
	uint32_t magic_num;
	uint16_t type_id;
} __attribute__((packed));

struct nand_spare_v1_s {
	uint8_t  type_id;
	uint16_t crc;
} __attribute__((packed));

struct nand_spare_s {
	uint8_t  bad_blk_tag;
	union {
		struct nand_spare_v0_s v0;
		struct nand_spare_v1_s v1;
	};
};

typedef struct nand_spare_s nand_spare_t;

typedef struct partition_s {
	nand_fci_t fci;

	nand_part_rec_t *part_rec;
} partition_t;

static partition_t g_partition;
static int g_pfw_inited = 0;
static snand_t flash;

// default page size and page per block
static snand_blksize = 2048 * 64;
static snand_pgsize = 2048;
static snand_ppb = 64;
static snand_spare_version = 1;

void pfw_dump_mem(uint8_t *buf, int size)
{
	int size16 = size & (~15);
	int size_r = size - size16;
	uint8_t *base = buf;
	printf("Address  :  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n\r");
	for (int i = 0; i < size16; i += 16) {
		printf("%08x :", i);
		for (int x = 0; x < 16; x++) {
			printf(" %2x", buf[i + x]);
		}
		printf("\n\r");
	}

	if (size_r) {
		printf("%08x :", size16);
		for (int x = 0; x < size_r; x++) {
			printf(" %2x", buf[size16 + x]);
		}
		printf("\n\r");
	}
}

#define PARTAB_TYPE_ID (((snand_spare_version)==0)?0xD9C4:0xC4)

int nand_pfw_get_typeid(nand_spare_t *spare)
{
	if (snand_spare_version == 0) {
		return (int)spare->v0.type_id;
	} else {
		return (int)spare->v1.type_id;
	}
}

int nand_pfw_get_spare_version(nand_spare_t *spare)
{
	if (spare->v0.magic_num == 0xff35ff87) {
		snand_spare_version = 0;
	} else {
		snand_spare_version = 1;
	}
}

void nand_pfw_init(void)
{
	if (g_pfw_inited == 1)	{
		return;
	}

	uint8_t *tmp = (uint8_t *)malloc(snand_pgsize * 4);

	if (!tmp) {
		printf("out of resource\n\r");
		return;
	}

	snand_init(&flash);


	nand_fci_t *fci  = NULL;
	//read partition_table block16-23
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	// read first page of nand control block
	for (int i = 0; i < 8; i++) {
		snand_page_read(&flash, i * snand_ppb, snand_pgsize + 32, tmp);
		//pfw_dump_mem(tmp, 2048+32);
		fci = (nand_fci_t *)tmp;
		nand_spare_t *spare = (nand_spare_t *)&tmp[snand_pgsize];
		//pfw_dump_mem(&tmp[snand_pgsize], 32);
		//printf("bad blk tag %x\n\r", spare->bad_blk_tag);
		if (spare->bad_blk_tag == 0xff) {
			nand_pfw_get_spare_version(spare);
			//pfw_dump_mem(spare, 32);
			break;
		} else {
			fci = NULL;
		}
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	if (fci == NULL) {
		printf("No Flash control infomation\n\r");
		goto pfw_init_fail;
	}

	memcpy(&g_partition.fci, fci, sizeof(nand_fci_t));
	//pfw_dump_mem(&g_partition.fci, sizeof(nand_fci_t));
	printf("fci part tbl start   %x\n\r", fci->part_tbl_start);
	printf("fci part tbl dup cnt %x\n\r", fci->part_tbl_dup_cnt);

	snand_blksize = fci->page_per_blk * fci->page_size;
	snand_pgsize = fci->page_size;
	snand_ppb    = fci->page_per_blk;

	printf("update page size %d  page per block %d\n\r", snand_pgsize, snand_ppb);
	// read partition table

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	for (int i = fci->part_tbl_start; i < fci->part_tbl_start + fci->part_tbl_dup_cnt; i++) {
		snand_page_read(&flash, i * snand_ppb, snand_pgsize + 32, tmp);
		nand_spare_t *spare = (nand_spare_t *)&tmp[snand_pgsize];
		//pfw_dump_mem(tmp, 2048);
		//pfw_dump_mem(&tmp[2048], 32);
		//printf("bad blk tag %x\n\r", spare->bad_blk_tag);
		//printf("type id     %x\n\r", spare->type_id);
		if (spare->bad_blk_tag == 0xff && nand_pfw_get_typeid(spare) == PARTAB_TYPE_ID) {	// partition table id = 0xC4 or 0xD9C4
			int ri = snand_pgsize;
			int si = 1;
			while (ri < snand_pgsize * 4) {
				snand_page_read(&flash, i * snand_ppb + si, snand_pgsize, &tmp[ri]);
				ri += snand_pgsize;
				si++;
			}
			break;
		}
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	//pfw_dump_mem(&tmp[2048], 2048);

	g_partition.part_rec = (nand_part_rec_t *)tmp;
	g_pfw_inited = 1;
	return;
pfw_init_fail:
	if (tmp) {
		free(tmp);
	}
	return;
}

void nand_pfw_deinit(void)
{
	if (g_pfw_inited && g_partition.part_rec) {
		free(g_partition.part_rec);
	}

	g_pfw_inited = 0;
}

/*
static int __part_rec_check_valid(void *rec)
{
	uint32_t tmp = 0xffffffff;
	uint32_t *rec32 = (uint32_t *)rec;
	for (int i = 0; i < 128 / 4; i++) {
		tmp &= rec32[i];
	}

	if (tmp == 0xffffffff)	{
		return 1;
	} else {
		return 0;
	}
}
*/

typedef struct part_id_map_s {
	uint16_t type_id;
	char *name;
} part_id_map_t;

static part_id_map_t id_map[] = {
	{0xF8E0, "INI_VAL"},
	{0xF1C1, "KEY_CER_TBL"},
	{0xE9C2, "KEY_CER1"},
	{0xE1C3, "KEY_CER2"},
	{0xD9C4, "PT"},
	{0xD1C5, "BL_PRI"},
	{0xC9C6, "BL_SEC"},
	{0xC1C7, "FW1"},
	{0xB9C8, "FW2"},
	{0xB1C9, "MP"},
	{0xA9CA, "SYSDATA"},
	{0xA1CB, "WLAN_CAL"},
	{0x99CC, "USER"},
	{0x91CD, "IMG_UPDATE"},
	{0x89CE, "ISP_IQ"},
	{0x81CF, "NN_MDL"},
	{0x79D0, "NAND_CTRL"}
};

char *__get_pt_name(uint16_t type_id)
{
	for (int i = 1; i < sizeof(id_map) / sizeof(id_map[0]); i++) {
		if (id_map[i].type_id == type_id) {
			return id_map[i].name;
		}
	}
	return id_map[0].name;
}

uint16_t __get_pt_type_id(char *name)
{
	for (int i = 1; i < sizeof(id_map) / sizeof(id_map[0]); i++) {
		if (strcmp(id_map[i].name, name) == 0) {
			return id_map[i].type_id;
		}
	}
	return 0xffff;
}

nand_part_rec_t *pfw_search_next(nand_part_rec_t *curr)
{
	nand_part_rec_t *next = NULL;
	while (curr->magic_num != 0xffffffff) {
		int offset = 1;
		next = &curr[offset];
		if (next->magic_num == 0xff35ff87) {
			return next;
		}
		curr = next;
	}

	return NULL;
}

nand_part_rec_t *pfw_search_next_type_id(nand_part_rec_t *curr, uint16_t type_id)
{
	nand_part_rec_t *next = NULL;
	while (curr->magic_num != 0xffffffff) {
		int offset = 1;
		next = &curr[offset];
		if (next->magic_num == 0xff35ff87) {
			if (next->type_id == type_id) {
				return next;
			}
		}
		curr = next;
	}

	return NULL;
}

nand_part_rec_t *pfw_search_type_id(uint16_t type_id)
{
	nand_part_rec_t *rec = g_partition.part_rec;
	if (g_pfw_inited == 0) {
		return;
	}

	int i = 0;
	while (rec[i].magic_num != 0xffffffff) {
		if (rec[i].magic_num == 0xff35ff87) {

			if (rec[i].type_id == type_id) {
				return &rec[i];
			}
			i++;
		} else {
			i++;
		}
	}
	return NULL;
}

void pfw_list2(void)
{
	nand_part_rec_t *rec = g_partition.part_rec;

	if (g_pfw_inited == 0) {
		return;
	}
	// dump partition items
	int i = 0;
	printf("%4s%8s%8s\t%s\n\r", "rec", "type_id", "blk_cnt", "name");
	while (rec[i].magic_num != 0xffffffff) {
		if (rec[i].magic_num == 0xff35ff87) {
			printf("%4d%8x%8d\t%s\n\r", i, rec[i].type_id, rec[i].blk_cnt, __get_pt_name(rec[i].type_id));

			if (rec[i].blk_cnt > 48) {
				i = i + ((rec[i].blk_cnt - 48) + 63) / 64;
			}
			i++;
		} else {
			i++;
		}
	}
}

void nand_pfw_list(void)
{
	nand_part_rec_t *rec = g_partition.part_rec;
	nand_part_rec_t *base = g_partition.part_rec;
	nand_part_rec_t *next = NULL;

	if (g_pfw_inited == 0) {
		return;
	}
	// dump partition items
	printf("%4s%8s%8s\t%s\n\r", "rec", "type_id", "blk_cnt", "name");

	while (rec != NULL) {
		next = NULL;
		uint16_t type_id = rec->type_id;
		uint16_t blk_cnt = rec->blk_cnt;

		if (rec->blk_cnt == 48) {
			do {
				nand_part_rec_t *next = pfw_search_next_type_id(rec, type_id);
				blk_cnt += next->blk_cnt;
				if (next) {
					rec = next;
				}
			} while (next != NULL && (next->blk_cnt == 48));
		}

		printf("%4d%8x%8d\t%s\n\r", (rec - base) / sizeof(nand_part_rec_t), type_id, blk_cnt, __get_pt_name(type_id));
		rec = pfw_search_next(rec);
	}
}

typedef struct fw_rec_s {
	uint8_t tmp_page[4096 + 32];
	int tmp_page_index;
	int tmp_page_valid;
	nand_part_rec_t *part_recs[32];
	int part_recs_cnt;
	nand_part_rec_t *part_rec;

	int curr_pos;
	uint16_t type_id;

	int manifest_valid;
	int content_len;
	int raw_offset;
} fw_rec_t;

/* open partiion talbe item */
int nand_pfw_read(void *fr, void *data, int size);

void *nand_pfw_open_by_typeid(uint16_t type_id, int mode)
{
	(void)mode;
	// init firmware systemp
	nand_pfw_init();

	// open partition items by type
	fw_rec_t *fr = (fw_rec_t *)malloc(sizeof(fw_rec_t));
	if (!fr)	{
		return NULL;
	}

	memset(fr, 0, sizeof(fw_rec_t));

	fr->type_id = type_id;

	nand_part_rec_t *tmp_rec = pfw_search_type_id(type_id);
	do {
		fr->part_recs[fr->part_recs_cnt] = tmp_rec;
		fr->part_recs_cnt ++;
		tmp_rec = pfw_search_next_type_id(tmp_rec, type_id);
	} while (tmp_rec != NULL);
	fr->part_rec = fr->part_recs[0];

	printf("open: part_rec %x, part_recs_cnt %d, type_id %x\n\r", fr->part_rec, fr->part_recs_cnt, type_id);

	// read 4k
	if (!fr->part_rec || (fr->part_recs_cnt == 0)) {
		free(fr);
		return NULL;
	}

	uint8_t *tmp = malloc(4096 + sizeof(img_hdr_t));
	if (!tmp) {
		free(fr);
		return NULL;
	}

	nand_pfw_read(fr, tmp, 4096 + sizeof(img_hdr_t));

	manifest_t *mani = (manifest_t *)tmp;
	if (memcmp(mani->lbl, manifest_valid_label, 8) == 0) {
		img_hdr_t *imghdr = &tmp[4096];
		fr->manifest_valid = 1;
		//fr->content_len = tlv_get_value(mani->tlv_start, mani->tlv_end, ID_IMGSZ, &fr->content_len);
		fr->content_len = imghdr->imglen;
		fr->raw_offset = 4096 + sizeof(img_hdr_t);
	} else {
		fr->manifest_valid = 0;
		//fr->content_len = fr->part_rec->blk_cnt * snand_blksize;
		fr->content_len = ((fr->part_recs_cnt - 1) * 48 + fr->part_recs[fr->part_recs_cnt - 1]->blk_cnt) * snand_blksize;
		fr->raw_offset = 0;
	}

	// clean tmp page and reset position, curr_pos increased by nand_pfw_read and need reset to 0
	fr->tmp_page_valid = 0;
	fr->tmp_page_index = 0;
	fr->curr_pos = 0;

	free(tmp);

	return fr;
}

typedef struct fwfs_file_s {
	char filename[32 + 8];
	int filelen;
	int offset;
} fwfs_file_t;

typedef struct fwfs_folder_s {
	char tag[12];
	int file_cnt;
	fwfs_file_t files[32];
} fwfs_folder_t;

void *nand_pfw_open(char *name, int mode)
{
	char name_dup[strlen(name) + 2];
	strcpy(name_dup, name);
	char *file_name = name_dup;

	char *type_name = strsep(&file_name, "/");

	printf("type_name %s, file_name %s\n\r", type_name, file_name);

	uint16_t type_id = __get_pt_type_id(type_name);
	if (type_id == 0xffff)	{
		return NULL;
	}

	//printf("open fw partition %s type id %x\n\r", name, type_id);

	fw_rec_t *fr = nand_pfw_open_by_typeid(type_id, mode);

	if (file_name != NULL) {
		// search filename and update file size and raw offset
		fwfs_folder_t *tmp = malloc(sizeof(fwfs_folder_t));
		if (!tmp) {
			free(fr);
			return NULL;
		}

		nand_pfw_read(fr, tmp, sizeof(fwfs_folder_t));
		nand_pfw_seek(fr, 0, SEEK_SET);
		if (strcmp(tmp->tag, "FWFSDIR") == 0) {
			for (int i = 0; i < tmp->file_cnt; i++) {
				if (strcmp(file_name, tmp->files[i].filename) == 0) {
					printf("file %s, len %d\n\r", tmp->files[i].filename, tmp->files[i].filelen);
					fr->content_len = tmp->files[i].filelen;
					fr->raw_offset += tmp->files[i].offset;
					break;
				}
			}
		}

	}
	return fr;
}

int nand_pfw_tell(void *fr)
{
	if (!fr)	{
		return -1;
	}
	fw_rec_t *r = (fw_rec_t *)fr;

	return r->curr_pos;
}

void nand_pfw_close(void *fr)
{
	// close opend partition item
	if (fr)	{
		free(fr);
	}
}

int nand_pfw_read(void *fr, void *data, int size)
{
	uint8_t *data8 = (uint8_t *)data;

	if (!fr)	{
		return -1;
	}

	fw_rec_t *r = (fw_rec_t *)fr;
	// read data from parition item


	int curr_pos = r->curr_pos + r->raw_offset;
	int blksize = snand_blksize;
	int pgsize = snand_pgsize;
	int ppb = snand_ppb;

	//printf("read fw size %d @ %x+%x\n\r", size, r->curr_pos), r->raw_offset;
	//printf("fwrd: >>> curr pos %x\n\r", curr_pos );
	// convert pos to block/page/byte
	int blkk_idx = curr_pos / blksize;
	int blkk_res = curr_pos - blkk_idx * blksize;
	int page_idx = blkk_res / pgsize;
	int byte_idx = blkk_res - page_idx * pgsize;

	int rest_size = r->content_len - r->curr_pos;

	if (rest_size <= 0) {
		return EOF;
	}
	if (size > rest_size) {
		size = rest_size;
	}

	//printf("fwrd: blk %x page %x byte %x rec %x\n\r", blkk_idx, page_idx, byte_idx, r);
	//
	if (r->tmp_page_valid && r->tmp_page_index == page_idx) {
		int op_size = 0;
		int valid_size = 2048 - byte_idx;

		//printf("fwrd: read from tmp buffer, valid size %x, needed size %x\n\r", valid_size, size);
		if (valid_size >= size) {
			op_size = size;
		} else {
			op_size = valid_size;
		}

		valid_size -= op_size;
		if (valid_size == 0) {
			r->tmp_page_valid = 0;
		}

		//printf("fwrd: read from tmp buffer, op_size %x dst %x src %x\n\r", op_size, data8, &r->tmp_page[byte_idx]);
		memcpy(data8, &r->tmp_page[byte_idx], op_size);

		r->curr_pos += op_size;

		size -= op_size;
		data8 += op_size;
	}

	//printf("fwrd: rest size to read %x\n\r", size);

	while (size > 0) {
		int op_size = 0;

		curr_pos = r->curr_pos + r->raw_offset;
		blkk_idx = curr_pos / blksize;
		blkk_res = curr_pos - blkk_idx * blksize;
		page_idx = blkk_res / pgsize;
		byte_idx = blkk_res - page_idx * pgsize;

		int real_blk_idx = 0;

		r->part_rec = r->part_recs[blkk_idx / 48];

		real_blk_idx = r->part_rec->vmap[blkk_idx % 48];
		//printf("fwrd: logical blk %x physical blk %x\n\r", blkk_idx, real_blk_idx);
		//printf("fwrd: blk %x phyb %x page %x byte %x rec %x\n\r", blkk_idx, real_blk_idx, page_idx, byte_idx, r);

		op_size = size;

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		snand_page_read(&flash, real_blk_idx * ppb + page_idx, 2048, r->tmp_page);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if (size >= 2048) {
			op_size = 2048;
			//snand_page_read(&flash, real_blk_idx * ppb + page_idx, 2048, data8);
		} else {
			//snand_page_read(&flash, real_blk_idx * ppb + page_idx, 2048, r->tmp_page);
			//memcpy(data8, r->tmp_page, size);
			r->tmp_page_valid = 1;
			r->tmp_page_index = page_idx;

			op_size = size;
		}
		memcpy(data8, &r->tmp_page[byte_idx], op_size);

		r->curr_pos += op_size;

		size -= op_size;
		data8 += op_size;
	}

	return (int)data8 - (int)data;
	//printf("fwrd: <<< curr pos %x\n\r", r->curr_pos );
}

int nand_pfw_write(void *fr, void *data, int size)
{
	// TODO
	return 0;
}

/* SEEK_SET 0, SEEK_CUR 1, SEEK_END 2 */
int nand_pfw_seek(void *fr, int offset, int pos)
{
	if (!fr)	{
		return -1;
	}

	fw_rec_t *r = (fw_rec_t *)fr;

	int blksize = snand_blksize;
	int pgsize = snand_pgsize;
	int ppb = snand_ppb;

	switch (pos) {
	case SEEK_SET:
		r->curr_pos = offset;

		break;
	case SEEK_CUR:

		r->curr_pos += offset;
		break;
	case SEEK_END:
		r->curr_pos = r->content_len - offset;

		break;
	}

	int max_pos = r->content_len - 1;
	if (r->curr_pos < 0)	{
		r->curr_pos = 0;
	}
	if (r->curr_pos > max_pos) {
		r->curr_pos = max_pos;
	}

	//printf("seek to offset %d from pos %d = %x\n\r", offset, pos, r->curr_pos);

	int curr_pos = r->curr_pos + r->raw_offset;
	int blkk_idx = curr_pos / blksize;
	int blkk_res = curr_pos - blkk_idx * blksize;
	int page_idx = blkk_res / pgsize;
	int byte_idx = blkk_res - page_idx * pgsize;
	int real_blk_idx = 0;

	r->part_rec = r->part_recs[blkk_idx / 48];

	real_blk_idx = r->part_rec->vmap[blkk_idx % 48];
	// update tmp_buffer

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	snand_page_read(&flash, real_blk_idx * ppb + page_idx, 2048, r->tmp_page);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	r->tmp_page_valid = 1;
	r->tmp_page_index = page_idx;

	return 0;
}

//--------------------------------------------------------------------------------------
// NOR implement
//--------------------------------------------------------------------------------------

typedef struct nor_part_hdr_s {
	uint8_t rec_num;
	uint8_t bl_p_idx;
	uint8_t bl_s_idx;
	uint8_t fw1_idx;
	uint8_t fw2_idx;
	uint8_t iq_idx;
	uint8_t nn_m_idx;
	uint8_t mp_idx;
	uint8_t resv1[8];
	uint16_t ota_trap;
	uint16_t mp_trap;
	uint32_t udl;
	uint8_t resv2[8];
} nor_part_hdr_t;

typedef struct nor_part_rec_s {
	uint32_t start_addr;
	uint32_t length;
	uint16_t type_id;
	uint8_t  resv1[5];
	uint8_t  valid;
	uint8_t  resv2[16];
} nor_part_rec_t;

#define NOR_ADDR(x) (FLASH_BASE+(x))

nor_part_hdr_t *g_nor_part_hdr = NULL;
nor_part_rec_t *g_nor_part_rec = NULL;

void nor_pfw_init(void)
{
	if (g_pfw_inited == 1)	{
		return;
	}

	// find partition record from NOR flash, fix position 0x2000

	g_nor_part_hdr = (nor_part_hdr_t *)NOR_ADDR(0x2000);
	g_nor_part_rec = (nor_part_rec_t *)NOR_ADDR(0x2000 + sizeof(nor_part_hdr_t));

	printf("NOR flash firmware init @ %x\n\r", g_nor_part_rec);

	g_pfw_inited = 1;
}

void nor_pfw_deinit(void)
{
	g_pfw_inited = 0;
}

void nor_pfw_list(void)
{
	nor_part_rec_t *rec = g_nor_part_rec;

	if (g_pfw_inited == 0) {
		return;
	}
	// dump partition items
	int i = 0;
	printf("%4s%8s%8s%8s%8s\t%s\n\r", "rec", "type_id", "addr", "len", "valid", "name");
	while (rec[i].type_id != 0xffff) {

		printf("%4d%8x%8d%8d%8d\t%s\n\r", i, rec[i].type_id, rec[i].start_addr, rec[i].length, rec[i].valid, __get_pt_name(rec[i].type_id));
		i++;
	}
}

unsigned int nor_pfw_get_address(char *name)
{
	uint16_t type_id = __get_pt_type_id(name);

	nor_pfw_init();
	nor_part_rec_t *rec = g_nor_part_rec;
	int i = 0;
	while (rec[i].type_id != 0xffff) {
		if (rec[i].type_id == type_id) {
			return rec[i].start_addr;
		}
		i++;
	}
	return 0xffffffff;
}

typedef struct nor_fw_rec_t {
	uint16_t type_id;

	nor_part_rec_t *part_rec;

	int curr_pos;

	int manifest_valid;
	int content_len;
	int raw_offset;
} nor_fw_rec_t;


nor_part_rec_t *nor_pfw_search_type_id(uint16_t type_id)
{
	nor_part_rec_t *rec = g_nor_part_rec;
	if (g_pfw_inited == 0) {
		return NULL;
	}

	int i = 0;
	while (rec[i].type_id != 0xffff) {
		if (rec[i].type_id == type_id) {
			return &rec[i];
		}
		i++;
	}
	return NULL;
}

void *nor_pfw_open_by_typeid(uint16_t type_id, int mode)
{
	(void)mode;
	// init firmware systemp
	nor_pfw_init();

	// open partition items by type
	nor_fw_rec_t *fr = (nor_fw_rec_t *)malloc(sizeof(nor_fw_rec_t));
	if (!fr)	{
		return NULL;
	}

	memset(fr, 0, sizeof(nor_fw_rec_t));

	fr->type_id = type_id;

	fr->part_rec = nor_pfw_search_type_id(type_id);

	printf("open: part_rec %x, type_id %x\n\r", fr->part_rec, type_id);

	if (!fr->part_rec || fr->part_rec->valid == 0) {
		free(fr);
		return NULL;
	}


	manifest_t *mani = (manifest_t *)NOR_ADDR(fr->part_rec->start_addr);
	if (memcmp(mani->lbl, manifest_valid_label, 8) == 0) {
		img_hdr_t *imghdr = (img_hdr_t *)NOR_ADDR(fr->part_rec->start_addr + 4096);
		fr->manifest_valid = 1;
		//fr->content_len = tlv_get_value(mani->tlv_start, mani->tlv_end, ID_IMGSZ, &fr->content_len);
		fr->content_len = imghdr->imglen;
		fr->raw_offset = 4096 + sizeof(img_hdr_t);
	} else {
		fr->manifest_valid = 0;
		fr->content_len = fr->part_rec->length;
		fr->raw_offset = 0;
	}

	return fr;
}

void *nor_pfw_open(char *name, int mode)
{
	char name_dup[strlen(name) + 2];
	strcpy(name_dup, name);
	char *file_name = name_dup;

	char *type_name = strsep(&file_name, "/");

	printf("type_name %s, file_name %s\n\r", type_name, file_name);


	uint16_t type_id = __get_pt_type_id(type_name);
	if (type_id == 0xffff)	{
		return NULL;
	}

	//printf("open fw partition %s type id %x\n\r", type_name, type_id);

	nor_fw_rec_t *fr = nor_pfw_open_by_typeid(type_id, mode);


	if (file_name != NULL) {
		// search filename and update file size and raw offset
		fwfs_folder_t *tmp = malloc(sizeof(fwfs_folder_t));
		if (!tmp) {
			free(fr);
			return NULL;
		}

		nor_pfw_read(fr, tmp, sizeof(fwfs_folder_t));
		nor_pfw_seek(fr, 0, SEEK_SET);
		//pfw_dump_mem(tmp, sizeof(fwfs_folder_t));
		if (strcmp(tmp->tag, "FWFSDIR") == 0) {
			//printf("FWFSDIR mode, file count %d\n\r", tmp->file_cnt);
			for (int i = 0; i < tmp->file_cnt; i++) {
				//printf("file[%d] %s\n\r", i, tmp->files[i].filename);
				if (strcmp(file_name, tmp->files[i].filename) == 0) {
					//printf("file %s, len %d\n\r", tmp->files[i].filename, tmp->files[i].filelen);
					fr->content_len = tmp->files[i].filelen;
					fr->raw_offset += tmp->files[i].offset;
					break;
				}
			}
		}
	}

	return fr;

}

void nor_pfw_close(void *fr)
{
	if (fr) {
		free(fr);
	}
}

int nor_pfw_tell(void *fr)
{
	nor_fw_rec_t *r = (nor_fw_rec_t *)fr;
	if (!fr)	{
		return -1;
	}

	return r->curr_pos;
}

int nor_pfw_read(void *fr, void *data, int size)
{
	nor_fw_rec_t *r = (nor_fw_rec_t *)fr;

	if (!data)	{
		return -1;
	}
	if (size == 0)	{
		return 0;
	}

	uint32_t start_addr = r->part_rec->start_addr;
	uint32_t max_length = r->part_rec->length;
	// copy to data

	uint8_t *curr_addr = (uint8_t *)NOR_ADDR(r->part_rec->start_addr + r->curr_pos + r->raw_offset);

	int rest_size = r->content_len - r->curr_pos;

	if (rest_size <= 0)	{
		return EOF;
	}

	if (rest_size < size) {
		size = rest_size;
	}

	//printf("dst %x src %x size %d\n\r", data, curr_addr, size);
	memcpy(data, curr_addr, size);

	r->curr_pos += size;

	return size;
}


int nor_pfw_seek(void *fr, int offset, int pos)
{
	if (!fr)	{
		return -1;
	}

	nor_fw_rec_t *r = (nor_fw_rec_t *)fr;

	switch (pos) {
	case SEEK_SET:
		r->curr_pos = offset;

		break;
	case SEEK_CUR:

		r->curr_pos += offset;
		break;
	case SEEK_END:
		r->curr_pos = r->content_len - offset;

		break;
	}

	int max_pos = r->content_len - 1;
	if (r->curr_pos < 0)	{
		r->curr_pos = 0;
	}
	if (r->curr_pos > max_pos) {
		r->curr_pos = max_pos;
	}

	return 0;

}

//--------------------------------------------------------------------------------------
// API interface
//--------------------------------------------------------------------------------------
typedef struct fwfs_interface_s {
	void (*init)(void);
	void (*deinit)(void);
	void (*list)(void);
	void *(*open_by_typeid)(uint16_t, int);
	void *(*open)(char *, int);
	void (*close)(void *);
	int (*tell)(void *);
	int (*read)(void *, void *, int);
	int (*write)(void *, void *, int);
	int (*seek)(void *, int, int);
} fwfs_interface_t;

static fwfs_interface_t nand_fw = {
	.init   		= nand_pfw_init,
	.deinit 		= nand_pfw_init,
	.list			= nand_pfw_list,
	.open_by_typeid = nand_pfw_open_by_typeid,
	.open 			= nand_pfw_open,
	.close 			= nand_pfw_close,
	.tell 			= nand_pfw_tell,
	.read 			= nand_pfw_read,
	.write 			= NULL,
	.seek 			= nand_pfw_seek
};

static fwfs_interface_t nor_fw = {
	.init   		= nor_pfw_init,
	.deinit 		= nor_pfw_init,
	.list			= nor_pfw_list,
	.open_by_typeid = nor_pfw_open_by_typeid,
	.open 			= nor_pfw_open,
	.close 			= nor_pfw_close,
	.tell 			= nor_pfw_tell,
	.read 			= nor_pfw_read,
	.write 			= NULL,
	.seek 			= nor_pfw_seek
};


static fwfs_interface_t *curr = NULL;

void pfw_init(void)
{
	// very ugly method to detect what type flash on SPIC, suck
	if (sys_get_boot_sel() == 0) { // NOR
		curr = &nor_fw;
	} else if (sys_get_boot_sel() == 1) { // NAND
		curr = &nand_fw;
	} else {
		printf("Cannot use flash firmware in this mode\n\t");
		while (1);
	}

	if (curr && curr->init) {
		curr->init();
	}
}

void pfw_deinit(void)
{
	if (curr && curr->deinit) {
		curr->deinit();
	}
}

void pfw_list(void)
{
	if (curr && curr->list) {
		curr->list();
	}
}

void *pfw_open_by_typeid(uint16_t type_id, int mode)
{
	if (curr && curr->open_by_typeid) {
		return curr->open_by_typeid(type_id, mode);
	}
	return NULL;
}

void *pfw_open(char *name, int mode)
{
	pfw_init();
	if (curr && curr->open) {
		return curr->open(name, mode);
	}
	return NULL;
}

void pfw_close(void *fr)
{
	if (curr && curr->close) {
		curr->close(fr);
	}
}

int pfw_tell(void *fr)
{
	if (curr && curr->tell) {
		return curr->tell(fr);
	}
	return 0;
}

int pfw_read(void *fr, void *data, int size)
{
	if (curr && curr->read) {
		return curr->read(fr, data, size);
	}
	return -1;
}

int pfw_write(void *fr, void *data, int size)
{
	if (curr && curr->write) {
		return curr->write(fr, data, size);
	}
	return -1;
}

int pfw_seek(void *fr, int offset, int pos)
{
	if (curr && curr->seek) {
		return curr->seek(fr, offset, pos);
	}
	return -1;
}



//--------------------------------------------------------------------------------------
// Test command
//--------------------------------------------------------------------------------------

void fLSFW(void *arg)
{
	pfw_init();
	//pfw_list();
	pfw_list();
}

void fDUMP(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	argc = parse_param(arg, argv);

	uint32_t addr = strtoul(argv[1], NULL, 16);
	uint32_t len = strtoul(argv[2], NULL, 16);

	pfw_dump_mem((void *)addr, len);
}

// FWRD=NN_MDL,seek_pos,read_len
void fFWRD(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	argc = parse_param(arg, argv);

	pfw_init();
	void *fp = pfw_open(argv[1], 0);
	if (!fp) {
		printf("cannot open file %s\n\r", argv[1]);
		return;
	}

	int offset = strtol(argv[2], NULL, 16);
	int rlen = strtol(argv[3], NULL, 10);

	uint8_t *tmp_buf = malloc(rlen + 32);
	if (!tmp_buf) {
		printf("out of memory\n\r");
		pfw_close(fp);
		return;
	}

	printf("offset %x, read len %d, target %x\n\r", offset, rlen, tmp_buf);

	pfw_seek(fp, offset, SEEK_SET);
	pfw_read(fp, tmp_buf, rlen);

	pfw_dump_mem(tmp_buf, rlen);

	pfw_close(fp);
	free(tmp_buf);
}

void fFWRT(void *arg)
{
	//int argc = 0;
	//char *argv[MAX_ARGC] = {0};
	//argc = parse_param(arg, argv);

	pfw_init();

	void *fp = pfw_open_by_typeid(0xC1C7, 0);

	uint8_t *ref = malloc(4096);
	uint8_t *tmp = malloc(4096);

	pfw_read(fp, ref, 2048);
	pfw_read(fp, &ref[2048], 2048);

	//pfw_dump_mem(ref, 4096);

	// test seek and internal tmp_page read
	pfw_seek(fp, 0, SEEK_SET);
	pfw_read(fp, tmp, 4096);
	//pfw_dump_mem(tmp, 4096);
	if (memcmp(ref, tmp, 4096) != 0) {
		printf("test 1 not match\n\r");
		if (memcmp(ref, tmp, 2048) != 0) {
			pfw_dump_mem(ref, 2048);
			pfw_dump_mem(tmp, 2048);
		}
		if (memcmp(&ref[2048], &tmp[2048], 2048) != 0) {
			pfw_dump_mem(&ref[2048], 2048);
			pfw_dump_mem(&tmp[2048], 2048);
		}
	} else {
		printf("test 1 matched\n\r");
	}

	// small size read
	int bi = 0;
	pfw_seek(fp, 0, SEEK_SET);
	pfw_read(fp, &tmp[bi], 1);
	bi += 1;
	pfw_read(fp, &tmp[bi], 1);
	bi += 1;
	pfw_read(fp, &tmp[bi], 1);
	bi += 1;
	pfw_read(fp, &tmp[bi], 1);
	bi += 1;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 64);
	bi += 64;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 4);
	bi += 4;
	pfw_read(fp, &tmp[bi], 256);
	bi += 256;


	if (memcmp(ref, tmp, bi) != 0) {
		printf("test 1-1 not match\n\r");
	} else {
		printf("test 1-1 matched\n\r");
	}

	// test under byte read
	pfw_seek(fp, 0, SEEK_SET);
	for (int i = 0; i < 4096; i += 8) {
		pfw_read(fp, &tmp[i], 8);
	}

	if (memcmp(ref, tmp, 4096) != 0) {
		printf("test 2 not match\n\r");
	} else {
		printf("test 2 matched\n\r");
	}


	// test cross page read
	pfw_seek(fp, 2045, SEEK_SET);
	pfw_read(fp, tmp, 8);

	if (memcmp(&ref[2045], tmp, 8) != 0) {
		printf("test 3 not match\n\r");
	} else {
		printf("test 3 matched\n\r");
	}

	// test cross block read
	pfw_seek(fp, 63 * 2048, SEEK_SET);

	pfw_read(fp, ref, 2048);
	pfw_read(fp, &ref[2048], 2048);

	//pfw_dump_mem(ref, 4096);

	pfw_seek(fp, 63 * 2048, SEEK_SET);
	pfw_read(fp, tmp, 4096);

	if (memcmp(ref, tmp, 4096) != 0) {
		printf("test 4 not match\n\r");
	} else {
		printf("test 4 matched\n\r");
	}


	// test cross page read
	pfw_seek(fp, 63 * 2048 + 2045, SEEK_SET);
	pfw_read(fp, tmp, 8);

	if (memcmp(&ref[2045], tmp, SEEK_SET) != 0) {
		printf("test 5 not match\n\r");
	} else {
		printf("test 5 matched\n\r");
	}

	// test huge data size read
	free(ref);
	free(tmp);

	ref = malloc(256 * 1024 + 32);
	tmp = malloc(256 * 1024 + 32);

	pfw_seek(fp, 0, SEEK_SET);
	for (int i = 0; i < 256 * 1024; i += 2048) {
		pfw_read(fp, &ref[i], 2048);
	}


	pfw_seek(fp, 0, SEEK_SET);
	pfw_read(fp, tmp, 256 * 1024);

	//pfw_dump_mem(&ref[128*1024], 2048);
	//pfw_dump_mem(&tmp[128*1024], 2048);

	if (memcmp(ref, tmp, 256 * 1024) != 0) {
		printf("test 6 not match\n\r");
	} else {
		printf("test 6 matched\n\r");
	}

	pfw_seek(fp, 2048, SEEK_SET);
	pfw_read(fp, tmp, 128 * 1024);

	if (memcmp(&ref[2048], tmp, 128 * 1024) != 0) {
		printf("test 7 not match\n\r");
		for (int i = 0; i < 128 * 1024; i += 2048) {
			if (memcmp(&ref[2048 + i], &tmp[i], 2048) != 0) {
				printf("test 7.(%x) not match\n\r", i / 2048);
			}
		}
	} else {
		printf("test 7 matched\n\r");
	}

	int pos = pfw_tell(fp);

	pfw_seek(fp, 123, SEEK_CUR);
	pfw_read(fp, tmp, 2049);

	if (memcmp(&ref[pos + 123], tmp, 2049) != 0) {
		printf("test 8 not match\n\r");
	} else {
		printf("test 8 matched\n\r");
	}

	free(ref);
	free(tmp);

	pfw_close(fp);
}



log_item_t pfw_items[] = {
	{"LSFW", fLSFW,},
	{"FWRT", fFWRT,},
	{"FWRD", fFWRD,},
	{"DUMP", fDUMP,},
};

void atcmd_pfw_init(void)
{
	log_service_add_table(pfw_items, sizeof(pfw_items) / sizeof(pfw_items[0]));
}
