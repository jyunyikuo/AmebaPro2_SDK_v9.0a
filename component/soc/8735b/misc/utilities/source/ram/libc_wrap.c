/**************************************************************************//**
 * @file     libc_wrap.c
 * @brief    The wraper functions of ROM code to replace some of utility
 *           functions in Compiler's Library.
 * @version  V1.00
 * @date     2018-08-15
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
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
#if defined(CONFIG_PLATFORM_8195BHP) \
	|| defined(CONFIG_PLATFORM_8195BLP) \
	|| defined(CONFIG_PLATFORM_8710C) \
	|| defined(CONFIG_PLATFORM_8735B)
#include "platform_conf.h"
#endif
#include "basic_types.h"

__attribute__((weak)) void *__dso_handle = 0;

#if defined(CONFIG_CMSIS_FREERTOS_EN) && (CONFIG_CMSIS_FREERTOS_EN != 0)
/**************************************************
 * FreeRTOS memory management functions's wrapper to replace
 * malloc/free/realloc of GCC Lib.
 **************************************************/
//#include "FreeRTOS.h"
// pvPortReAlloc currently not defined in portalbe.h
extern void *pvPortReAlloc(void *pv,  size_t xWantedSize);
extern void *pvPortMalloc(size_t xWantedSize);
extern void *pvPortCalloc(size_t xWantedCnt, size_t xWantedSize);
extern void vPortFree(void *pv);

void *__wrap_malloc(size_t size)
{
	return pvPortMalloc(size);
}

void *__wrap_realloc(void *p, size_t size)
{
	return (void *)pvPortReAlloc(p, size);
}

void *__wrap_calloc(size_t cnt, size_t size)
{
	void *pbuf;
	uint32_t total_size;

	total_size = cnt * size;
	pbuf = pvPortMalloc(total_size);
	memset(pbuf, 0, total_size);

	return pbuf;
//	return (void*)pvPortCalloc(cnt, size);
}

void __wrap_free(void *p)
{
	vPortFree(p);
}
#endif  //  #if defined(CONFIG_CMSIS_FREERTOS_EN) && (CONFIG_CMSIS_FREERTOS_EN != 0)

/**************************************************
 * string and memory api wrap for compiler
 *
 **************************************************/

#if defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8195BLP) || defined(CONFIG_PLATFORM_8710C)\
	|| defined(CONFIG_PLATFORM_8735B)
#include "stdio_port.h"
#include "rt_printf.h"


int __wrap_puts(const char *str)
{
	while (*str != '\0') {
		stdio_printf_stubs.stdio_port_putc(*str++);
	}
	// converter LF --> CRLF
	stdio_printf_stubs.stdio_port_putc('\r');
	stdio_printf_stubs.stdio_port_putc('\n');
	return 0;
}

int __wrap_printf(const char *fmt, ...)
{
	int count;
	va_list list;
	va_start(list, fmt);
#if defined(CONFIG_BUILD_SECURE)
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_sputc, (void *)NULL, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_sputc, (void *)NULL, fmt, list);
#endif
	va_end(list);
	return count;
}

int __wrap_vprintf(const char *fmt, va_list args)
{
	int count;
#if defined(CONFIG_BUILD_SECURE)
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_sputc, (void *)NULL, fmt, args);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_sputc, (void *)NULL, fmt, args);
#endif
	return count;
}

int __wrap_sprintf(char *buf, const char *fmt, ...)
{
	int count;
	va_list list;
	stdio_buf_t pnt_buf;

	pnt_buf.pbuf = buf;
	pnt_buf.pbuf_lim = 0;

	va_start(list, fmt);
#if defined(CONFIG_BUILD_SECURE)
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#endif
	*(pnt_buf.pbuf) = 0;
	va_end(list);
	(void)list;

	return count;
}

int __wrap_snprintf(char *buf, size_t size, const char *fmt, ...)
{
	int count;
	va_list list;
	stdio_buf_t pnt_buf;

	pnt_buf.pbuf = buf;
	pnt_buf.pbuf_lim = buf + size - 1;  // reserve 1 byte for 'end of string'

	va_start(list, fmt);
#if defined(CONFIG_BUILD_SECURE)
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#endif
	*(pnt_buf.pbuf) = 0;
	va_end(list);
	(void)list;

	return count;
}

int __wrap_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int count;
	stdio_buf_t pnt_buf;

	pnt_buf.pbuf = buf;
	pnt_buf.pbuf_lim = buf + size - 1;  // reserve 1 byte for 'end of string'
#if defined(CONFIG_BUILD_SECURE)
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, args);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, args);
#endif
	*(pnt_buf.pbuf) = 0;

	return count;
}


// define in AmebaPro utilites/include/memory.h
#include "memory.h"
int __wrap_memcmp(const void *av, const void *bv, size_t len)
{
	return rt_memcmp(av, bv, len);
}

void *__wrap_memcpy(void *s1, const void *s2, size_t n)
{
	return rt_memcpy(s1, s2, n);
}

void *__wrap_memmove(void *destaddr, const void *sourceaddr, unsigned length)
{
	return rt_memmove(destaddr, sourceaddr, length);
}

void *__wrap_memset(void *dst0, int val, size_t length)
{
	return rt_memset(dst0, val, length);
}
// define in AmebaPro utilites/include/strporc.h
// replace by linking command
#include "strproc.h"
char *__wrap_strcat(char *dest,  char const *src)
{
	return strcat(dest, src);
}

char *__wrap_strchr(const char *s, int c)
{
	return strchr(s, c);
}

int __wrap_strcmp(char const *cs, char const *ct)
{
	return strcmp(cs, ct);
}

int __wrap_strncmp(char const *cs, char const *ct, size_t count)
{
	return strncmp(cs, ct, count);
}

int __wrap_strnicmp(char const *s1, char const *s2, size_t len)
{
	return strnicmp(s1, s2, len);
}


char *__wrap_strcpy(char *dest, char const *src)
{
	return strcpy(dest, src);
}


char *__wrap_strncpy(char *dest, char const *src, size_t count)
{
	return strncpy(dest, src, count);
}


size_t __wrap_strlcpy(char *dst, char const *src, size_t s)
{
	return strlcpy(dst, src, s);
}


size_t __wrap_strlen(char const *s)
{
	return strlen(s);
}


size_t __wrap_strnlen(char const *s, size_t count)
{
	return strnlen(s, count);
}


char *__wrap_strncat(char *dest, char const *src, size_t count)
{
	return strncat(dest, src, count);
}

char *__wrap_strpbrk(char const *cs, char const *ct)
{
	return strpbrk(cs, ct);
}


size_t __wrap_strspn(char const *s, char const *accept)
{
	return strspn(s, accept);
}


char *__wrap_strstr(char const *s1, char const *s2)
{
	return strstr(s1, s2);
}


char *__wrap_strtok(char *s, char const *ct)
{
	return strtok(s, ct);
}


size_t __wrap_strxfrm(char *dest, const char *src, size_t n)
{
	return strxfrm(dest, src, n);
}

char *__wrap_strsep(char **s, const char *ct)
{
	return strsep(s, ct);
}

double __wrap_strtod(const char *str, char **endptr)
{
	return strtod(str, endptr);
}

float __wrap_strtof(const char *str, char **endptr)
{
	return strtof(str, endptr);
}


long double __wrap_strtold(const char *str, char **endptr)
{
	return strtold(str, endptr);
}

long __wrap_strtol(const char *nptr, char **endptr, int base)
{
	return strtol(nptr, endptr, base);
}


long long __wrap_strtoll(const char *nptr, char **endptr, int base)
{
	return strtoll(nptr, endptr, base);
}


unsigned long __wrap_strtoul(const char *nptr, char **endptr, int base)
{
	return strtoul(nptr, endptr, base);
}


unsigned long long __wrap_strtoull(const char *nptr, char **endptr, int base)
{
	return strtoull(nptr, endptr, base);
}

int __wrap_atoi(const char *num)
{
	return atoi(num);
}

unsigned int __wrap_atoui(const char *num)
{
	return atoui(num);
}

long __wrap_atol(const char *num)
{
	return atol(num);
}

unsigned long __wrap_atoul(const char *num)
{
	return atoul(num);
}


unsigned long long __wrap_atoull(const char *num)
{
	return atoull(num);
}


double __wrap_atof(const char *str)
{
	return atof(str);
}

void __wrap_abort(void)
{
	__wrap_printf("\n\rabort execution\n\r");
	while (1);
}

/**************************************************
* FILE api wrap for compiler
*
**************************************************/
/*
--redirect fopen=__wrap_fopen
--redirect fclose=__wrap_fclose
--redirect fread=__wrap_fread
--redirect fwrite=__wrap_fwrite
--redirect fseek=__wrap_fseek
--redirect fsetpos=__wrap_fsetpos
--redirect fgetpos=__wrap_fgetpos
--redirect rewind=__wrap_rewind
--redirect fflush=__wrap_fflush
--redirect remove=__wrap_remove
--redirect rename=__wrap_rename
--redirect feof=__wrap_feof
--redirect ferror=__wrap_ferror
--redirect ftell=__wrap_ftell
--redirect fputc=__wrap_fputc
--redirect fputs=__wrap_fputs
--redirect fgets=__wrap_fgets
*/


#include <stdio.h>
#include "time.h"
#include "vfs.h"

typedef int(*qsort_compar)(const void *, const void *);
int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcoll((*a)->d_name, (*b)->d_name);
}

FILE *__wrap_fopen(const char *filename, const char *mode)
{
	int prefix_len = 0;
	int inf_id = 0;
	int drv_id = 0;
	int ret = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(filename, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	vfs_file *finfo = __wrap_malloc(sizeof(vfs_file));
	if (finfo == NULL) {
		return NULL;
	}
	memset(finfo, 0x00, sizeof(vfs_file));
	finfo->vfs_id = vfs_id;
	if (finfo->vfs_id == VFS_FATFS) {
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(finfo->name, sizeof(finfo->name), "%s%s", temp, filename + prefix_len);
	} else {
		snprintf(finfo->name, sizeof(finfo->name), "%s", filename + prefix_len);
	}
	//printf("finfo->name %s\r\n",finfo->name);
	ret = vfs.drv[vfs_id]->open(finfo->name, mode, finfo);
	return (FILE *)finfo;
}

int __wrap_fclose(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->close((vfs_file *)stream);
	__wrap_free(finfo);
	return ret;
}

size_t __wrap_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->read(ptr, size, count, (vfs_file *)stream);
	return ret;
}

size_t __wrap_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->write((void *)ptr, size, count, (vfs_file *)stream);
	return ret;
}

int  __wrap_fseek(FILE *stream, long int offset, int origin)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->seek(offset, origin, (vfs_file *)stream);
	return ret;
}

void  __wrap_rewind(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	vfs.drv[finfo->vfs_id]->rewind((vfs_file *)stream);
}

int __wrap_fgetpos(FILE *stream, fpos_t   *p)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
#if defined(__ICCARM__)
	p->_Off = vfs.drv[finfo->vfs_id]->fgetpos((vfs_file *)stream);
#elif defined(__GNUC__)
	*p = vfs.drv[finfo->vfs_id]->fgetpos((vfs_file *)stream);
#endif
	return 0;
}

int __wrap_fsetpos(FILE *stream, fpos_t   *p)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
#if defined(__ICCARM__)
	ret = vfs.drv[finfo->vfs_id]->fsetpos(p->_Off, (vfs_file *)stream);
#elif defined(__GNUC__)
	ret = vfs.drv[finfo->vfs_id]->fsetpos((unsigned int) * p, (vfs_file *)stream);
#endif
	return ret;
}

int  __wrap_fflush(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->fflush((vfs_file *)stream);
	return ret;
}

int __wrap_remove(const char *filename)
{
	int ret = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(filename, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, filename + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", filename + prefix_len);
	}
	//printf("name %s\r\n",name);
	ret = vfs.drv[vfs_id]->remove(name);
	return ret;
}

int __wrap_rename(const char *oldname, const char *newname)
{
	int ret = 0;
	char old_name[1024] = {0};
	char new_name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(oldname, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(old_name, sizeof(old_name), "%s%s", temp, oldname + prefix_len);
		snprintf(new_name, sizeof(new_name), "%s%s", temp, newname + prefix_len);
	} else {
		snprintf(old_name, sizeof(old_name), "%s", oldname + prefix_len);
		snprintf(new_name, sizeof(new_name), "%s", newname + prefix_len);
	}
	//printf("old_name %s new_name %s\r\n",old_name,new_name);
	ret = vfs.drv[vfs_id]->rename(old_name, new_name);
	return ret;
}

int __wrap_feof(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->eof((vfs_file *)stream);
	return ret;
}

int __wrap_ferror(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->error((vfs_file *)stream);
	return ret;
}

long int __wrap_ftell(FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->tell((vfs_file *)stream);
	return ret;
}

#include "stdio_port_func.h"
int __wrap_fputc(int character, FILE *stream)
{
	if (stream == stdout || stream == stderr) {
		stdio_port_putc(character);
		if (character == '\n') {
			stdio_port_putc('\r');
		}
		return character;
	}
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->fputc(character, (vfs_file *)stream);
	return ret;
}

int __wrap_fputs(const char *str, FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->fputs(str, (vfs_file *)stream);
	return ret;
}

char *__wrap_fgets(char *str, int num, FILE *stream)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)stream;
	ret = vfs.drv[finfo->vfs_id]->fgets(str, num, (vfs_file *)stream);
	return ret;
}

DIR *__wrap_opendir(const char *name)
{
	int ret = 0;
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(name, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	vfs_file *finfo = __wrap_malloc(sizeof(vfs_file));
	if (finfo == NULL) {
		return NULL;
	}
	memset(finfo, 0x00, sizeof(vfs_file));
	finfo->vfs_id = vfs_id;
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(finfo->name, sizeof(finfo->name), "%s%s", temp, name + prefix_len);
	} else {
		snprintf(finfo->name, sizeof(finfo->name), "%s", name + prefix_len);
	}
	ret = vfs.drv[vfs_id]->opendir(finfo->name, finfo);
	return (DIR *)finfo;
}

struct dirent *__wrap_readdir(DIR *pdir)
{
	struct dirent *ent = NULL;
	vfs_file *finfo = (vfs_file *)pdir;
	ent = vfs.drv[finfo->vfs_id]->readdir(((vfs_file *)pdir));
	return ent;
}

int __wrap_closedir(DIR *dirp)
{
	int ret = 0;
	vfs_file *finfo = (vfs_file *)dirp;
	ret = vfs.drv[finfo->vfs_id]->closedir(((vfs_file *)dirp));
	__wrap_free(finfo);
	return ret;
}

int __wrap_scandir(const char *dirp, struct dirent ***namelist,
				   int (*filter)(const struct dirent *),
				   int (*compar)(const struct dirent **, const struct dirent **))
{
	int count = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(dirp, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, dirp + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", dirp + prefix_len);
	}
	//printf("name %s\r\n",name);
	count = vfs.drv[vfs_id]->scandir(name, namelist, filter, compar);
	return count;

}

int __wrap_rmdir(const char *path)
{
	int ret = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(path, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, path + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", path + prefix_len);
	}
	//printf("name %s\r\n",name);
	ret = vfs.drv[vfs_id]->rmdir(name);
	return ret;
}

int __wrap_mkdir(const char *pathname, mode_t mode)
{
	int ret = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(pathname, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, pathname + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", pathname + prefix_len);
	}
	//printf("name %s\r\n",name);
	ret = vfs.drv[vfs_id]->mkdir(name);
	return ret;
}

int __wrap_access(const char *pathname, int mode)
{
	int ret = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(pathname, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, pathname + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", pathname + prefix_len);
	}
	//snprintf(name,sizeof(name),"%s",pathname+prefix_len);
	//printf("name %s\r\n",name);
	ret = vfs.drv[vfs_id]->access(name, mode);
	return ret;
}

int __wrap_stat(const char *path, struct stat *buf)
{
	int ret = 0;
	char name[1024] = {0};
	int prefix_len = 0;
	int user_id = 0;
	int vfs_id = find_vfs_number(path, &prefix_len, &user_id);
	if (vfs_id < 0) {
		printf("It can't find the file system\r\n");
		return NULL;
	}
	if (vfs_id == VFS_FATFS) {
		int drv_id = 0;
		drv_id = vfs.drv[vfs_id]->get_interface(vfs.user[user_id].vfs_interface_type);
		char temp[4] = {0};
		temp[0] = drv_id + '0';
		temp[1] = ':';
		temp[2] = '/';
		snprintf(name, sizeof(name), "%s%s", temp, path + prefix_len);
	} else {
		snprintf(name, sizeof(name), "%s", path + prefix_len);
	}
	//printf("name %s\r\n",name);
	ret = vfs.drv[vfs_id]->stat(name, buf);
	return ret;
}

#if defined(__GNUC__)
#include <errno.h>

static int gnu_errno;
volatile int *__aeabi_errno_addr(void)
{
	return &gnu_errno;
}
#endif

#endif // #if defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8195BLP) \
|| defined(CONFIG_PLATFORM_8710C) || defined(CONFIG_PLATFORM_8715B)

