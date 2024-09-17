#include "TF_card_manage.h"

static char *file_type_str[6] = {"FULLTIME", "TIMING", "MOTION", "HUMAN", "DEVICE", "TIMELAPSE"};
static char *file_mode_str[3] = {"[Mode_DEL]", "[Mode_GET_FP]", "[Mode_GET_PATH]"};

typedef enum{
	DATE_FOLDER = 0,
	HOUR_FOLDER
}Folder_Type;

typedef enum{
	CHECK_FILE = 1,
	CHECK_DIR
}Check_Type;

typedef enum{
	Time_Start = 0,
	Time_End
}Time_print;

typedef enum{
	Mode_DEL = 0,
	Mode_GET_FP,
	Mode_GET_PATH
}Mode_t;

typedef enum{
	FRONT = 0,
	MIDDLE,
	BACK
}Find_mode;

typedef struct {
    char **dir_name;
	int dir_index;
	int dir_count;
}Folder_Info;

typedef struct{
	char folder_prefixes[32];
    int start_second;
    int end_second;
	int type;
}FileName;

static Folder_Info date_info;
static Folder_Info hour_info;

static int getPowerOfTwo(int num){
    int count = 0;
    while(num > 1){
        num >>= 1;
        count++;
    }
    return count;
}

static void print_tf_data(TF_data tf_data, Time_print time_print)
{
	char type[256] = {0};
	for(int i = getPowerOfTwo(RESERVE)-1; i >= 0; i--){
		if(tf_data.type & (1<<i)){
			if(strlen(type) == 0){
				sprintf(type ,"%s", file_type_str[i]);
			}else{
				sprintf(type ,"%s|%s", type, file_type_str[i]);
			}
		}
	}

	printf("------------------------\n");
	if(time_print == Time_Start){
		printf("file_fp:%p\n", tf_data.fp);
		printf("folder_path:%s\n", tf_data.folder_path);
		printf("file_name:%s\n", tf_data.file_name);
		printf("start_time:%07d, type:%s(%02d)\n", tf_data.start_time, type, tf_data.type);
	}else if(time_print == Time_End){
		printf("folder_path:%s\n", tf_data.folder_path);
		printf("file_name:%s\n", tf_data.file_name);
		printf("end_time:%07d, type:%s(%02d)\n", tf_data.end_time, type, tf_data.type);
	}
	printf("------------------------\n");
}

void add_file_type(TF_data *tf_data, File_type _file_type)
{
	tf_data->type |= _file_type;
}

static int _check_file(Mode_t mode, const char *_folder, const char *_file_id, FILE **_file_fp, char *_file_path)
{
    DIR *dir = opendir(_folder);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }
    
	struct dirent *entry;
    while((entry = readdir(dir)) != NULL){//遍历文件夹中的文件
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

        if(strncmp(entry->d_name, _file_id, strlen("3600000")) == 0){
            char filepath[PATH_MAX] = {0};
            sprintf(filepath, "%s/%s", _folder, entry->d_name);
			printf("find file name:%s\n", entry->d_name);
			
			if(mode == Mode_DEL){
				if (unlink(filepath) != 0) {
					perror("Unable to delete file");
					closedir(dir);
					return -1;
				}
				printf("File '%s' deleted successfully.\n", filepath);
				return 0;
			}else if(mode == Mode_GET_FP){
				closedir(dir);
				FILE *file = fopen(filepath, "r");
				if(file){
					*_file_fp = file;
					return 0;
				}else{
					return -1;
				}
			}else if(mode == Mode_GET_PATH){
				closedir(dir);
				strcpy(_file_path, filepath);
				return 0;
			}
            
        }
    }
	printf("%sdon't find file name.\n", file_mode_str[mode]);
    closedir(dir);
    return -1;
}

static int folderExists(const char *folder_name) {
    DIR *dir = opendir(folder_name);
    if(dir != NULL){
        closedir(dir);
        return 1;
    } else {
        return 0;
    }
}

static int _check_path(uint64_t _file_id, File_format _file_format, int mode, char *folder_path, int folder_path_len, char *file_id, int file_id_len)
{
	char file_format[16] = {0};
	if(_file_format == FORMAT_MP4){
		strcpy(file_format, "video");
	}else if(_file_format == FORMAT_JPG){
		strcpy(file_format, "pic");
	}

	long file_id_second = _file_id/1000;
    struct tm *local_time = localtime(&file_id_second);
    if(local_time == NULL){
        perror("localtime");
        return -2;
    }

	char _folder_path[32] = {0};
	sprintf(_folder_path, "%d-%02d-%02d/%02d/%s", local_time->tm_year+1900, local_time->tm_mon+1, local_time->tm_mday, local_time->tm_hour, file_format);
	snprintf(folder_path, folder_path_len, "%s/%s", PATH_SDCARD, _folder_path);

	if(mode == 0 && folderExists(folder_path) == 0){
		printf("hour subfile:%s don't exist.\n", folder_path);
		return -1;
	}

	snprintf(file_id, file_id_len, "%07d", (int)(_file_id % (60*60*1000)));
	return 0;
}

static int check_file(Mode_t mode, uint64_t _file_id, File_format _file_format, FILE **_file_fp, char *_file_path)//TODO 换成内部
{
	char folder_path[64] = {0};
	char file_id[16] = {0};
	int ret = _check_path(_file_id, _file_format, 0, folder_path, sizeof(folder_path), file_id,sizeof(file_id));
	if(ret == -1){
		return -1;
	}
	printf("folder_path:%s,file_id:%s\n",folder_path,file_id);

	if(mode == Mode_DEL){
		ret = _check_file(mode, folder_path, file_id, NULL, NULL);
	}else if(mode == Mode_GET_FP){	
		ret = _check_file(mode, folder_path, file_id, _file_fp, NULL);
	}else if(mode == Mode_GET_PATH){
		ret = _check_file(mode, folder_path, file_id, NULL, _file_path);
	}else{
		return -1;
	}

	return ret;
}

int del_file(uint64_t _file_id, File_format _file_format)
{
	int ret = check_file(Mode_DEL, _file_id, _file_format, NULL, NULL);
	return ret;
}

FILE *get_file_fp(uint64_t _file_id, File_format _file_format)
{
	FILE *fp;
	int ret = check_file(Mode_GET_FP, _file_id, _file_format, &fp, NULL);
	if(ret == 0){
		return fp;
	}else{
		return NULL;
	}
}

char *get_file_path(uint64_t _file_id, File_format _file_format)
{
	char *filepath = malloc(PATH_MAX * sizeof(char));
	int ret = check_file(Mode_GET_PATH, _file_id, _file_format, NULL, filepath);
	if(ret == 0){
		return filepath;
	}else{
		free(filepath);
		return NULL;
	}
}

/***************************************************************
* Description:	对文件进行重命名并将文件数据清空
* Parameter:
* @TF_data		文件数据信息
****************************************************************/
void close_file(TF_data *tf_data)
{
	fclose(tf_data->fp);
	tf_data->fp = NULL;

	struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm_info;
    tm_info = localtime(&tv.tv_sec);

	tf_data->end_time = (tm_info->tm_min*60+tm_info->tm_sec)*1000+tv.tv_usec/1000;
	if(tf_data->start_time > tf_data->end_time){
		tf_data->end_time += 60*60*1000;
	}
	
	char original_dir[PATH_MAX];
	if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
        perror("getcwd");
        return ;
    }

    if (chdir(tf_data->folder_path) < 0){
        perror("chdir");
        return;
    }

	char new_name[64] = {0};
	sprintf(new_name, "%07d_%07d_%02d%s", tf_data->start_time, tf_data->end_time, tf_data->type ,tf_data->file_suffix);

    if (rename(tf_data->file_name, new_name) < 0) {
        perror("rename");
        return;
    }else{
		printf("rename success.\n");
	}

	if (chdir(original_dir) != 0) {
        perror("chdir");
        return ;
    }

	strcpy(tf_data->file_name, new_name);
	print_tf_data(*tf_data, Time_End);
	memset(tf_data, 0, sizeof(TF_data));
}

static int mkdir_recursive(const char *path)
{
    int status = mkdir(path, 0777);

    if(status == 0){
        printf("creat folder %s success.\n", path);
        return 1;
    }else{
        if (errno == EEXIST) {
            printf("folder %s already exists.\n", path);
            return 1; 
        } else if (errno == ENOENT) {
            char *parent_path = strdup(path);//获取父文件夹路径
            char *last_slash = strrchr(parent_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                if (!mkdir_recursive(parent_path)) {
                    free(parent_path);
                    return 0;//父文件夹创建失败
                }
            }
            free(parent_path);
            return mkdir_recursive(path);//重新尝试创建文件夹
        } else {
            printf("creat folder %s fail.\n", path);
            return 0;
        }
    }
}

/***************************************************************
* Description:	在本地创建文件
* Parameter:
* @TF_data		文件数据信息
* @File_format	FORMAT_MP4、FORMAT_JPG
* @File_type	创建的文件类型
****************************************************************/
void creat_file(TF_data *tf_data, File_format _file_format, File_type _file_type)
{
	char file_format[16] = {0};

	if(_file_format == FORMAT_MP4){
		strcpy(tf_data->file_suffix, ".mp4");
		strcpy(file_format, "video");
	}else if(_file_format == FORMAT_JPG){
		strcpy(tf_data->file_suffix, ".jpg");
		strcpy(file_format, "pic");
	}

	struct timeval tv;
    gettimeofday(&tv, NULL);
	printf("time:%ld\n", tv.tv_sec*1000+(tv.tv_usec/1000));//将这个时间作为查找去查找文件

    struct tm *tm_info;
    tm_info = localtime(&tv.tv_sec);

	char _folder_path[32] = {0};
	sprintf(_folder_path, "%d-%02d-%02d/%02d/%s", tm_info->tm_year+1900, tm_info->tm_mon+1, tm_info->tm_mday, tm_info->tm_hour, file_format);
	sprintf(tf_data->folder_path, "%s/%s", PATH_SDCARD, _folder_path);
	mkdir_recursive(tf_data->folder_path);
	
	tf_data->type = _file_type;
	tf_data->start_time = (tm_info->tm_min*60+tm_info->tm_sec)*1000+tv.tv_usec/1000;
	sprintf(tf_data->file_name, "%07d_0000000_%02d%s", tf_data->start_time, tf_data->type, tf_data->file_suffix);
	
	char file_name[128] = {0};
	sprintf(file_name, "%s/%s", tf_data->folder_path, tf_data->file_name);
	tf_data->fp = fopen(file_name, "w+");

	print_tf_data(*tf_data, Time_Start);
}

void find_earliest_folder(char *folder_path, char *earliest_folder, int len, Folder_Type type)
{
	DIR *dir;
    struct dirent *entry;

    char _earliest_folder[16] = "";
    char current_folder_name[16];
    int earliest_date_found = 0;

	regex_t regex;
	if(type == DATE_FOLDER){
		const char *pattern = "^[0-9]{4}-[0-9]{2}-[0-9]{2}$";
		if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
			printf("Failed to compile regex pattern\n");
			return;
		}
	}else if(type == HOUR_FOLDER){
		const char *pattern = "^[0-9]{2}$";
		if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
			printf("Failed to compile regex pattern\n");
			return;
		}
	}
    

    dir = opendir(folder_path);
    if (dir == NULL) {
        perror("Unable to open directory");
        return ;
    }

    while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR && regexec(&regex, entry->d_name, 0, NULL, 0) == 0) {
            strncpy(current_folder_name, entry->d_name, sizeof(current_folder_name));

			if (!earliest_date_found || strcmp(current_folder_name, _earliest_folder) < 0) {
			    strcpy(_earliest_folder, current_folder_name);
			    earliest_date_found = 1;
			}
            // printf("%s\n", entry->d_name);
        }
    }
	strncpy(earliest_folder, _earliest_folder, sizeof(_earliest_folder)<=len?sizeof(_earliest_folder):len);

	regfree(&regex);
    closedir(dir);
}

int del_1hours_file(void)
{
	char earliest_folder_date[16];
	char earliest_folder_day[8];
	char folder_path[PATH_MAX];
	char rm_cmd[PATH_MAX];

loop:
	memset(earliest_folder_date, 0, sizeof(earliest_folder_date));
	memset(earliest_folder_day, 0, sizeof(earliest_folder_day));
	memset(folder_path, 0, sizeof(folder_path));
	memset(rm_cmd, 0, sizeof(rm_cmd));

	sprintf(folder_path, "%s", PATH_SDCARD);
	find_earliest_folder(folder_path, earliest_folder_date, sizeof(earliest_folder_date), DATE_FOLDER);
	if(strlen(earliest_folder_date) == 0){
		printf("earliest_date_folder is empty.\n");
		return -1;
	}

	sprintf(folder_path, "%s/%s", folder_path, earliest_folder_date);
	find_earliest_folder(folder_path, earliest_folder_day, sizeof(earliest_folder_day), HOUR_FOLDER);
	if(strlen(earliest_folder_day) == 0){
		printf("earliest_day_folder is empty, del date folder and then del next folder.\n");
		sprintf(rm_cmd, "rm -rf %s", folder_path);
		printf("delete 1hours folder:%s\n", folder_path);
		system(rm_cmd);
		goto loop;
		return -1;
	}
    
    sprintf(folder_path, "%s/%s", folder_path, earliest_folder_day);

	sprintf(rm_cmd, "rm -rf %s", folder_path);
	printf("delete 1hours folder:%s\n", folder_path);
	system(rm_cmd);
    
	return 0;
}

// 比较函数，用于排序文件名数组
int compare_1(const void *a, const void *b)
{
    const char *const *fa = (const char *const *)a;
    const char *const *fb = (const char *const *)b;
    return strcmp(*fa, *fb);
}

int compare_2(const void *a, const void *b)
{
	FileName *fa = (FileName *)a;
	FileName *fb = (FileName *)b;
	return fa->start_second - fb->start_second;
}

int compare_reverse_2(const void *a, const void *b)
{
	FileName *fa = (FileName *)a;
	FileName *fb = (FileName *)b;
	return fb->start_second - fa->start_second;
}

int find_string_index(char *target, char *array[], int array_size, bool *_is_find)
{
	*_is_find = false;
	int index = -1;
    for(int i = 0; i < array_size; i++){//小到大排序查找索引值
		if(strcmp(target, array[i]) == 0){
			index = i;
			*_is_find = true;
			return index;
		}else if(strcmp(target, array[i]) > 0){
			index = i;
		}else if(strcmp(target, array[i]) < 0){
			return index;
		}
    }
	index = array_size;
    return index;
}

int check_subfiles(const char *dirname, char ***path_name, Check_Type check_type)
{
    DIR *dir;
    struct dirent *entry;
	int subfile_count = 0;

	regex_t regex;
	if(check_type == CHECK_DIR){
		if(strcmp(dirname, PATH_SDCARD) == 0){
			const char *pattern = "^[0-9]{4}-[0-9]{2}-[0-9]{2}$";
			if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
				printf("Failed to compile regex pattern\n");
				return -1;
			}
		}else{
			const char *pattern = "^[0-9]{2}$";
			if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
				printf("Failed to compile regex pattern\n");
				return -1;
			}
		}
	}else if(check_type == CHECK_FILE){
		#if LOG_DEBUG
		printf("test1\n");
		#endif
		const char *pattern = "^[0-9]{7}_[0-9]{7}_[0-9]{2}\\.(jpg|mp4)$";
		if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
			printf("Failed to compile regex pattern\n");
			return -1;
		}
	}

    for(int i = 0; i < check_type; i++){
		subfile_count = 0;

		dir = opendir(dirname);
		if(dir == NULL){
			if(i == 1){
				free(*path_name);
			}
			regfree(&regex);
			return -1;
		}

		while((entry = readdir(dir)) != NULL){
			if(regexec(&regex, entry->d_name, 0, NULL, 0) != 0){
				continue;
			}

			if(check_type == CHECK_DIR && entry->d_type == DT_DIR){
				if(i == 0){
					subfile_count++;
				}else if(i == 1){
					(*path_name)[subfile_count++] = strdup(entry->d_name);
				}
			}else if(check_type == CHECK_FILE && entry->d_type == DT_REG){
				subfile_count++;
			}
		}
		closedir(dir);
		if(check_type == CHECK_FILE){break;}
		if(i == 0){
			*path_name = (char **)malloc(subfile_count * sizeof(char *));
		}
	}
	regfree(&regex);
    return subfile_count;
}

int get_dir_info(char *folder_path, int path_offset, int offset_len, char *check_path, int *index, char ***path_name, bool *is_find)
{
	int subfile_count = check_subfiles(check_path, &(*path_name), CHECK_DIR);
	if(subfile_count == -1){
		return -1;
	}

	qsort(*path_name, subfile_count, sizeof(char *), compare_1);

	if(folder_path){
		char src_folder_path[16] = {0};
		strncpy(src_folder_path, folder_path+path_offset, offset_len) ;
		*index = find_string_index(src_folder_path, *path_name, subfile_count, &(*is_find));
	}

	return subfile_count;
}

static cJSON *get_files_info(Find_mode find_mode, File_type _file_type, int num_get, uint64_t start_time, uint64_t end_time)
{
	if(!(_file_type > 0 && _file_type <= ALL_VIDEO) || !(start_time >= 0)){
		printf("_file_type or start_time error\n");
		return NULL;
	}

	if(find_mode != MIDDLE && !(num_get > 0)){
		printf("num_get error\n");
		return NULL;
	}

	if(find_mode == MIDDLE && !(end_time > start_time)){
		printf("end_time error\n");
		return NULL;
	}
#if LOG_DEBUG
	printf("find_mode:%d, start search files info...\n", find_mode);
#endif
	char folder_path[64] = {0};
	char _file_id[16] = {0};
	int ret = _check_path(start_time, FORMAT_MP4, 1, folder_path, sizeof(folder_path), _file_id, sizeof(_file_id));
	if(ret == -2){
		return NULL;
	}
	int file_id = atoi(_file_id);
#if LOG_DEBUG
	printf("    date folder_path:%s, file_id:%d\n", folder_path, file_id);
#endif	
	char end_date[16] = {0};
	char end_hour[8] = {0};
	int end_file_id = 0;
	bool search_over = false;
	if(find_mode == MIDDLE){
		if(!(end_time > 0)){
			printf("end_time error!\n");
			return NULL;
		}
		char end_folder_path[64] = {0};
		char _file_id[16] = {0};
		int ret = _check_path(end_time, FORMAT_MP4, 1, end_folder_path, sizeof(end_folder_path), _file_id, sizeof(_file_id));
		if(ret == -2){
			return NULL;
		}
		memcpy(end_date, end_folder_path+strlen(PATH_SDCARD)+strlen("/"), strlen("xxxx-xx-xx"));
		memcpy(end_hour, end_folder_path+strlen(PATH_SDCARD)+strlen("/")+strlen("xxxx-xx-xx")+strlen("/"), strlen("xx"));
		end_file_id = atoi(_file_id);
#if LOG_DEBUG
		printf("end_date:%s, end_hour:%s, end_file_id:%d\n", end_date, end_hour, end_file_id);
		printf("end_folder_path:%s\n",end_folder_path);
#endif
	}

	int files_info_num = 0;
	bool first_entry = true;
	bool malloc_error = false;

	FileName **files_info = (FileName **)malloc(256 * sizeof(FileName *));
#if LOG_DEBUG
	printf("    1.1 enter root folder:%s\n", PATH_SDCARD);
#endif
	bool is_find;
	date_info.dir_count = get_dir_info(folder_path, strlen(PATH_SDCARD)+strlen("/"), strlen("xxxx-xx-xx"), PATH_SDCARD, &date_info.dir_index, &date_info.dir_name, &is_find);
	if(date_info.dir_count == 0){//日期所在目录为空
#if LOG_DEBUG
		printf("date folder is empty:%s\n", PATH_SDCARD);
#endif
		free(date_info.dir_name);
		free(files_info);
		return NULL;
	}else if(date_info.dir_count == -1){//日期所在目录打不开
		printf("Unable to open root folder:%s\n", PATH_SDCARD);
		free(files_info);
		return NULL;
	}else{
#if LOG_DEBUG
		printf("    ----------------------\n");
		printf("    date_info.dir_name:\n");
		for (int i = 0; i < date_info.dir_count; i++) {
			printf("    %s\n", date_info.dir_name[i]);
		}
		printf("    date_info.dir_index:%d, date_info.dir_count:%d, is_find:%d\n", date_info.dir_index, date_info.dir_count, is_find);
		printf("    ----------------------\n");
#endif
		if(find_mode == FRONT){
			if(date_info.dir_index == -1){//查找的日期文件夹在所有日期文件夹之前，不必查找，立刻退出
				printf("    date folder don't exist and at the top.\n");
				for(int i = 0; i < date_info.dir_count; i++){free(date_info.dir_name[i]);}
				free(date_info.dir_name);
				free(files_info);
				return NULL;
			}
			if(is_find == false){
				if(date_info.dir_index < date_info.dir_count){
					date_info.dir_index++;
				}
				
				goto END;
			}
		}else if(find_mode == BACK || find_mode == MIDDLE){
			if(date_info.dir_index == date_info.dir_count){//查找的日期文件夹在所有日期文件夹之后，不必查找，立刻退出
				printf("    date folder don't exist and at the bottom.\n");
				for(int i = 0; i < date_info.dir_count; i++){free(date_info.dir_name[i]);}
				free(date_info.dir_name);
				free(files_info);
				return NULL;
			}
			if(is_find == false){
				goto END;
			}
		}
	}

	while(1){
		char check_path[128] = {0};
		strncpy(check_path, folder_path, strlen(PATH_SDCARD)+strlen("/")+strlen("xxxx-xx-xx"));//日期目录
		
		if(first_entry){
#if LOG_DEBUG
			printf("        2.1.1 enter date folder:%s\n", check_path);
#endif
			hour_info.dir_count = get_dir_info(folder_path, strlen(PATH_SDCARD)+strlen("/")+strlen("xxxx-xx-xx")+strlen("/"), strlen("xx"), check_path, &hour_info.dir_index, &hour_info.dir_name, &is_find);
#if LOG_DEBUG			
			printf("        ----------------------\n");
			printf("        hour_info.dir_name:\n");
			for (int i = 0; i < hour_info.dir_count; i++) {
				printf("        %s\n", hour_info.dir_name[i]);
			}
			printf("        hour_info.dir_index:%d, hour_info.dir_count:%d\n", hour_info.dir_index, hour_info.dir_count);
			printf("        ----------------------\n");
#endif
			if(find_mode == FRONT){
				if(hour_info.dir_index == -1){//查找的小时文件夹在所有小时文件夹之前，找日期文件夹
					hour_info.dir_index = 0;//防止跳到调整日期判断出错
					goto END_date;
				}
			}else if(find_mode == BACK){
				if(hour_info.dir_index == hour_info.dir_count){//查找的小时文件夹在所有小时文件夹之后，找日期文件夹
					hour_info.dir_index = hour_info.dir_count - 1;//防止跳到调整日期判断出错
					goto END_date;
				}else if(hour_info.dir_index == -1){//查找的小时文件夹在所有小时文件夹之前，找下一个小时文件夹=====
					goto END_hour;
				}
				
				
			}else if(find_mode == MIDDLE){
				if(hour_info.dir_index == hour_info.dir_count){//查找的小时文件夹在所有小时文件夹之后，找日期文件夹
					hour_info.dir_index = hour_info.dir_count - 1;//防止跳到调整日期判断出错
					goto END_date;
				}else if(hour_info.dir_index == -1){//查找的小时文件夹在所有小时文件夹之前，找下一个小时文件夹=====
					goto END_hour;
				}

				if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0){
					if(strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) > 0){
						goto END_date;
					}
				}
			}
		}else{
			#if LOG_DEBUG
			printf("        2.1.2 enter date folder:%s\n", check_path);
			#endif
			hour_info.dir_count = get_dir_info(NULL, strlen(PATH_SDCARD)+strlen("/")+strlen("xxxx-xx-xx")+strlen("/"), strlen("xx"), check_path, &hour_info.dir_index, &hour_info.dir_name, &is_find);
			#if LOG_DEBUG
			printf("        ----------------------\n");
			printf("        hour_info.dir_name:\n");
			for (int i = 0; i < hour_info.dir_count; i++) {
				printf("        %s\n", hour_info.dir_name[i]);
			}
			printf("        hour_info.dir_index:%d, hour_info.dir_count:%d\n", hour_info.dir_index, hour_info.dir_count);
			printf("        ----------------------\n");
			#endif
			char *_date = folder_path+strlen(PATH_SDCARD)+strlen("/")+strlen("xxxx-xx-xx")+strlen("/");
			if(hour_info.dir_count > 0){
				if(find_mode == FRONT){
					hour_info.dir_index = hour_info.dir_count - 1;//小时往前找
					memcpy(_date, hour_info.dir_name[hour_info.dir_index], strlen(hour_info.dir_name[hour_info.dir_index]));
				}else if(find_mode == BACK || find_mode == MIDDLE){
					hour_info.dir_index = 0;//小时往后找
					memcpy(_date, hour_info.dir_name[hour_info.dir_index], strlen(hour_info.dir_name[hour_info.dir_index]));
				}
			}
		}

		if(hour_info.dir_count == 0){//小时文件夹为空，找日期文件夹
			printf("        hour folder is empty:%s, check date folder.\n", folder_path);
			goto END_date;

		}else if(hour_info.dir_count == -1){//小时文件夹不存在，往前或后找小时文件夹
			printf("        hour folder is not exist:%s, check date folder.\n", folder_path);
			goto END_hour;

		}

		while(1){
			#if LOG_DEBUG
			printf("            3.1 start check files......\n");
			#endif
			int file_count = check_subfiles(folder_path, NULL, CHECK_FILE);
			if(file_count == -1 || file_count == 0){
				printf("            %s is not exist or empty.\n", folder_path);
				goto END_hour;
			}

			FileName *files = (FileName *)malloc(file_count * sizeof(FileName));
			memset(files, 0, file_count * sizeof(FileName));

			DIR *dir = opendir(folder_path);
			if (dir == NULL) {
				printf("            opendir fail:%s\n", folder_path);
				for(int i = 0; i < hour_info.dir_count; i++){free(hour_info.dir_name[i]);}
				free(hour_info.dir_name);
				for(int i = 0; i < date_info.dir_count; i++){free(date_info.dir_name[i]);}
				free(date_info.dir_name);
				free(files_info);
				free(files);
				return NULL;
			}

			char folder_prefixes[32] = {0};
			strcpy(folder_prefixes, folder_path+strlen(PATH_SDCARD)+strlen("/"));

			struct dirent *entry;
			int files_num = 0;
			while((entry = readdir(dir)) != NULL){
				if(entry->d_type == DT_REG){//只处理普通文件
					sscanf(entry->d_name, "%07d_%07d_%02d.", &files[files_num].start_second, &files[files_num].end_second, &files[files_num].type);
					memcpy(files[files_num].folder_prefixes, folder_prefixes, strlen(folder_prefixes));
					files[files_num].folder_prefixes[strlen(folder_prefixes)] = 0;
					#if LOG_DEBUG
					printf("            %s/%07d_%07d_%02d\n", files[files_num].folder_prefixes, files[files_num].start_second, files[files_num].end_second, files[files_num].type);
					#endif
					files_num++;
					if(files_num == file_count){break;}
				}
			}
			closedir(dir);

			if(find_mode == FRONT){
				// 对文件名数组按照起始秒数进行排序
				qsort(files, files_num, sizeof(FileName), compare_reverse_2);
			}else if(find_mode == BACK || find_mode == MIDDLE){
				qsort(files, files_num, sizeof(FileName), compare_2);
			}
			
			if(first_entry){
				for(int i = 0; i < files_num; i++){
					if(files[i].end_second == 0){continue;}

					if(find_mode == FRONT && files[i].start_second <= file_id){
						if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
							files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
							if (files_info[files_info_num] == NULL) {
								printf("%d:malloc error\n", __LINE__);
								malloc_error = true;//释放所有内存
								break;
							}

							memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
							files_info[files_info_num]->start_second = files[i].start_second;
							files_info[files_info_num]->end_second = files[i].end_second;
							files_info[files_info_num]->type = files[i].type;
							files_info_num++;
							if(files_info_num == num_get){break;}
						}
					}else if(find_mode == BACK && files[i].start_second >= file_id){
						if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
							files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
							if (files_info[files_info_num] == NULL) {
								printf("%d:malloc error\n", __LINE__);
								malloc_error = true;//释放所有内存
								break;
							}

							memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
							files_info[files_info_num]->start_second = files[i].start_second;
							files_info[files_info_num]->end_second = files[i].end_second;
							files_info[files_info_num]->type = files[i].type;
							files_info_num++;
							if(files_info_num == num_get){break;}
						}
					}else if(find_mode == MIDDLE){
						if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0 &&
						strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) == 0){
							if(files[i].start_second >= file_id && files[i].start_second <= end_file_id){
								if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
									files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
									if (files_info[files_info_num] == NULL) {
										printf("%d:malloc error\n", __LINE__);
										malloc_error = true;//释放所有内存
										break;
									}

									memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
									files_info[files_info_num]->start_second = files[i].start_second;
									files_info[files_info_num]->end_second = files[i].end_second;
									files_info[files_info_num]->type = files[i].type;
									files_info_num++;
								}
							}else if(files[i].start_second > end_file_id){
								search_over = true;
							}
						}else if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0 &&
						strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) > 0){//检测下一个日期文件夹发现里面的第一个小时文件夹比要结束的小时文件夹要后的话，立刻退出
							printf("            1.hour_info large end_hour!\n");
							break;
						}else{
							if(files[i].start_second >= file_id && (_file_type == ALL_VIDEO || (_file_type & files[i].type))){
								files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
								if (files_info[files_info_num] == NULL) {
									printf("%d:malloc error\n", __LINE__);
									malloc_error = true;//释放所有内存
									break;
								}

								memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
								files_info[files_info_num]->start_second = files[i].start_second;
								files_info[files_info_num]->end_second = files[i].end_second;
								files_info[files_info_num]->type = files[i].type;
								files_info_num++;
							}
						}
					}
				}
			}else{
				if(find_mode == MIDDLE){
					for(int i = 0; i < files_num; i++){
						if(files[i].end_second == 0){continue;}

						if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0 &&
						strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) == 0){
							if(files[i].start_second <= end_file_id){
								if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
									files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
									if (files_info[files_info_num] == NULL) {
										printf("%d:malloc error\n", __LINE__);
										malloc_error = true;//释放所有内存
										break;
									}

									memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
									files_info[files_info_num]->start_second = files[i].start_second;
									files_info[files_info_num]->end_second = files[i].end_second;
									files_info[files_info_num]->type = files[i].type;
									files_info_num++;
								}
							}else if(files[i].start_second > end_file_id){
								search_over = true;
							}
						}else if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0 &&
						strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) > 0){//检测下一个日期文件夹发现里面的第一个小时文件夹比要结束的小时文件夹要后的话，立刻退出
							printf("            2.hour_info large end_hour!\n");
							break;
						}else{
							if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
								files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
								if (files_info[files_info_num] == NULL) {
									printf("%d:malloc error\n", __LINE__);
									malloc_error = true;//释放所有内存
									break;
								}

								memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
								files_info[files_info_num]->start_second = files[i].start_second;
								files_info[files_info_num]->end_second = files[i].end_second;
								files_info[files_info_num]->type = files[i].type;
								files_info_num++;
							}
						}
					}
				}else{
					for(int i = 0; i < files_num; i++){
						if(files[i].end_second == 0){continue;}

						if(_file_type == ALL_VIDEO || (_file_type & files[i].type)){
							files_info[files_info_num] = (FileName *)malloc(sizeof(FileName));
							if (files_info[files_info_num] == NULL) {
								printf("%d:malloc error\n", __LINE__);
								malloc_error = true;//释放所有内存
								break;
							}

							memcpy(files_info[files_info_num]->folder_prefixes, files[i].folder_prefixes, sizeof(files[i].folder_prefixes));
							files_info[files_info_num]->start_second = files[i].start_second;
							files_info[files_info_num]->end_second = files[i].end_second;
							files_info[files_info_num]->type = files[i].type;
							files_info_num++;
							if(files_info_num == num_get){break;}
						}
					}
				}
			}
			
			free(files);
		END_hour:
		#if LOG_DEBUG
			printf("            3.2 end check files......\n");
		#endif
			first_entry = false;
			if(malloc_error){break;}//内存申请失败
			if(find_mode != MIDDLE && files_info_num == num_get){break;}

			if(find_mode == FRONT){
				hour_info.dir_index--;
				if(hour_info.dir_index == -1){break;}
			}else if(find_mode == BACK){
				hour_info.dir_index++;
				if(hour_info.dir_index == hour_info.dir_count){break;}
			}else if(find_mode == MIDDLE){
				hour_info.dir_index++;
				if(hour_info.dir_index == hour_info.dir_count){break;}//达到最大小时数
				
				if(search_over){break;}//同个日期同个小时内触发
				#if LOG_DEBUG
				printf("            compare date:%s<=%s, compare hour:%s<=%s\n",date_info.dir_name[date_info.dir_index], end_date, hour_info.dir_name[hour_info.dir_index], end_hour);
				#endif
				if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index])) == 0 &&
				strncmp(hour_info.dir_name[hour_info.dir_index], end_hour, strlen(hour_info.dir_name[hour_info.dir_index])) > 0){
					printf("            3.hour_info large end_hour!\n");
					break;
				}
			}

			char *ptr_date = folder_path+strlen(PATH_SDCARD)+strlen("/");
			char *date = ptr_date+11;
			memcpy(date, hour_info.dir_name[hour_info.dir_index], strlen(hour_info.dir_name[hour_info.dir_index]));
		}

	END_date:
		for(int i = 0; i < hour_info.dir_count; i++){free(hour_info.dir_name[i]);}
		free(hour_info.dir_name);
	END:
	#if LOG_DEBUG
		printf("        2.2 exit date folder:%s\n", folder_path);
	#endif
		first_entry = false;
		if(malloc_error){break;}//内存申请失败
		if(find_mode != MIDDLE && files_info_num == num_get){break;}

		if(find_mode == FRONT){
			date_info.dir_index--;
			if(date_info.dir_index == -1){break;}
		}else if(find_mode == BACK){
			date_info.dir_index++;
			if(date_info.dir_index == date_info.dir_count){break;}
		}else if(find_mode == MIDDLE){
			date_info.dir_index++;
			if(date_info.dir_index == date_info.dir_count){break;}//达到最大日期数
			
			if(search_over){break;}//同个日期同个小时内触发

			if(strncmp(date_info.dir_name[date_info.dir_index], end_date, strlen(date_info.dir_name[date_info.dir_index]))> 0){
				printf("        date_info large end_date!\n");
				break;
			}
		}

		char *date = folder_path+strlen(PATH_SDCARD)+strlen("/");
		memcpy(date, date_info.dir_name[date_info.dir_index], strlen(date_info.dir_name[date_info.dir_index]));
	}
	
	for(int i = 0; i < date_info.dir_count; i++){free(date_info.dir_name[i]);}
	free(date_info.dir_name);

	if(find_mode == MIDDLE){
		#if LOG_DEBUG
		printf("    search_over:%d\n", search_over);
		#endif
	}

	cJSON *file_info_array = cJSON_CreateArray();

	struct tm timeinfo = {0};
	char str_copy[8];
	#if LOG_DEBUG
	printf("    4.1 search find files:\n");
	#endif
	for(int i = 0; i < files_info_num; i++){//id type end即可
	#if LOG_DEBUG
		printf("    path:%s,start:%07d,end:%07d,type:%02d\n", files_info[i]->folder_prefixes, files_info[i]->start_second,files_info[i]->end_second,files_info[i]->type);
	#endif
		if(files_info[i]->end_second != 0){
			memset(str_copy, 0, sizeof(str_copy));
			strncpy(str_copy, files_info[i]->folder_prefixes, 4);
			timeinfo.tm_year = atoi(str_copy) - 1900;

			memset(str_copy, 0, sizeof(str_copy));
			strncpy(str_copy, files_info[i]->folder_prefixes+strlen("xxxx-"), 2);
			timeinfo.tm_mon = atoi(str_copy) - 1;

			memset(str_copy, 0, sizeof(str_copy));
			strncpy(str_copy, files_info[i]->folder_prefixes+strlen("xxxx-xx-"), 2);
			timeinfo.tm_mday = atoi(str_copy);

			memset(str_copy, 0, sizeof(str_copy));
			strncpy(str_copy, files_info[i]->folder_prefixes+strlen("xxxx-xx-xx/"), 2);
			timeinfo.tm_hour = atoi(str_copy);

			timeinfo.tm_min = files_info[i]->start_second/1000/60;
			timeinfo.tm_sec = files_info[i]->start_second/1000%60;

			uint64_t timestamp_start = mktime(&timeinfo);
			if (timestamp_start == -1) {
				perror("mktime");
				goto ERR;
			}
			timestamp_start = timestamp_start*1000 + files_info[i]->start_second%1000;

			timeinfo.tm_min = files_info[i]->end_second/1000/60;
			timeinfo.tm_sec = files_info[i]->end_second/1000%60;

			uint64_t timestamp_end = mktime(&timeinfo);
			if (timestamp_end == -1) {
				perror("mktime");
				goto ERR;
			}
			timestamp_end = timestamp_end*1000 + files_info[i]->end_second%1000;

			cJSON *file_info_obj = cJSON_CreateObject();
			cJSON_AddItemToObject(file_info_obj, "id", cJSON_CreateNumber(timestamp_start));
			cJSON_AddItemToObject(file_info_obj, "type", cJSON_CreateNumber(files_info[i]->type));
			cJSON_AddItemToObject(file_info_obj, "end", cJSON_CreateNumber(timestamp_end));
			cJSON_AddItemToArray(file_info_array, file_info_obj);
		}
		ERR:
		free(files_info[i]);
	}
	free(files_info);
#if LOG_DEBUG
	char *file_info_str = cJSON_PrintUnformatted(file_info_array);
	if(file_info_str){
		printf("------------file_info_str:\n%s\n------------\n",file_info_str);
		free(file_info_str);
	}
	
	printf("end search files info...\n");
#endif
	return file_info_array;
}

cJSON *get_files_info_front(File_type _file_type, int num_get, uint64_t start_time)
{
	return get_files_info(FRONT, _file_type, num_get, start_time, 0);
}

cJSON *get_files_info_back(File_type _file_type, int num_get, uint64_t start_time)
{
	return get_files_info(BACK, _file_type, num_get, start_time, 0);
}

cJSON *get_files_info_middle(File_type _file_type, uint64_t start_time, uint64_t end_time)
{
	return get_files_info(MIDDLE, _file_type, 0, start_time, end_time);
}