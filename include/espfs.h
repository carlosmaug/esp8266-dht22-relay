#ifndef ESPFS_H
#define ESPFS_H

typedef struct EspFsFile EspFsFile;

EspFsFile *espfs_open(char *fileName);
int espfs_read(EspFsFile *fh, char *buff, int len);
void espfs_close(EspFsFile *fh);

#endif
