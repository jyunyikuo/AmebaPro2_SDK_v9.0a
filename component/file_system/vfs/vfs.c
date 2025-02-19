#include <stdio.h>
#include "freertos_service.h"
#include "time.h"
#include "vfs.h"

vfs_drv  vfs = {0};

_mutex vfs_mutex = NULL;

void vfs_init(void *parm)
{
	if (vfs_mutex == NULL) {
		printf("vfs_mutex init\r\n");
		memset(&vfs, 0, sizeof(vfs_drv));
		rtw_mutex_init(&vfs_mutex);
	} else {
		printf("vfs_mutex is already init\r\n");
	}
}

void vfs_deinit(void *parm)
{
	if (vfs_mutex != NULL) {
		printf("vfs_mutex deinit\r\n");
		memset(&vfs, 0, sizeof(vfs_drv));
		rtw_mutex_free(&vfs_mutex);
		vfs_mutex = NULL;
	} else {
		printf("vfs_mutex is already deinit\r\n");
	}
}

int find_vfs_number(const char *name, int *prefix_len, int *user_id)
{
	int i, j = 0;
	int ret = -1;
	for (i = 0; i < MAX_FS_SIZE; i++) {
		ret =  strncmp(name, vfs.user[i].tag, strlen(vfs.user[i].tag));
		if (ret == 0) {
			for (j = strlen(vfs.user[i].tag); j < strlen(name); j++) {
				if (name[j] != '/' && name[j] != ':') {
					*prefix_len = j;
					break;
				} else {
					if ((j + 1) == strlen(name)) {
						*prefix_len = j + 1;
						break;
					}
				}
			}
			//printf("name %s %s prefix_len %d\r\n",name,__FUNCTION__,*prefix_len);
			*user_id = i;
			return vfs.user[i].vfs_type_id;
		}
	}
	return ret;
}

int find_inf_number(const char *name)
{
	int i = 0;
	int ret = 0;
	for (i = 0; i < MAX_USER_SIZE; i++) {
		if (vfs.user[i].tag != NULL) {
			ret =  strncmp(name, vfs.user[i].tag, strlen(vfs.user[i].tag));
			if (ret == 0) {
				printf("Correct %s %d\r\n", __FUNCTION__, i);
				return i;
			} else {
				printf("Wrong %s %d\r\n", __FUNCTION__, i);
			}
		}
	}
	return -1;
}

int vfs_register(vfs_opt *drv, int vfs_type)
{
	unsigned char drv_num = -1;
	if (vfs.nbr < MAX_FS_SIZE) {
		drv->drv_num = vfs.nbr;	// record driver number for a specific disk
		vfs.drv[vfs.nbr] = drv;
		vfs.drv[vfs.nbr]->vfs_type = vfs_type;
		vfs.nbr++;
		drv_num = drv->drv_num;
	}
	return drv_num;
}

int vfs_scan_vfs(int vfs_type)
{
	int vfs_num = -1;
	int i = 0;
	for (i = 0; i < vfs.nbr; i++) {
		if (vfs.drv[i]->vfs_type == vfs_type) {
			vfs_num = i;
		}
	}
	return vfs_num;
}

int vfs_user_register(char *prefix, int vfs_type, int interface)
{
	int vfs_num = 0;
	int ret = -1;
	rtw_mutex_get(&vfs_mutex);
	if (vfs_type != VFS_FATFS && vfs_type != VFS_LITTLEFS) {
		printf("It don't support the file system\r\n");
	} else {
		vfs_num = vfs_scan_vfs(vfs_type);
		if (vfs_num < 0) {
			if (vfs_type == VFS_FATFS) {
				vfs_num = vfs_register(&fatfs_drv, vfs_type);
				printf("fatfs register\r\n");
			} else {
				vfs_num = vfs_register(&littlefs_drv, vfs_type);
				printf("littlefs register\r\n");
			}
		}
		vfs.user[vfs.ibr].tag = prefix;
		vfs.user[vfs.ibr].vfs_type = vfs_type;
		vfs.user[vfs.ibr].vfs_interface_type = interface;
		vfs.user[vfs.ibr].vfs_type_id = vfs_num;
		vfs.ibr++;
		ret = vfs.drv[vfs_num]->mount(interface);
	}
	rtw_mutex_put(&vfs_mutex);
	return ret;
}

int vfs_user_unregister(char *prefix, int vfs_type, int interface)
{
	int user_id = 0;
	int vfs_id = 0;
	int ret = -1;
	rtw_mutex_get(&vfs_mutex);
	user_id = find_inf_number(prefix);
	if (user_id >= 0) {
		vfs_id = vfs.user[user_id].vfs_type_id;
		memset(&vfs.user[user_id], 0x00, sizeof(user_config));
		ret = vfs.drv[vfs_id]->unmount(interface);
		vfs.ibr--;
	} else {
		ret = -1;
	}
	rtw_mutex_put(&vfs_mutex);
	return ret;
}