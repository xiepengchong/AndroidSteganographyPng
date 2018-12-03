#ifndef STEGO_H_FILE

#include <stdio.h>
#define STEGO_H_FILE

//Program killer
#ifdef __cplusplus
extern "C" {
#endif
void die(char* reason);
#ifdef __cplusplus
}
#endif
//Check that the file is P6 format
#ifdef __cplusplus
extern "C" {
#endif
int read_ppm_type(FILE *);
#ifdef __cplusplus
}
#endif
//Skips past the comments in the file
#ifdef __cplusplus
extern "C" {
#endif
void skip_comments(FILE *);
#ifdef __cplusplus
}
#endif
//Get's the width of the image
#ifdef __cplusplus
extern "C" {
#endif
int get_width(FILE *);
#ifdef __cplusplus
}
#endif
//Get's the height of the image
#ifdef __cplusplus
extern "C" {
#endif
int get_height(FILE *);
#ifdef __cplusplus
}
#endif
//Returns 1 if 255, else returns 0
#ifdef __cplusplus
extern "C" {
#endif
int read_color_depth(FILE *);
#ifdef __cplusplus
}
#endif
#endif