typedef struct {
    int next_empty;
    int datablocks [13];
} Datastream ;


typedef struct {
    unsigned int nodeid ;
    char filename[16] ;
    unsigned int size ;
    unsigned int type ;
    unsigned int count; //Counts files in a dir or blocks of a file
    unsigned int parent_nodeid ;
    int parent_block; //Block number for parent directory
    time_t creation_time ;
    time_t access_time ;
    time_t modification_time ;
    Datastream data ;
} MDS ;	

typedef struct{
    char filename[16];
    int fileid;
    int blocknumber;
}STREAM;

typedef struct{
    int block_size; //Size of one block
    int block_stream_capacity; // How many streams can a block store
    int block_count; //Number of blocks consisting the FS
    int files_count; //Count of files-dirs-links in the FS
    int last_inode; //Last inode;
    int path; //curent path
    int max_file_size;
    int max_directory_size;
}SUPER;