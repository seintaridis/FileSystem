#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include "MFS.h"

int main(void)
{
  


/* local variable definition */
	char command[128];
    char * pch;
	char argument[100];
    char arguments[1024];
    char path[1024];
    char mfs_file[128];
    char args[100][128];
    int i;
    char directory[128];
    char *p=getenv("USER");
    if(p==NULL) return EXIT_FAILURE;
    strcpy(directory,"/home/");
    strcat(directory, p);
    strcat(directory,"/Desktop");
    
    /*Bash Get's commands until exit command is given*/
    while(1)
    {
        for(i=0; i<100; i++){
            strcpy(args[i], "null");
        }
        strcpy(path,"");
        fgets(arguments,1000,stdin);
        size_t ln = strlen(arguments) - 1;
        if (arguments[ln] == '\n') arguments[ln] = '\0';
        i=0;
        pch = strtok (arguments," -");
        strcpy(command,pch);
        while (pch != NULL)
        {
            if(strcmp(pch,command)!=0){
                strcpy(args[i],pch);
                i++;
            }
            pch = strtok (NULL, " -");

        }
        strcpy(argument,args[i-1]);
        /*Executing Commands*/
		if (strcmp(command,"mfs_create")==0)
		{
            int block_size=2048;
            int filename_size=16;
            int file_size=0;
            int directory_size=25;
            i=0;
            while(strcmp(args[i],argument)!=0){
                if(strcmp(args[i],"bs")==0){
                    block_size=atoi(args[i+1]);
                    i++;
                    continue;
                }
                if(strcmp(args[i],"fns")==0){
                    filename_size=atoi(args[i+1]);
                    i++;
                    continue;
                }
                if(strcmp(args[i],"mfs")==0){
                    file_size=atoi(args[i+1]);
                    i++;
                    continue;
                }
                if(strcmp(args[i],"mdfn")==0){
                    directory_size=atoi(args[i+1]);
                    i++;
                    continue;
                }
                i++;
            }
            //printf("Calling create with:\nbs: %d\nfns: %d\nmfs: %d\nmdfn: %d\n",block_size,filename_size,file_size,directory_size);
			strcpy(mfs_file,argument);
			mfs_create(mfs_file, block_size, filename_size, file_size, directory_size);
			strcpy(path,"/");
			

		}
        else if (strcmp(command,"mfs_workwith")==0)
        {
            strcpy(mfs_file,argument);
            
            
        }
        else if (strcmp(command,"mfs_export")==0)
        {
            if(strcmp(args[1],"null") != 0) strcpy(directory,args[1]);
            strcpy(argument,args[0]);
            mfs_export(mfs_file,argument,directory);
            strcpy(directory,"/Users/Panos/Desktop"); //CHANGE TO CURRENT OS DESKTOP
            
        }
        else if (strcmp(command,"mfs_import")==0)
        {

            mfs_import(mfs_file, argument);
            
            
        }
        else if(strcmp(command,"mfs_mkdir")==0)
        {
            mfs_mkdir(mfs_file,argument);
        }
        else if (strcmp(command,"mfs_ls")==0)
        {
            i=0;
            int a=0;
            int l=0;
            int r=0;
            int U=0;
            int d=0;
            while(strcmp(args[i],"null")!=0){
		printf("%d %s \n",i,args[i]);
                if(strcmp(args[i],"a")==0){
                    a=1;
                    i++;
                    continue;
                }
                if(strcmp(args[i],"r")==0){
                    r=1;
                    i++;
                    continue;
                }
                if(strcmp(args[i],"l")==0){
                    l=1;
                    i++;
                    continue;
                }
                if(strcmp(args[i],"U")==0){
                    U=1;
                    i++;
                    continue;
                }
                if(strcmp(args[i],"d")==0){
                    d=1;
                    i++;
                    continue;
                }
            }
            //printf("Calling ls for %s a: %d  m:%d\n",a,r,l,U,d);
            mfs_ls(mfs_file,a,r,l,U,d);
        }
        else if (strcmp(command,"mfs_cd")==0)
        {
            mfs_cd(mfs_file,argument);
        }
        else if (strcmp(command,"mfs_pwd")==0)
        {
            mfs_pwd(mfs_file);
        }
        else if (strcmp(command,"mfs_touch")==0)
        {
            i=0;
            int a=0;
            int m=0;
            while(strcmp(args[i],argument)!=0){
                if(strcmp(args[i],"m")==0){
                    m=1;
                    i++;
                    continue;
                }
                if(strcmp(args[i],"a")==0){
                    a=1;
                    i++;
                    continue;
                }
            }
            //printf("Calling touch for %s a: %d  m:%d\n",argument,a,m);
            mfs_touch(mfs_file,argument, a, m);
        }
        else if (strcmp(command,"exit")==0)
        {
            SUPER* super=malloc(sizeof(SUPER));
            mfs_getsuper(mfs_file, super);
            printf("Goodbye %s!\nData of fs on shut down:\n blocks:%d\n files:%d\n last inode:%d\n max_size:%d\n",p,super->block_count,super->files_count,super->last_inode,super->max_file_size);
            free(super);
            return 0;
        }
        else{
            printf("Command not found!\n");
        }
        printf("%s: ",path);
	}
}