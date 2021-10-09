#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process info for the pipe.
struct pipe_info_per_process {

    // TODO:: Add members as per your need...
    u32 pid;
    int read;       //read end open
    int write;      //write end open
    int accq;
};

// Global information for the pipe.
struct pipe_info_global {

    char *pipe_buff;    // Pipe buffer: DO NOT MODIFY THIS.
    
    // TODO:: Add members as per your need...
    int read;       //read idx next
    int write;      //write idx next
    int empty;
    int num_proc;   //num of active proc
};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info {

    struct pipe_info_per_process pipe_per_proc [MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;

};


// Function to allocate space for the pipe and initialize its members.
struct pipe_info* alloc_pipe_info () {
	
    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *
     */

    // Return the pipe.
    pipe->pipe_global.read = 0;
    pipe->pipe_global.write = 0;
    pipe->pipe_global.empty = 1;
    pipe->pipe_global.num_proc = 1;
    for(int i=0;i<MAX_PIPE_PROC;i++){
        pipe->pipe_per_proc[i].accq = 0;
        pipe->pipe_per_proc[i].read = 0;
        pipe->pipe_per_proc[i].write = 0;
    }
    pipe->pipe_per_proc[0].pid = get_current_ctx()->pid;
    pipe->pipe_per_proc[0].accq = 1;
    return pipe;

}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);

}

// Fork handler for the pipe.
int do_pipe_fork (struct exec_context *child, struct file *filep) {

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    int i;
    for(i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].pid==child->pid) break;
    }
    if(i==MAX_PIPE_PROC){
        if(filep->pipe->pipe_global.num_proc==MAX_PIPE_PROC) return -EOTHERS;
        for (i = 0;i<MAX_PIPE_PROC;i++){
            if(filep->pipe->pipe_per_proc[i].accq==0){
                filep->pipe->pipe_global.num_proc++;
                filep->pipe->pipe_per_proc[i].accq=1;
                filep->pipe->pipe_per_proc[i].pid = child->pid;
                break;
            }
        }
    }
    if(filep->mode & O_WRITE == O_WRITE) filep->pipe->pipe_per_proc[i].write = 1;
    if(filep->mode & O_READ == O_READ) filep->pipe->pipe_per_proc[i].read = 1;
    // Return successfully.
    return 0;

}

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */

    int ret_value,i;
    u32 pid = get_current_ctx()->pid;
    for(i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].pid==pid) break;
    }
    if(i==MAX_PIPE_PROC) return -EOTHERS;
    if((filep->mode & O_READ) == O_READ) filep->pipe->pipe_per_proc[i].read = 0;
    else if((filep->mode & O_WRITE) == O_WRITE) filep->pipe->pipe_per_proc[i].write = 0;
    else return -EOTHERS;
    if(filep->pipe->pipe_per_proc[i].read==0 && filep->pipe->pipe_per_proc[i].write==0){
        filep->pipe->pipe_global.num_proc--;
        filep->pipe->pipe_per_proc[i].accq=0;
    }
    if(filep->pipe->pipe_global.num_proc==0) free_pipe(filep);
    // Close the file and return.
    ret_value = file_close(filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range (unsigned long buff, u32 count, int access_bit) {

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */

    int ret_value = -EBADMEM;
    struct exec_context *curr = get_current_ctx();
    //printk("memory:%X count:%d\n",buff,count);
    for(int i=0;i<MAX_MM_SEGS-1;i++){
        //printk("mmstart:%X mmnextfree:%X mmend:%X access:%d\n",curr->mms[i].start,curr->mms[i].next_free,curr->mms[i].end,curr->mms[i].access_flags);
        if(buff>=curr->mms[i].start && buff<=curr->mms[i].end){
            if(buff+count<curr->mms[i].next_free){
                if(curr->mms[i].access_flags & access_bit == access_bit) ret_value = 1;
            }
            return ret_value;
        }
    }
    if(buff>=curr->mms[MM_SEG_STACK].start && buff<=curr->mms[MM_SEG_STACK].end){
        if(buff+count<=curr->mms[MM_SEG_STACK].end){
            //printk("mmstart:%X mmend:%X access:%d\n",curr->mms[3].start,curr->mms[3].end,curr->mms[3].access_flags);
            if(curr->mms[MM_SEG_STACK].access_flags & access_bit == access_bit) ret_value = 1;
        }
        return ret_value;
    }
    struct vm_area *next = curr->vm_area;
    while (next!=NULL){
        //printk("vm_start:%X vm_end:%X access:%d\n",next->vm_start,next->vm_end,next->access_flags);
        if(buff>=next->vm_start && buff<=next->vm_end){
            if(buff+count<=next->vm_end){
                if(next->access_flags & access_bit == access_bit) ret_value = 1;
            }
            return ret_value;
        }
        next = next->vm_next;
    }
    // Return the finding.
    return ret_value;
}

// Function to read given no of bytes from the pipe.
int pipe_read (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    int bytes_read = 0;
    if(!filep) return -EINVAL;
	if((filep->mode & O_READ) != O_READ) return -EACCES;
    if(is_valid_mem_range((unsigned long)buff, count, 2)==-EBADMEM) return -EOTHERS;
    int read = filep->pipe->pipe_global.read;
    int write = filep->pipe->pipe_global.write;
    u32 count1 = 0, count2 = 0;
    if(filep->pipe->pipe_global.empty!=1){
        if(write>read){
            if(write-read<=count){
                count1 = write-read;
                filep->pipe->pipe_global.empty = 1;
            }
            else count1 = count;
        }
        else{
            if(count<=MAX_PIPE_SIZE-read) count1 = count;
            else{
                count1 = MAX_PIPE_SIZE-read;
                count2 = count-count1;
            }
            if(count2>0 && write<=count2){
                count2 = write;
                filep->pipe->pipe_global.empty = 1;
            }
        }
    }
    while(bytes_read<count1)
        buff[bytes_read++] = filep->pipe->pipe_global.pipe_buff[read++];
    if(read == MAX_PIPE_SIZE) read = 0;
    while(bytes_read<count1+count2)
        buff[bytes_read++] = filep->pipe->pipe_global.pipe_buff[read++];
    filep->pipe->pipe_global.read = read;
    // Return no of bytes read.
    return bytes_read;
}

// Function to write given no of bytes to the pipe.
int pipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    int bytes_written = 0;
    if(!filep){
        //printk("file not open\n");
        return -EINVAL;
    }
	if((filep->mode & O_WRITE) != O_WRITE){
        //printk("file no O_WRITE rights\n");
        return -EACCES;
    }
    if(is_valid_mem_range((unsigned long)buff, count, 1)==-EBADMEM){
        //printk("buff memory not accessible\n");
        return -EOTHERS;
    }
    int read = filep->pipe->pipe_global.read;
    int write = filep->pipe->pipe_global.write;
    u32 count1=0,count2=0;
    if(read>write){
        if(read-write>=count) count1 = read-write;
        else count1 = count;
    }
    else if((read==write && filep->pipe->pipe_global.empty==1)|| read<write){
        if(count>0) filep->pipe->pipe_global.empty = 0;
        if(count>=MAX_PIPE_SIZE-read){
            count1 = MAX_PIPE_SIZE-read;
            count2 = (count-count1>write)?write:count-count1;
        }
        else count1 = count;
    }
    while(bytes_written<count1)
        filep->pipe->pipe_global.pipe_buff[write++] = buff[bytes_written++];
    if(write == MAX_PIPE_SIZE) write = 0;
    while(bytes_written<count1+count2)
        filep->pipe->pipe_global.pipe_buff[write++] = buff[bytes_written++];
    filep->pipe->pipe_global.write = write;
    // Return no of bytes written.
    return bytes_written;
}

// Function to create pipe.
int create_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
     *
     */

    // Simple return.
    int i,t=0;
    struct pipe_info* pip_in = alloc_pipe_info();
    pip_in->pipe_per_proc[0].read = 1;
    pip_in->pipe_per_proc[0].write = 1;
    for(i=0;i<MAX_OPEN_FILES;i++){
		if(current->files[i] == NULL){
            current->files[i] = alloc_file();
            current->files[i]->pipe = pip_in;
            current->files[i]->type = PIPE;
            current->files[i]->fops->close = pipe_close;
            fd[t++] = i;
        }
        if(t==2) break;
    }
    if(i==MAX_OPEN_FILES)
        return -ENOMEM;
    current->files[fd[0]]->mode = O_READ;
    current->files[fd[1]]->mode = O_WRITE;
    current->files[fd[0]]->fops->read = pipe_read;
    current->files[fd[1]]->fops->write = pipe_write;
    return 0;

}
