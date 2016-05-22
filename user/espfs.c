/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/

/**
 * @file espfs.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @contributors Carlos Martin Ugalde and Ignacio Ripoll García
 * @brief Simple read-only file syetem for use with httpd. 
 *
 * 
 * This is a simple read-only implementation of a file system. It uses a block 
 * of data coming from the mkespfsimg tool, and can use that block to do
 * abstracted operations on the files that are in there. It's written for use
 * with httpd, but doesn't need to be used as such.
 *
 * These routines can also be tested by comping them in with the espfstest tool.
 * This simplifies debugging, but needs some slightly different headers. The 
 * #ifdef takes care of that.
 */

#ifdef __ets__
//esp build
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "espmissingincludes.h"
#else
//Test build
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define os_malloc malloc
#define os_free free
#define os_memcpy memcpy
#define os_strncmp strncmp
#define os_strcmp strcmp
#define os_strcpy strcpy
#define os_printf printf
#define ICACHE_FLASH_ATTR
extern char* espFsData;
#endif

//Debug mode 1 = on
#define DEBUG 0

#include "../mkespfsimage/espfsformat.h"
#include "espfs.h"
#include "httpdconfig.h"
#ifdef EFS_HEATSHRINK
#include "heatshrink_decoder.h"
#endif

struct EspFsFile {
	EspFsHeader *header;
	char decompressor;
	int32_t posDecomp;
	char *posStart;
	char *posComp;
	void *decompData;
};

/*
 * @brief Copies len bytes over from dst to src, but does it using *only*
 * aligned 32-bit reads. 
 *
 * Copies len bytes over from dst to src, but does it using *only*
 * aligned 32-bit reads. Yes, it's no too optimized but it's short and sweet and
 * it works.
 * 
 * To vew a full memory map of esp8266 we recomed this sites:
 * https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map
 * http://esp8266-re.foogod.com/wiki/Memory_Map
 * 
 * Accessing the flash through the mem emulation at 0x40200000 is a bit hairy: 
 * All accesses *must* be aligned 32-bit accesses. Reading a short, byte or 
 * unaligned word will result in a memory exception, crashing the program.
 */

void ICACHE_FLASH_ATTR _memcopy_aligned(char *dst, char *src, int len) {
	int x;
	int w, b;
	for (x=0; x<len; x++) {
		b=((int)src&3);
		w=*((int *)(src-b));
		if (b==0) *dst=(w>>0);
		if (b==1) *dst=(w>>8);
		if (b==2) *dst=(w>>16);
		if (b==3) *dst=(w>>24);
		dst++; src++;
	}
}

/*
 * @brief Open a file and return a pointer to the file desc struct.
 */

EspFsFile ICACHE_FLASH_ATTR *espfs_open(char *fileName) {
#ifdef __ets__
	char *p = (char *)(ESPFS_POS+0x40200000);
#else
	char *p = espFsData;
#endif
	char *hpos;
	char namebuf[256];
	EspFsHeader h;
	EspFsFile *r;

	//Strip initial slashes
	while(fileName[0] == '/') fileName++;

	//Go find that file!
	while(1) {
		hpos = p;
		//Grab the next file header.
		os_memcpy(&h, p, sizeof(EspFsHeader));
		if (h.magic != 0x73665345) {
			os_printf("Magic mismatch. EspFS image broken.\n");
			return NULL;
		}

		if (h.flags & FLAG_LASTFILE) {
			os_printf("End of image.\n");
			return NULL;
		}

		//Grab the name of the file.
		p += sizeof(EspFsHeader); 
		os_memcpy(namebuf, p, sizeof(namebuf));
# if DEBUG
		os_printf("Found file '%s'. Namelen=%x fileLenComp=%x, compr=%d flags=%d\n", 
				namebuf, (unsigned int)h.nameLen, (unsigned int)h.fileLenComp, h.compression, h.flags);
#endif

		if (os_strcmp(namebuf, fileName) == 0) {
			// Yay, this is the file we need!
			p += h.nameLen; // Skip to content.
			r = (EspFsFile *)os_malloc(sizeof(EspFsFile)); // Alloc file desc mem

			if (r == NULL) return NULL;

			r->header = (EspFsHeader *)hpos;
			r->decompressor = h.compression;
			r->posComp = p;
			r->posStart = p;
			r->posDecomp = 0;

			if (h.compression == COMPRESS_NONE) {
				r->decompData = NULL;
#ifdef EFS_HEATSHRINK
			} else if (h.compression == COMPRESS_HEATSHRINK) {
				// File is compressed with Heatshrink.
				char parm;
				heatshrink_decoder *dec;
				// Decoder params are stored in 1st byte.
				_memcopy_aligned(&parm, r->posComp, 1);
				r->posComp++;
#if DEBUG
				os_printf("Heatshrink compressed file; decode parms = %x\n", parm);
#endif
				dec = heatshrink_decoder_alloc(16, (parm>>4)&0xf, parm&0xf);
				r->decompData = dec;
#endif
			} else {
				os_printf("Invalid compression: %d\n", h.compression);
				return NULL;
			}
			return r;
		}

		//We don't need this file. Skip name and file
		p += h.nameLen+h.fileLenComp;

		if ((int)p&3)p += 4-((int)p&3); // Align to next 32bit val
	}
}

/*
 * @brief Read len bytes from the given file into buff. Returns the actual 
 * amount of bytes read. 
 */

int ICACHE_FLASH_ATTR espfs_read(EspFsFile *fh, char *buff, int len) {
	int flen;

	if (fh == NULL) return 0;

	//Cache file length.
	_memcopy_aligned((char*)&flen, (char*)&fh->header->fileLenComp, 4);

	//Do stuff depending on the way the file is compressed.
	if (fh->decompressor == COMPRESS_NONE) {
		int toRead;
		toRead = flen-(fh->posComp-fh->posStart);

		if (len>toRead) len = toRead;

# if DEBUG
		os_printf("Reading %d bytes from %x\n", len, (unsigned int)fh->posComp);
# endif
		_memcopy_aligned(buff, fh->posComp, len);
		fh->posDecomp += len;
		fh->posComp += len;
# if DEBUG
		os_printf("Done reading %d bytes, pos=%x\n", len, fh->posComp);
# endif
		return len;
#ifdef EFS_HEATSHRINK
	} else if (fh->decompressor == COMPRESS_HEATSHRINK) {
		int decoded = 0;
		unsigned int elen, rlen;
		char ebuff[16];
		heatshrink_decoder *dec = (heatshrink_decoder *)fh->decompData;

		while (decoded < len) {
			// Feed data into the decompressor
			// ToDo: Check ret val of heatshrink fns for errors
			elen = flen-(fh->posComp - fh->posStart);

			if (elen == 0) return decoded; // File is read

			if (elen > 0) {
				_memcopy_aligned(ebuff, fh->posComp, 16);
				heatshrink_decoder_sink(dec, (uint8_t *)ebuff, (elen>16)?16:elen, &rlen);
				fh->posComp += rlen;

				if (rlen == elen) {
					heatshrink_decoder_finish(dec);
				}
			}

			// Grab decompressed data and put into buff
			heatshrink_decoder_poll(dec, (uint8_t *)buff, len-decoded, &rlen);
			fh->posDecomp += rlen;
			buff += rlen;
			decoded += rlen;
		}

		return len;
#endif
	}

	return 0;
}

/*
 * @brief Close the file. 
 */

void ICACHE_FLASH_ATTR espfs_close(EspFsFile *fh) {
	if (fh == NULL) return;
#ifdef EFS_HEATSHRINK
	if (fh->decompressor == COMPRESS_HEATSHRINK) {
		heatshrink_decoder_free((heatshrink_decoder*)fh->decompData);
	}
#endif
	os_free(fh);
}
