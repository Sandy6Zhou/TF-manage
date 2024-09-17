#include "TF_card_manage.h"

int main() {
	static TF_data tf_data;
	char test[100] = {"test1"};
/* ----------------------------- */
	// creat_file(&tf_data, FORMAT_JPG, FULLTIME);
	// for(int i=0;i<5;i++){
	// 	fwrite(test, 1, strlen(test), tf_data.fp);
	// 	if(i==2){add_file_type(&tf_data, MOTION);}
	// 	if(i==4){add_file_type(&tf_data, DEVICE);}
	// 	sleep(1);
	// }
	// close_file(&tf_data);
/* ----------------------------- */
	// creat_file(&tf_data, FORMAT_MP4, FULLTIME);
	// for(int i=0;i<5;i++){
	// 	fwrite(test, 1, strlen(test), tf_data.fp);
	// 	if(i==2){add_file_type(&tf_data, FULLTIME);}
	// 	if(i==4){add_file_type(&tf_data, DEVICE);}
	// 	if(i==5){add_file_type(&tf_data, HUMAN);}
	// 	sleep(1);
	// }
	// close_file(&tf_data);
/* ----------------------------- */
	// FILE *fp = get_file_fp(1726565516834, FORMAT_JPG);
	// printf("fp:%p\n", fp);
	// if(fp==NULL){
	// 	return -1;
	// }

	// char *path = get_file_path(1726565621293, FORMAT_MP4);
	// printf("path:%s\n", path);
	// free(path);

	// int ret = del_file(1726565516834, FORMAT_JPG);
	// printf("ret:%d\n",ret);
/* ----------------------------- */
	// del_1hours_file();
/* ----------------------------- */
	// get_files_info_front(ALL_VIDEO, 15, 1726565621293);
	// get_files_info_back((FULLTIME | TIMING | MOTION | HUMAN | DEVICE), 1, 1726565621293);
	// get_files_info_middle(MOTION|DEVICE, 1726565621293, 1726565699293);

    return 0;
}
