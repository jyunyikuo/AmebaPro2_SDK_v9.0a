#ifndef FWFS_H
#define FWFS_H

void *pfw_open(char *name, int mode);
void pfw_close(void *fr);
int pfw_read(void *fr, void *data, int size);
int pfw_seek(void *fr, int offset, int pos);
int pfw_tell(void *fr);
int pfw_write(void *fr, void *data, int size);

void *pfw_open_by_typeid(uint16_t type_id, int mode);
void pfw_list(void);

void pfw_init(void);
void pfw_deinit(void);

// API for NOR
unsigned int nor_pfw_get_address(char *name);

#endif