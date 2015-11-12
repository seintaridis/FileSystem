#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "MFS.h"

#define BLOCK_SIZE 2048

int mfs_create(char* filename,int block_size, int filename_size, int file_size, int directory_size)
{
    int i;
    //printf("I am create with:\nbs: %d\nfns: %d\nmfs: %d\nmdfn: %d\n",block_size,filename_size,file_size,directory_size);

    struct tm * timeinfo;
    /*Creating super block*/
    SUPER* super;
    super = malloc(sizeof(SUPER));
    super->path=1;
    super->block_count=2;
    super->files_count=1;
    super->block_size=block_size;
    super->block_stream_capacity=(super->block_size)/sizeof(STREAM);
    super->last_inode=0;
    super->max_file_size=file_size;
    super->max_directory_size=directory_size;
    //printf("A block can handle %zu streams!\n",super->block_size/sizeof(STREAM));
    
    
    /*Writing super block on the first block of .mfs file*/
    FILE* filedesc = fopen(filename,"w+");
    if (filedesc == NULL)
    {
        perror(filename); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    if ( fwrite(super,sizeof(SUPER),1,filedesc) == -1) {
        perror("writing super");
        exit(1);
    }
    fseek(filedesc,(super->block_size)+1,SEEK_SET);//Write on the next block
    
    /*Creating root mds*/
    MDS* mds;
    mds = malloc(sizeof(MDS));
    mds->nodeid=0;
    mds->count=0; //count files of folder
    strcpy(mds->filename,"/");
    mds->size=0;
    mds->type=0;
    mds->parent_nodeid=0;
    time( &mds->creation_time);
    time( &mds->access_time);
    time( &mds->modification_time);
    timeinfo = localtime (&mds->creation_time);
    mds->data.next_empty=0;
    for(i=0; i<13; i++){
        mds->data.datablocks[i]=-1;
    }
    /*Writing mds on file*/
    if ( fwrite(mds,sizeof(MDS),1,filedesc) == -1) {
        perror("writing node");
        exit(1);
    }

    /*Clearing out*/
    if ( fclose( filedesc ) == -1 ) {
        perror("closing file");
        exit(1);
    }
    
    free(mds);
    free(super);
    return 0;
}

void mfs_getsuper(char* filename,SUPER* retrn){
    /*Function returns super block of a file (filename) and returns it on pre-allocated retrn*/
    FILE* filedesc = fopen(filename,"r+");
    if (filedesc == NULL)
    {
        perror("getsuper open"); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    SUPER* retrieved_super;
    retrieved_super = malloc(sizeof(SUPER));
    if ( fread(retrieved_super, sizeof(SUPER),1,filedesc) == -1  ) {
        perror("read");
        exit(1);
    }
    memcpy(retrn, retrieved_super, sizeof(SUPER));
    //printf("mfs_getsuper: %d %d\n",retrieved_super->block_size,retrieved_super->block_count);
    free(retrieved_super);
    fclose(filedesc);
    return;
    
}

void mfs_updatesuper(char* filename,SUPER* super){
    /*Updates the super block with the data found on super var */
    //printf("mfs_updatesuper: updates super with %d blocks and %d size\n",super->block_count,super->block_size);
    FILE* filedesc = fopen(filename,"r+");
    if (filedesc == NULL)
    {
        perror("updatesuper open"); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    if ( fwrite(super,sizeof(SUPER),1,filedesc) == -1) {
        perror("updatesuper write");
        exit(1);
    }
    fclose(filedesc);
    return;
}


void mfs_getmds(char* filename, MDS* mds, int path){
    /*Returns the mds of a file in block #path (starting from zero) */
    MDS* temp_mds;
    temp_mds = malloc(sizeof(MDS));
    SUPER* super;
    super=malloc(sizeof(SUPER));
    mfs_getsuper(filename, super);
    FILE* filedesc = fopen(filename,"r");
    if (filedesc == NULL)
    {
        perror("getmds open"); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    fseek(filedesc, ((super->block_size)*path)+1, SEEK_SET);
    //printf("mfs_getmds: Reading from %d (%lu)\n",((super->block_size)*path)+1,ftell(filedesc));
    if ( fread(temp_mds, sizeof(MDS),1,filedesc) == -1  ) {
        perror("read");
        exit(1);
    }
    //  printf("mfs_getmds: id: %d -- name: %s \n", temp_mds->nodeid, temp_mds->filename);
    if ( fclose( filedesc ) == -1 ) {
        perror("closing file");
        exit(1);
    }
    memcpy(mds,temp_mds,sizeof(MDS));
    free(super);
    free(temp_mds);
}

void mfs_updateMDS(char* filename,int path ,MDS* mds,int size_of_block){
    /*Updates an mds with the data found on mds*/
    //printf("mfs_update: %s %d %d\n",filename,path,size_of_block);
    FILE* filedesc = fopen(filename,"r+");
    if (filedesc == NULL)
    {
        perror("updatemds open"); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    int offset = (path*size_of_block)+1;
    fseek(filedesc,(path*size_of_block)+1,SEEK_SET);//Write on the next block
    if ( fwrite(mds,sizeof(MDS),1,filedesc) == -1) {
        perror("updatemds write");
        exit(1);
    }
    fclose(filedesc);
    return;
}



int mfs_mkdir(char*filename,char*path)
{
   
    /*Get super block*/
    SUPER* retrieved_super = malloc(sizeof(SUPER));;

    
    mfs_getsuper(filename,retrieved_super);
    retrieved_super->last_inode++;
    mfs_updatesuper(filename,retrieved_super);
    printf("Creating directory: %s\n",path);
    /*Creates a tripleta in fathers directory for the new directory*/
    if(mfs_create_tripleta(filename,path,retrieved_super->last_inode,retrieved_super->block_count,retrieved_super->path)!=0){
        retrieved_super->last_inode--;
        mfs_updatesuper(filename,retrieved_super);
        free(retrieved_super);
        return 0;
    }
    mfs_getsuper(filename,retrieved_super);
    retrieved_super->files_count++;
    
    
    
    
    /*Creating the mds of the new directory*/
    int i;
    MDS* mds;
    MDS* mds2;
    mds = malloc(sizeof(MDS));
    mds2 = malloc(sizeof(MDS));
    mfs_getmds(filename, mds2, retrieved_super->path);
    mds->nodeid=retrieved_super->last_inode;      //to inode t neou fakelou
    mds->count=0;
    strcpy(mds->filename,path);
    mds->size=0;
    mds->type=0;
    mds->parent_block=retrieved_super->path;
    mds->parent_nodeid=mds2->nodeid;
    time( &mds->creation_time);
    time( &mds->access_time);
    time( &mds->modification_time);
    mds->data.next_empty=0;
    for(i=0; i<13; i++){
        mds->data.datablocks[i]=-1;
    }
    free(mds2);
    
    /*Write mds on .mfs file*/
    mfs_updateMDS(filename,retrieved_super->block_count,mds,retrieved_super->block_size);//kanw update to neo mds
    
    retrieved_super->block_count++;
    
    /*Update super block*/
    mfs_updatesuper(filename,retrieved_super);

    /*Clean Up*/
    free(mds);
    free(retrieved_super);
    

    
    return 0;
}


void mfs_ls(char* filename,int a,int r,int l,int U,int d)
{
	char buf[120]="";
	printf("to a einai %d\n",a);
    struct tm * timeinfo;
    SUPER* super=malloc(sizeof(SUPER));
    MDS* mds=malloc(sizeof(MDS));
    FILE* filedesc = fopen(filename,"r+");
    STREAM* tripleta=malloc(sizeof(STREAM));
    char buffer[120]="";
    mfs_getsuper(filename,super);
    printf("to super path stin ls einai %d \n",super->path);
    mfs_getmds(filename,mds,super->path);
    fseek(filedesc,mds->data.datablocks[mds->data.next_empty]*(super->block_size)+1,SEEK_SET);
    
    printf("Reading tripletas from %lu\n",ftell(filedesc));
    
    int i;
    for (i=0;i<mds->count;i++)
    {
        if ( fread(tripleta, sizeof(STREAM),1,filedesc) == -1  )
        {
            perror("read");
            exit(1);
        }
        if((a==0 && r==0 && l==0 && U==0 && d==0)||(U==1))
        {
        	strcpy(buffer,tripleta->filename);
        	if (buffer[0]!='.')
        		printf("%s\n",tripleta->filename);

        }
        else if (a==1)
        {
            strcpy(buffer,tripleta->filename);
            printf("%s\n",buffer);
        }
        else if (l==1)
    	{
    		strcpy(buffer,tripleta->filename);
        	if (buffer[0]!='.')
        	{
        		//printf("--------------------------------------------\n");
            	MDS* retrieved_mds=malloc(sizeof(MDS));
            	mfs_getmds(filename,retrieved_mds,tripleta->blocknumber);
				printf("%s\n",retrieved_mds->filename);
            	timeinfo = localtime(&retrieved_mds->creation_time);
            	printf("creation time: %s",asctime(timeinfo));
            	timeinfo = localtime(&retrieved_mds->access_time);
            	printf("access time: %s",asctime(timeinfo));
            	timeinfo = localtime(&retrieved_mds->modification_time);
            	printf("modification time: %s",asctime(timeinfo));
            	if (retrieved_mds->type==0)
            		printf("size is %d\n",retrieved_mds->count);
            	else	
            		printf("size is %d\n",retrieved_mds->size);
            	printf("--------------------------------------------\n");
  			}
    	}
    	else if (d==1)
    	{
    		MDS* retrieved_mds=malloc(sizeof(MDS));
            mfs_getmds(filename,retrieved_mds,tripleta->blocknumber);
            if (retrieved_mds->type==0)
            	printf("%s\n",retrieved_mds->filename);

    	}
    	else if (r==1)
    	{
    		strcpy(buffer,tripleta->filename);
    		strcat(buffer,"\n");
        	if (buffer[0]!='.')
        		{strcat(buffer,buf);
        		
        		strcpy(buf,buffer);

        	}
        		//printf("%s\n",tripleta->filename);

    	}
    	/*while(strcmp(checker,"/")!=0)
    {
        strcpy(path1,checker);
        mfs_getmds(filename,mds,mds->parent_block);
        strcpy(checker,mds->filename);
        if (i!=0)
            strcat(path1,"/");
        i=1;
        strcat(path1,path);
        strcpy(path,path1);
    }
    strcpy(path1,"/");
    strcat(path1,path);
    strcpy(path,path1);*/
    
    }
    if (r==1)
    	printf("%s\n",buf);
    free(super);
    free(mds);
    
}

void mfs_export(char* filename, char* file,char* directory){
    /*Export file from current dir to directory of the main fs*/
    SUPER* super=malloc(sizeof(SUPER));
    MDS* mds=malloc(sizeof(MDS));
    FILE* filedesc = fopen(filename,"r+");
    if (filedesc == NULL)
    {
        perror("ls open"); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    /*Get super block*/
    mfs_getsuper(filename,super);
    char buffer[super->block_size]; //Create buffs for transferin a max of super->block_size sized blocks
    rewind(filedesc);
    fseek(filedesc,8193,SEEK_SET);
    fread(buffer,1,733,filedesc);
    rewind(filedesc);
    /*Check all tripletas to find the one coresponding to the file we wish to export*/
    STREAM* tripleta=malloc(sizeof(STREAM));
    mfs_getmds(filename,mds,super->path);
    fseek(filedesc,mds->data.datablocks[mds->data.next_empty]*(super->block_size)+1,SEEK_SET);

    int found=0;
    
    int i;
    for (i=0;i<mds->count;i++)
    {
        if ( fread(tripleta, sizeof(STREAM),1,filedesc) == -1  )
        {
            perror("read");
            exit(1);
        }
        if(strcmp(tripleta->filename,file)==0) found=tripleta->blocknumber;
    }
    /*If there is not such file in the current dir return*/
    if(found==0){
        printf("File not found!\n");
        free(super);
        free(mds);
        fclose(filedesc);
        return;
    }
    /*Get the mds of the file*/
    mfs_getmds(filename, mds, found);
    int blocks_no=(mds->size/super->block_size)+1;
    long unsigned ret;
    int map[abs(blocks_no-10)];
    /*If there are more than 10 blocks read the array containing the info of the rest blocks*/
    if(mds->data.next_empty==11){
        rewind(filedesc);
        fseek(filedesc,((mds->data.datablocks[10])*(super->block_size)+1),SEEK_SET);
        //printf("Reading map from %lu\n",ftell(filedesc));
        if ( (ret=fread(map,1,sizeof(map),filedesc)) == -1) {
            perror("reading map");
            exit(1);
        }
    }
    //printf("mfs_export: is about to start exporting a file of %d blocks %d next empty!\n",blocks_no,mds->data.next_empty);
    char new_path[512];
    int size=mds->size;
    /*Open the new file to write on*/
    strcpy(new_path,directory);
    strcat(new_path,"/");
    strcat(new_path, file);
    int j;
    FILE* new = fopen(new_path,"w+");
    if(new == NULL){
        perror(new_path); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    /*Get all the blocks except the last one (which is smaller than block_size*/
    for(i=0; i<mds->data.next_empty-1; i++){
        rewind(filedesc);
        fseek(filedesc,(mds->data.datablocks[i]*(super->block_size)+1),SEEK_SET);
        //printf("Reading block #%d from %lu size %d!\n",i+1,ftell(filedesc),mds->size);
        if ( (ret=fread(buffer,1,super->block_size,filedesc)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Read %lu bytes from mfs\n",ret);
        //printf("Wrting on %lu\n",ftell(new));
        if ( (ret=fwrite(buffer,1,super->block_size,new)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Wrote %lu bytes on new file!\n",ret);
    }
    /*If there are more than 10 after taking the first 10 take all the rest except the last one*/
    if(mds->data.next_empty==11){

        for(j=0; j<blocks_no-11; j++){
            fseek(filedesc,(map[j]*(super->block_size)+1),SEEK_SET);
            //printf("Reading block #%d from %lu size %d!\n",i+1,ftell(filedesc),mds->size);
            if ( (ret=fread(buffer,1,super->block_size,filedesc)) == -1) {
                perror("writing node");
                exit(1);
            }
            //printf("Read %lu bytes from mfs\n",ret);
            //printf("Wrting on %lu\n",ftell(new));
            if ( (ret=fwrite(buffer,1,super->block_size,new)) == -1) {
                perror("writing node");
                exit(1);
            }
            //printf("Wrote %lu bytes on new file!\n",ret);
        }
        fseek(filedesc,(map[j]*(super->block_size)+1),SEEK_SET);
        //printf("Reading block #%d from %lu size %d!\n",i+1,ftell(filedesc),mds->size);
        
        /*Get the last block*/
        if ( (ret=fread(buffer,1,size%super->block_size,filedesc)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Read %lu bytes from mfs\n",ret);
       // printf("Wrting on %lu\n",ftell(new));
        if ( (ret=fwrite(buffer,1,size%super->block_size,new)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Wrote %lu bytes on new file!\n",ret);
    }
    
    if(mds->data.next_empty!=11){
        /*Get the last block*/
        rewind(filedesc);
        //printf("Super has %d blovks!\n",super->block_count);
        fseek(filedesc,(mds->data.datablocks[i]*(super->block_size)+1),SEEK_SET);
        //printf("Reading block #%d from %lu size %d!\n",i+1,ftell(filedesc),mds->size);
        if ( (ret=fread(buffer,1,(size%super->block_size),filedesc)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Read %lu bytes from mfs\n",ret);
        ret=0;
        //printf("Wrting on %lu\n",ftell(new));
        if ( (ret=fwrite(buffer,1,(size%super->block_size),new)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Wrote %lu bytes on new file!\n",ret);
    }
    
    /*Cleanup*/
    fclose(filedesc);
    fclose(new);
    free(super);
    free(mds);
   
}

int mfs_create_tripleta(char* mfsfile,char* filename,int nodeid,int blockid,int path)
{
    //printf("create_tri: called with:%s %d %d\n",filename,nodeid,blockid);
    
    int seek_var=0;
    int inode_index=0;
    int id_tripletas=0;
    FILE* filedesc = fopen(mfsfile,"r+");
    if(filedesc == NULL){
        perror(mfsfile); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    SUPER* retrieved_super = malloc(sizeof(SUPER));;
    MDS* retrieved_mds=malloc(sizeof(MDS));
    mfs_getsuper(mfsfile,retrieved_super);						//pairnoume to super block
    mfs_getmds(mfsfile,retrieved_mds,path);	//pairnume to mds-inode struct t twrinou fakelou
    
    if(retrieved_super->max_directory_size!=0 && retrieved_super->max_directory_size==retrieved_mds->count){
        printf("Directory has already reached maxsize (%d)\n",retrieved_super->max_directory_size);
        free(retrieved_super);
        free(retrieved_mds);
        return -1;
    }
    
    
    STREAM* tripleta=malloc(sizeof(STREAM));;
    

    //printf("the name of file is %s\n",retrieved_mds->filename);
    
    
    
    if ((retrieved_mds->count!=0) &&((retrieved_mds->count)%retrieved_super->block_stream_capacity==0))//tsekarw an tha prepei na allaksw deikti t datablock[]
    {																	//gt an mpw sto if simainei gemato ekei p deixnei to datablock[] twra
        retrieved_mds->data.next_empty++;
    }
    
    inode_index=retrieved_mds->data.next_empty;
    seek_var=retrieved_mds->data.datablocks[inode_index];
    id_tripletas=retrieved_mds->count;
    retrieved_mds->count++;
    
    if (seek_var<0)     //ean dn iparxei akoma to datablock t folder dimiourgite
    {
        //printf("i timi p travaw exei timi %d\n",retrieved_super->block_count);
        seek_var=retrieved_super->block_count;      //kai pairnoume t deikti t neou block
        retrieved_super->block_count++;          //kai auksanoume ston super block to deikti pros ton teleutaiou block
        blockid++;
        retrieved_mds->data.datablocks[inode_index]=seek_var;//prin edeixne se arnitiko enw twra deixnei se block
    }
    
    //printf("i seek var exei timi %d\n",seek_var);
    //printf("i id_tripleta exei timi %d\n",id_tripletas);
    fseek(filedesc,seek_var*(retrieved_super->block_size)+1,SEEK_SET);
    if (id_tripletas>0){
        
        fseek(filedesc,id_tripletas*(sizeof(STREAM)),SEEK_CUR);
        
    }
    //printf("create_tri: Creating:<%s, %d, %d>\n",filename,nodeid,blockid);
    strcpy(tripleta->filename,filename);
    tripleta->blocknumber=blockid;
    tripleta->fileid=nodeid;
    
    //printf("Writing tripleta on %lu\n",ftell(filedesc));
    if ( fwrite(tripleta,sizeof(STREAM),1,filedesc) == -1) {
        perror("writing super");
        exit(1);
    }
    
    mfs_updateMDS(mfsfile,path,retrieved_mds,retrieved_super->block_size);
    mfs_updatesuper(mfsfile,retrieved_super);
    fclose(filedesc);
    free(tripleta);
    //free(mds);
    free(retrieved_super);
    free(retrieved_mds);
    
    return 0;
}



int mfs_import(char* filename, char* path){
    int i;
    
    char name[64];
    mfs_parsepath(path, name);
    //printf("path:%s filename:%s\n",path,name);
    /*Get super-block*/
    SUPER* super;
    super = malloc(sizeof(SUPER));
    mfs_getsuper(filename,super);
    super->last_inode++; //Update last_inode
    mfs_updatesuper(filename, super);
    char buffer[super->block_size]; //Create buffer for reading from file (max size is block size).

    /*Create tripleta for the new file*/
    if(mfs_create_tripleta(filename,name,super->last_inode,super->block_count,super->path)!=0){
        super->last_inode--;
        mfs_updatesuper(filename, super);
        free(super);
        return 0;
    }
    mfs_getsuper(filename,super);
    /*Get Mds of the directory the file will be created*/
    MDS* parent_mds;
    parent_mds = malloc(sizeof(MDS));
    mfs_getmds(filename, parent_mds, super->path);
    //printf("mfs_import: Parent dir is: %s %d\n",parent_mds->filename,parent_mds->nodeid);
    
    /*Creating new inode (mds) for the file*/
    FILE* fp = fopen(path,"r");
    if(fp == NULL){
        perror(path); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    fseek(fp,0,SEEK_END);
    int size=(int)ftell(fp);
    if(super->max_file_size!=0 && size>super->max_file_size){
        printf("File size should be less than %d bytes!\n",super->max_file_size);
        free(super);
        free(parent_mds);
        fclose(fp);
        return 0;
    }
    int blocks_count=(size/(super->block_size))+1;
    //printf("mfs_import: Inserting file of %d bytes!\n",size);
    int mds_spot = super->block_count;
    //printf("Will write mds on block %d on spot %d!\n",super->block_count,((super->block_count)*(super->block_size))+1);
    rewind(fp);
    
    MDS* mds;
    mds = malloc(sizeof(MDS));
    
    mds->nodeid=(super->last_inode);
    strcpy(mds->filename,name);
    mds->size=size;
    mds->type=1;
    mds->parent_nodeid=parent_mds->nodeid;
    mds->parent_block=super->path;
    time( &mds->creation_time);
    time( &mds->access_time);
    time( &mds->modification_time);
    mds->data.next_empty=0;
    for(i=0; i<13; i++){
        mds->data.datablocks[i]=-1;
    }
    /*Write inode to the file*/
    FILE* filedesc = fopen(filename,"r+");
    if (filedesc == NULL)
    {
        perror(filename); // in order to print error messages, ie: "opening file: Permission denied"
        exit(1);
    }
    fseek(filedesc,(super->block_count *super->block_size)+1,SEEK_SET);
    //printf("Writing the new mds on %lu!\n",ftell(filedesc));
    if ( fwrite(mds,sizeof(MDS),1,filedesc) == -1) {
        perror("writing node");
        exit(1);
    }
    mfs_getsuper(filename, super);
    super->block_count++;
    //printf("Number of blocks:%d files:%d\n",super->block_count,super->files_count);
    mfs_updatesuper(filename, super);
    
    /*Copy File*/
    rewind(fp);
    int j=0;
    int map[abs(blocks_count-10)];
    
    //printf("Start writing %s on %d blocks!\n",path,blocks_count);
    unsigned long ret;
    for(i=0; i<blocks_count-1; i++){
        rewind(filedesc);
        fseek(filedesc,((super->block_count+i)*(super->block_size)+1),SEEK_SET);
        //printf("Reading from file block #%d from %lu!\n",i+1,ftell(fp));
        if ( (ret=fread(buffer,1,super->block_size,fp)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Read %lu bytes!\n",ret);
        //printf("Writing on block %d on %lu\n",(super->block_count+i),ftell(filedesc));
        if(i<=9){
            mds->data.next_empty++;
            mds->data.datablocks[i]=(super->block_count+i);
        }
        if(i>9){
            map[j]=(super->block_count+i);
            j++;
        }
        if ( (ret=fwrite(buffer,1,super->block_size,filedesc)) == -1) {
            perror("writing node");
            exit(1);
        }
        //printf("Wrote %lu bytes\n",ret);
    }
    rewind(filedesc);
    fseek(filedesc,((super->block_count+i)*(super->block_size)+1),SEEK_SET);
    //printf("Reading from file block #%d from %lu!\n",i+1,ftell(fp));
    if ( (ret=fread(buffer,1,(size%super->block_size),fp)) == -1) {
        perror("writing node");
        exit(1);
    }
    //printf("Read %lu bytes!\n",ret);
    //printf("Writing on block %d on %lu\n",(super->block_count+i),ftell(filedesc));
    if(i<=9){
        mds->data.next_empty++;
        printf("\t%d\n",(super->block_count+i));
        mds->data.datablocks[i]=(super->block_count+i);
    }
    if(i>9){
        map[j]=(super->block_count+i);
        j++;
    }
    if ( (ret=fwrite(buffer,1,(size%super->block_size),filedesc)) == -1) {
        perror("writing node");
        exit(1);
    }
    //printf("Wrote %lu bytes\n",ret);
   /*Get Super and Update it*/
    mfs_getsuper(filename, super);
    super->files_count++;
    super->block_count=super->block_count+blocks_count;
    //super->last_inode++;
    //printf("Updating super: blocks:%d files:%d\n",super->block_count,super->files_count);
    mfs_updatesuper(filename, super);
    if(blocks_count>10){
        rewind(filedesc);
        fseek(filedesc,(super->block_count *super->block_size)+1,SEEK_SET);
        //printf("Writing int list on block %d %lu!\n",super->block_count,ftell(filedesc));
        if ( fwrite(map,sizeof(map),1,filedesc) == -1) {
            perror("writing node");
            exit(1);
        }
        mds->data.datablocks[10]=super->block_count;
        mds->data.next_empty++;
        super->block_count+=(sizeof(map)/super->block_size)+1;

    }
    mfs_updateMDS(filename, mds_spot, mds, super->block_size);
    mfs_updatesuper(filename, super);


    printf("Imported file of %d bytes!\n",size);
    fclose(filedesc);
    fclose(fp);
    //fclose(new);
    free(mds);
    free(parent_mds);
    free(super);
    return 0;
}

void mfs_parsepath(char*path,char* ret){
    char buffer[128];
    int i;
    int k=0;
    for(i=0; i<strlen(path); i++){
        if(path[i]=='/'){
            k=0;
            continue;
        }
        buffer[k]=path[i];
        k++;
    }
    buffer[k]='\0';
    strcpy(ret,buffer);
}

void mfs_touch(char* filename,char* file, int a, int m){
    
    MDS* mds;
    mds= malloc(sizeof(MDS));
    SUPER* super;
    super=malloc(sizeof(SUPER));
    /*Get Mds and Super block*/
    mfs_getsuper(filename,super);
    mfs_getmds(filename, mds, super->path);
    int x=search_folder(filename,mds,super,file);
    if(x>=0)
    {
    	mfs_getmds(filename, mds, x);
    	printf("to onoma t arxeiou p kanw touch einai %s \n",mds->filename);
    }
    else
    	{printf("There isn't such file\n");return;}
    /*Based on flags update the timestamps*/
   // printf("%d %d\n",m,a);
    if(a==1) time( &mds->access_time);

    if(m==1) time( &mds->modification_time);
    /*Update the mds with the new values*/
    mfs_updateMDS(filename, x, mds, super->block_size);
    free(mds);
}

void my_cd(char* filename,char*path)
{
    
    int new_path=-1;
    MDS* new_mds;
    new_mds=malloc(sizeof(MDS));
    SUPER* retrieved_super = malloc(sizeof(SUPER));
    mfs_getsuper(filename,retrieved_super);						//pairnoume to super block
    MDS* retrieved_mds=malloc(sizeof(MDS));
    mfs_getmds(filename,retrieved_mds,retrieved_super->path);
    
    if (strcmp(path,"..")==0)
    {
        new_path=retrieved_mds->parent_block;
    }
    else if(strcmp(path,".")==0)
    {
        new_path=retrieved_super->path;
    }
    else
    {
        new_path=search_folder(filename,retrieved_mds,retrieved_super,path);
    }
    //printf("palio path %d neo path %d \n",retrieved_super->path,new_path);
    
    if (new_path>=0)
    {
        mfs_getmds(filename,new_mds,new_path);
        if (new_mds->type!=0)
        {
            printf("You can't cd on a file!\n");
            
        }
        else
        {
            //printf("allazw to path\n");
            retrieved_super->path=new_path;
        }
        
    }
    
    mfs_updatesuper(filename,retrieved_super);
    free(retrieved_super);
    free(retrieved_mds);
    free(new_mds);
    
    
}

void mfs_cd(char*filename,char*path)
{
    char buffer[128];
    int i;
    int k=0;
    
    for(i=0; i<strlen(path); i++){
        //if(path[0]=='/')
        if(path[i]=='/'&& i!=0){
            buffer[k]='\0';
            //printf("mpainw ston fakelo %s\n",buffer);
            my_cd(filename,buffer);
            k=0;
            continue;
        }
        else if(path[i]=='/'&& i==0)
        {
            SUPER* super=malloc(sizeof(SUPER));
            mfs_getsuper(filename,super);
            super->path=1;
            mfs_updatesuper(filename,super);
            free(super);
            if (strlen(path)==1)
                return;
            k=0;
            continue;
        }
        buffer[k]=path[i];
        k++;
    }
    buffer[k]='\0';
    //printf("mpainw ston fakelo %s\n",buffer);
    my_cd(filename,buffer);
    //strcpy(ret,buffer);
}

int search_folder(char*filename,MDS* mds,SUPER* retrieved_super,char* folder_name)
{
    FILE* filedesc = fopen(filename,"r+");
    STREAM* tripleta=malloc(sizeof(STREAM));
    //printf("mpika stin search folder \n");
    int i,j;
    //printf("mds->data.next_empty = %d \n",mds->data.next_empty);
    for (i=0;i<=mds->data.next_empty;i++)
    {
        //rintf("mds->data.datablocks[i] %d\n",mds->data.datablocks[i]);
        fseek(filedesc,mds->data.datablocks[i]*(retrieved_super->block_size)+1,SEEK_SET);
        for (j=0;j<=retrieved_super->block_stream_capacity;j++)
        {
            if ( fread(tripleta, sizeof(STREAM),1,filedesc) == -1  )
            {
                perror("read");
                exit(1);
            }
            if (strcmp(tripleta->filename,folder_name)==0)
            {
                //printf("o fakelos vrethike\n");
                fclose(filedesc);
                free(tripleta);
                return tripleta->blocknumber;//epistrefei se pio block vriskete to arxeio p vrikame
                
            }
        }
    }
    printf("There is no such folder!\n");
    return -1;   //ean o fakelos dn iparxei epistrefei -1
    
    
}


void mfs_pwd(char* filename)
{
    char checker[120];
    char path[120];
    char path1[120];
    MDS* mds=malloc(sizeof(MDS));
    SUPER* super=malloc(sizeof(SUPER));
    mfs_getsuper(filename,super);
    strcpy(path,"");
    mfs_getmds(filename,mds,super->path);
    strcpy(checker,mds->filename);
    int i=0;
    while(strcmp(checker,"/")!=0)
    {
        strcpy(path1,checker);
        mfs_getmds(filename,mds,mds->parent_block);
        strcpy(checker,mds->filename);
        if (i!=0)
            strcat(path1,"/");
        i=1;
        strcat(path1,path);
        strcpy(path,path1);
    }
    strcpy(path1,"/");
    strcat(path1,path);
    strcpy(path,path1);
    //printf("%s\n",mds->filename);
    printf("%s\n",path);
    free(mds);
    free(super);
    
}
