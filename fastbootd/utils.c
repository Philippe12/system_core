/*
 * Copyright (c) 2009-2013, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <mtd/mtd-user.h>

#include "utils.h"
#include "debug.h"

//#ifndef BLKDISCARD
//#define BLKDISCARD _IO(0x12,119)
//#endif

//#ifndef BLKSECDISCARD
//#define BLKSECDISCARD _IO(0x12,125)
//#endif


int get_stream_size(FILE *stream) {
    int size;
    fseek(stream, 0, SEEK_END);
    size = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    return size;
}

uint64_t get_block_device_size(int fd)
{
    uint64_t size = 0;
    int ret;

    ret = ioctl(fd, BLKGETSIZE64, &size);

    if (ret)
        return 0;

    return size;
}

uint64_t get_file_size(int fd)
{
    struct stat buf;
    int ret;
    int64_t computed_size;

    ret = fstat(fd, &buf);
    if (ret)
        return 0;

    if (S_ISREG(buf.st_mode))
        computed_size = buf.st_size;
    else if (S_ISBLK(buf.st_mode))
        computed_size = get_block_device_size(fd);
    else
        computed_size = 0;

    return computed_size;
}

uint64_t get_file_size64(int fd)
{
    struct stat64 buf;
    int ret;
    uint64_t computed_size;

    ret = fstat64(fd, &buf);
    if (ret)
        return 0;

    if (S_ISREG(buf.st_mode))
        computed_size = buf.st_size;
    else if (S_ISBLK(buf.st_mode))
        computed_size = get_block_device_size(fd);
    else
        computed_size = 0;

    return computed_size;
}


char *strip(char *str)
{
    int n;

    n = strspn(str, " \t");
    str += n;
    n = strcspn(str, " \t");
    str[n] = '\0';

    return str;
}

int wipe_block_device(int fd, int64_t len)
{
    int ret;
	erase_info_t erase;
	mtd_info_t mtd_info;           // the MTD structure
	
	ret = ioctl(fd, MEMGETINFO, &mtd_info);   // get the device info
	if( ret < 0 ) {
		D(WARN, "Can't get flash info");
		return 1;
	}
	
	erase.length = mtd_info.erasesize;   //set the erase block size
    for(erase.start = 0; erase.start < mtd_info.size; erase.start += erase.length)
    {
		loff_t offs;
        ret = ioctl(fd, MEMUNLOCK, &erase);

		offs = erase.start;
		if( ioctl(fd, MEMGETBADBLOCK, &offs) == 1 ) {
			D(WARN, "Bad block flash at 0x%X", erase.start);
			continue;
		}
        ret = ioctl(fd, MEMERASE, &erase);
		if( ret < 0 ) {
			D(WARN, "Can't erase flash at 0x%X", erase.start);
			return 1;
		}
    }    
#if 0
    uint64_t range[2];
    int ret;

    range[0] = 0;
    range[1] = len;
    ret = ioctl(fd, BLKSECDISCARD, &range);
    if (ret < 0) {
        range[0] = 0;
        range[1] = len;
        ret = ioctl(fd, BLKDISCARD, &range);
        if (ret < 0) {
            D(WARN, "Discard failed\n");
            return 1;
        } else {
            D(WARN, "Wipe via secure discard failed, used discard instead\n");
            return 0;
        }
    }
#endif
    return 0;
}

