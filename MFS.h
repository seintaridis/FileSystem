#include "MDS.h"

int mfs_create(char* filename,int block_size, int filename_size, int file_size, int directory_size);
int mfs_import(char* filename, char* path);
void mfs_getsuper(char* filename,SUPER* retrn);
void mfs_updatesuper(char* filename,SUPER* super);
void mfs_getmds(char* filename, MDS* mds, int path);
void mfs_updateMDS(char* filename,int path ,MDS* mds,int size_of_block);
int mfs_mkdir(char* filename,char* path);
void mfs_ls(char* filename,int x,int y,int z,int k,int n);
void mfs_touch(char* filename,char* file, int a, int m);
void mfs_export(char* filename, char* file,char* directory);
int mfs_create_tripleta(char* mfsfile,char* filename,int nodeid,int blockid,int path);
void mfs_parsepath(char*path,char* ret);
void my_cd(char* filename,char*path);
void mfs_cd(char* filename,char*path);
int search_folder(char*filename,MDS* mds,SUPER* retrieved_super,char* folder_name);
void mfs_pwd(char* filename);