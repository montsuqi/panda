#ifndef	_INC_MULTIPART_H
#define	_INC_MULTIPART_H

typedef struct _MultipartFile {
    char *filename;
    char *value;
    int length;
} MultipartFile;

char *GetMultipartBoundary(char *content_type);
int ParseMultipart(FILE *fp, char *boundary,
                   GHashTable *values, GHashTable *files);

#endif
