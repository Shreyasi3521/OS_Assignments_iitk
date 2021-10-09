#include<ppipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process information for the ppipe.
struct ppipe_info_per_process {

    // TODO:: Add members as per your need...
    u32 pid;
    int read;
    int empty;
    int read_end;
    int write_end;
    int accq;
};

// Global information for the ppipe.
struct ppipe_info_global {

    char *ppipe_buff;       // Persistent pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    int all_read;
    int write;
    int empty;
    int num_proc;
};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info {

    struct ppipe_info_per_process ppipe_per_proc [MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;

};


// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info* alloc_ppipe_info() {

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */ 
    ppipe->ppipe_global.all_read = 0;
    ppipe->ppipe_global.num_proc = 1;
    ppipe->ppipe_global.write = 0;
    ppipe->ppipe_global.empty = 1;
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        ppipe->ppipe_per_proc[i].accq = 0;
        ppipe->ppipe_per_proc[i].read_end = 0;
        ppipe->ppipe_per_proc[i].write_end = 0;
    }

    ppipe->ppipe_per_proc[0].pid = get_current_ctx()->pid;
    ppipe->ppipe_per_proc[0].accq = 1;
    ppipe->ppipe_per_proc[0].read = 0;
    ppipe->ppipe_per_proc[0].empty = 1;
    // Return the ppipe.
    return ppipe;
}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);

} 

// Fork handler for ppipe.
int do_ppipe_fork (struct exec_context *child, struct file *filep) {
    
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    int i;
    for(i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].pid==child->pid) break;
    }
    if(i==MAX_PPIPE_PROC){
        if(filep->ppipe->ppipe_global.num_proc==MAX_PPIPE_PROC) return -EOTHERS;
        for (i = 0;i<MAX_PPIPE_PROC;i++){
            if(filep->ppipe->ppipe_per_proc[i].accq==0){
                filep->ppipe->ppipe_global.num_proc++;
                filep->ppipe->ppipe_per_proc[i].accq=1;
                filep->ppipe->ppipe_per_proc[i].pid = child->pid;
                filep->ppipe->ppipe_per_proc[i].empty=filep->ppipe->ppipe_global.empty;
                filep->ppipe->ppipe_per_proc[i].read=filep->ppipe->ppipe_global.all_read;
                break;
            }
        }
    }
    if(filep->mode & O_WRITE == O_WRITE) filep->ppipe->ppipe_per_proc[i].write_end = 1;
    if(filep->mode & O_READ == O_READ) filep->ppipe->ppipe_per_proc[i].read_end = 1;
    // Return successfully.
    return 0;

}


// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *                                                                          
     */

    int ret_value,i;
    u32 pid = get_current_ctx()->pid;
    for(i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].pid==pid) break;
    }
    if(i==MAX_PPIPE_PROC) return -EOTHERS;
    if((filep->mode & O_READ) == O_READ) filep->ppipe->ppipe_per_proc[i].read_end = 0;
    else if((filep->mode & O_WRITE) == O_WRITE) filep->ppipe->ppipe_per_proc[i].write_end = 0;
    else return -EOTHERS;
    if(filep->ppipe->ppipe_per_proc[i].read_end==0 && filep->ppipe->ppipe_per_proc[i].write_end==0){
        filep->ppipe->ppipe_global.num_proc--;
        filep->ppipe->ppipe_per_proc[i].accq=0;
    }
    if(filep->ppipe->ppipe_global.num_proc==0) free_ppipe(filep);
    // Close the file.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;
}

// Function to perform flush operation on ppipe.
int do_flush_ppipe (struct file *filep) {

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */

    int reclaimed_bytes = 0;
    int min1=MAX_PPIPE_SIZE,write=filep->ppipe->ppipe_global.write,all_read=filep->ppipe->ppipe_global.all_read;
    int min2 = write,count_empty=0,count_pipe=0;
    if(filep->ppipe->ppipe_global.empty==1) return reclaimed_bytes;
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].read_end==1){
            count_pipe++;
            if(filep->ppipe->ppipe_per_proc[i].read==write){
                if(filep->ppipe->ppipe_per_proc[i].empty==0) return 0;
                else count_empty++;
            }
            else if(filep->ppipe->ppipe_per_proc[i].read<write) 
                min2 = (min2<filep->ppipe->ppipe_per_proc[i].read)?min2:filep->ppipe->ppipe_per_proc[i].read;
            else min1 = (min1<filep->ppipe->ppipe_per_proc[i].read)?min1:filep->ppipe->ppipe_per_proc[i].read;
        }
    }
    if(count_empty==count_pipe){
        if(write>=all_read) reclaimed_bytes = write-all_read;
        else reclaimed_bytes = MAX_PPIPE_SIZE-all_read+write;
        filep->ppipe->ppipe_global.all_read = write;
        filep->ppipe->ppipe_global.empty = 1;
    }
    else{
        int temp = (min1==MAX_PPIPE_SIZE)?min2:min1;
        reclaimed_bytes = temp-all_read;
        filep->ppipe->ppipe_global.all_read = temp;
    }
    // Return reclaimed bytes.
    return reclaimed_bytes;

}

// Read handler for the ppipe.
int ppipe_read (struct file *filep, char *buff, u32 count) {
    
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    int bytes_read = 0;
    if(!filep) return -EINVAL;
	if((filep->mode & O_READ) != O_READ) return -EACCES;
    int pid = get_current_ctx()->pid,proc;
    for(proc=0;proc<MAX_PPIPE_PROC;proc++){
        //printk("pid:%d\n",pid);
        if(filep->ppipe->ppipe_per_proc[proc].pid == pid) break;
    }
    if(proc==MAX_PPIPE_PROC) return -EOTHERS;
    if (filep->ppipe->ppipe_per_proc[proc].read_end==0) return -EACCES;
    int read = filep->ppipe->ppipe_per_proc[proc].read;
    int write = filep->ppipe->ppipe_global.write;
    //printk("read:%d write:%d\n",read,write);
    u32 count1 = 0, count2 = 0;
    if(filep->ppipe->ppipe_per_proc[proc].empty!=1){
        if(write>read){
            if(write-read<=count){
                //printk
                count1 = write-read;
                filep->ppipe->ppipe_per_proc[proc].empty = 1;
            }
            else count1 = count;
        }
        else{
            if(count<=MAX_PPIPE_SIZE-read) count1 = count;
            else{
                count1 = MAX_PPIPE_SIZE-read;
                count2 = count-count1;
            }
            if(count2>0 && write<=count2){
                count2 = write;
                filep->ppipe->ppipe_per_proc[proc].empty = 1;
            }
        }
    }
    //printk("count1:%d count2:%d\n",count1,count2);
    while(bytes_read<count1)
        buff[bytes_read++] = filep->ppipe->ppipe_global.ppipe_buff[read++];
    if(read == MAX_PPIPE_SIZE) read = 0;
    while(bytes_read<count1+count2)
        buff[bytes_read++] = filep->ppipe->ppipe_global.ppipe_buff[read++];
    filep->ppipe->ppipe_per_proc[proc].read = read;
    // Return no of bytes read.
    return bytes_read;
}

// Write handler for ppipe.
int ppipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    int bytes_written = 0;
    if(!filep) return -EINVAL;
	if((filep->mode & O_WRITE) != O_WRITE) return -EACCES;
    int read = filep->ppipe->ppipe_global.all_read;
    int write = filep->ppipe->ppipe_global.write;
    u32 count1=0,count2=0;
    if(read>write){
        if(read-write>=count) count1 = read-write;
        else count1 = count;
    }
    else if((read==write && filep->ppipe->ppipe_global.empty==1)|| read<write){
        if(count>0){
            filep->ppipe->ppipe_global.empty = 0;
            for(int i=0;i<MAX_PPIPE_PROC;i++){
                if(filep->ppipe->ppipe_per_proc[i].accq==1)
                    filep->ppipe->ppipe_per_proc[i].empty = 0;
            }
        }
        if(count>=MAX_PPIPE_SIZE-read){
            count1 = MAX_PPIPE_SIZE-read;
            count2 = (count-count1>write)?write:count-count1;
        }
        else count1 = count;
    }
    while(bytes_written<count1)
        filep->ppipe->ppipe_global.ppipe_buff[write++] = buff[bytes_written++];
    if(write == MAX_PPIPE_SIZE) write = 0;
    while(bytes_written<count1+count2)
        filep->ppipe->ppipe_global.ppipe_buff[write++] = buff[bytes_written++];
    //printk("bytes written:%d read:%d write:%d\n",bytes_written,read,write);
    filep->ppipe->ppipe_global.write = write;
    // Return no of bytes written.
    return bytes_written;
}

// Function to create persistent pipe.
int create_persistent_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
     *
     */
    int i,t;
    struct ppipe_info *ppipe = alloc_ppipe_info();
    ppipe->ppipe_per_proc[0].read_end = 1;
    ppipe->ppipe_per_proc[0].write_end = 1;
    for(i=0,t=0;i<MAX_OPEN_FILES && t<2;i++){
		if(current->files[i] == NULL){
            current->files[i] = alloc_file();
            current->files[i]->type = PPIPE;
            current->files[i]->ppipe = ppipe;
            current->files[i]->fops->close = ppipe_close;
            fd[t++] = i;
        }
    }
    if(i==MAX_OPEN_FILES)
        return -ENOMEM;
    current->files[fd[0]]->mode = O_READ;
    current->files[fd[1]]->mode = O_WRITE;
    current->files[fd[0]]->fops->read = ppipe_read;
    current->files[fd[1]]->fops->write = ppipe_write;
    // Simple return.
    return 0;

}
