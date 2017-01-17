/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/elf.h>


/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat)
{
	int i;
	unsigned short numPH;
	
    if(exeFileData == NULL){
		return (-1);
	}
	
	elfHeader *ourElfHeader = (elfHeader *)exeFileData;
	if(!(ourElfHeader->ident[0] == 0x7f && ourElfHeader->ident[1] == 'E' && ourElfHeader->ident[2] == 'L' && ourElfHeader->ident[3] == 'F'))
	{
	  return (-1);
	}
	
	numPH = ourElfHeader->phnum;
	programHeader *ourProgramHeader = (programHeader *)(exeFileData + ourElfHeader->phoff);
	
	if(numPH > EXE_MAX_SEGMENTS)
	{
	  return (-1);
	}
	
	for(i=0; i<numPH; i++)
	{
		exeFormat->segmentList[i].offsetInFile=(ourProgramHeader->offset);
		exeFormat->segmentList[i].lengthInFile=(ourProgramHeader->fileSize);
		exeFormat->segmentList[i].startAddress=(ourProgramHeader->vaddr);
		exeFormat->segmentList[i].sizeInMemory=(ourProgramHeader->memSize);
		exeFormat->segmentList[i].protFlags=(ourProgramHeader->flags);
		
		ourProgramHeader += 1;
	}
	
	exeFormat->numSegments = numPH;
	exeFormat->entryAddr = ourElfHeader->entry;
	return(0);
}

