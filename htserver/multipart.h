#ifndef	_INC_MULTIPART_H
#define	_INC_MULTIPART_H

typedef struct _MultipartFile {
    char *filename;
    char *value;
    int length;
} MultipartFile;

#endif
