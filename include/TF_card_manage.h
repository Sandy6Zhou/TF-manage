#ifndef __TF_CARD_MANAGE_H__
#define __TF_CARD_MANAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include <regex.h>

#define PATH_SDCARD "/home/zsd/workspace/TF-manage/mnt/sdcard"	//改为本地设备存储数据的路径即可
#define LOG_DEBUG 1

typedef enum{
	FORMAT_MP4 = 0,
	FORMAT_JPG
}File_format;

typedef enum{
	FULLTIME = 1<<0,
	TIMING = 1<<1,
	MOTION = 1<<2,
	HUMAN = 1<<3,
	DEVICE = 1<<4,
	TIMELAPSE = 1<<5,

	RESERVE = 1<<6
}File_type;

#define ALL_VIDEO	(FULLTIME | TIMING | MOTION | HUMAN | DEVICE | TIMELAPSE)

typedef struct{
	FILE *fp;
	char folder_path[64];
	char file_name[64];
	char file_suffix[16];
	int start_time;
	int end_time;
	int type;
}TF_data;

void creat_file(TF_data *tf_data, File_format _file_format, File_type _file_type);
void add_file_type(TF_data *tf_data, File_type _file_type);
void close_file(TF_data *tf_data);

int del_file(uint64_t _file_id, File_format _file_format);
FILE *get_file_fp(uint64_t _file_id, File_format _file_format);
char *get_file_path(uint64_t _file_id, File_format _file_format);

int del_1hours_file(void);

cJSON *get_files_info_front(File_type _file_type, int num_get, uint64_t start_time);
cJSON *get_files_info_back(File_type _file_type, int num_get, uint64_t start_time);
cJSON *get_files_info_middle(File_type _file_type, uint64_t start_time, uint64_t end_time);

#endif