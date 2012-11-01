/** 
* @file         file_util.cpp 
* @Synopsis     write PCM date into file, just for debug 
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#include "stdio.h"

static FILE *file;

int open_pcm_file(const char *filename)
{
    return fopen_s(&file, filename, "ab");//二进制文件，追加方式打开
}

int write_pcm_data(void *data, int len)
{
    if (!file)
        return -1;

    fwrite(data, sizeof(unsigned char), len, file);
    return 0;
}

void close_pcm_file()
{
    fclose(file);
}
