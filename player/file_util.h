/** 
* @file         file_util.h 
* @Synopsis      
* @author       XieMingming(c265n46@163.com) 
* @version      1.0 
* @date         2012-10-8  
*/
#ifndef __FILE_UTIL_H__
#define __FILE_UTIL_H__


int open_pcm_file(const char *filename);
int write_pcm_data(void *data, int len);
void close_pcm_file();

#endif //__FILE_UTIL_H__