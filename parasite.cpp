 /*
 *  Copyright (C) 2007  Nick Plante <SowWn@CodeDump.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see http://www.gnu.org/licenses
 *  or write to the Free Software Foundation,Inc., 51 Franklin Street,
 *  Fifth Floor, Boston, MA 02110-1301  USA
 */
/**
 *	@file parasite.cpp
 *	Parasite file injector command line utility.
 *	This is the source code for the stand along command line utility "parasite"
 *	that can inject, read, extract and excersize all the features of the parasite
 *	classes found in #parasite.h
 */
#define _CRT_SECURE_NO_WARNINGS

#define parasite_export
#define parasite_static_lib
#include "parasite.h"

namespace parasite
{
	char* ExtractFileName(char* path)
	{
		char* rslt = NULL;
	
		rslt = strrchr(path, '\\');
		if(rslt != NULL)
			return rslt + 1;
	
		rslt = strrchr(path, '/');
		if(rslt != NULL)
			return rslt + 1;
		
		return path;
	}

	
	BOOL NewItemFromFile(PARASITE_ITEM& item, char* fileName, unsigned char flags)
	{
		FILE* file = fopen(fileName, "r");
		if(file == NULL)
		{
			printf("Could not read input file %s\n", fileName);
			return FALSE;
		}

		/*
			Store original file name
		*/
		strcpy(item.localpath, fileName);
		strcpy(item.filename, ExtractFileName(fileName));

		/* 
			Store original file size
		*/
		fseek(file, 0, SEEK_END);
		item.size = ftell(file);
		item.lzSize = 0;
		item.offset = 0;
		
		/*
			Assign requested flags to the new file item
		*/
		item.flags = flags;
	
		fclose(file);
		return TRUE;
	}

	
	void ParasiteHost::SetLastError(const char* error) 
	{
		strcpy(LastError, error);
	}
	
	char* ParasiteHost::GetLastError()
	{
		return (char*)&LastError;
	}


	BOOL ParasiteHost::OpenHostFile(char* file, const char* mode)
	{
		hostFile = fopen(file, mode);
		return(hostFile != NULL);
	}


	BOOL ParasiteHost::CloseHostFile()
	{
		assert(hostFile != NULL);
		return(fclose(hostFile) == 0);
	}

	
	BOOL ParasiteHost::Seek(unsigned int offset)
	{
		assert(hostFile != NULL);
		if((offset < 0) || (offset > host.size))
			return FALSE;
		
		fseek(hostFile, offset, SEEK_SET);
		return TRUE;
	}

	
	BOOL ParasiteHost::RSeek(unsigned int offset)
	{
		assert(hostFile != NULL);
		if((offset < 0) || (offset > host.size))
			return FALSE;
		
		fseek(hostFile, (host.size - offset), SEEK_SET);
		return TRUE;
	}
	
	
	BOOL ParasiteHost::WriteItemToHost(PARASITE_ITEM* item)
	{
		assert(item != NULL);
		assert(hostFile != NULL);

		if(verboseOutput)
		{		  
			printf("\n Infecting <%s> with [%u] bytes from File <%s>\n", host.filename, item->size, item->localpath);
			if(item->flags > 0)
			{
				printf("  Using features:\n");
				if(item->flags & FEATURE_COMPRESS)
					printf("    compression\n");
			}
		}


		/*
			Calculate the HASH of the _original_ file.
			This hash will be checked against after the file has been restored.
		*/
		if(md5_file(item->localpath, item->hash) != 0)
		{
			printf(" Failed to gernerate MD5 sum for %s", item->localpath);
			return FALSE;
		}
		if(verboseOutput)
		{
			printf("  * %s Hash: ", item->localpath);
			for(int i = 0; i < HASH_SIZE; i++)
				printf("%x", item->hash[i]);
			printf("\n");			
		}
		
		/* 
			These allocations are calculated from lz4 worst case compress size.
			TODO: I Believe that this calculation is incorrect (too large) fix it! 
		*/
		unsigned int bufsize = (item->size * 104 + 50) / 100 + 384;
		unsigned char* itemBuf = (unsigned char*) malloc(item->size + 2 * bufsize);
		if(!itemBuf)
		{
			printf(" Failed to allocate [itemBuf] buffer for WriteItemToHost\n");
			return FALSE;
		}
		unsigned char* buf = &itemBuf[item->size];

		/* 
			Read item file into buffer 
		*/
		FILE* readsrc = fopen(item->localpath, "rb");
		fread(itemBuf, 1, item->size, readsrc);	   
		fclose(readsrc);

		/* 
			If the compression feature is set on this item, compress the file buffer
		*/
		if(item->flags & FEATURE_COMPRESS)
		{
			unsigned int* work = (unsigned int*) malloc(sizeof(unsigned int) * (65536 + item->size));
			if(work)
			{
				printf("  Original file size: %u\n", item->size);

				item->lzSize = item->size;
	  			item->size = LZ_CompressFast(itemBuf, buf, item->size, work);
				
				printf("  Finished compress with size: %u\n", item->size);
				free(work);

				item->offset = ftell(hostFile);
				fwrite(buf, 1, item->size, hostFile);
				
				free(itemBuf);
				return TRUE;
			}	  
			else
				printf(" Failed to allocate work buffer for compress\n");
		}

		/* 
			Append the file final data into the current working item.
		*/
		item->offset = ftell(hostFile);
		fwrite(itemBuf, 1, item->size, hostFile);
		
		free(itemBuf);
		return TRUE;
	}


	void ParasiteHost::SetVerboseOutput(BOOL verbose)
	{
		verboseOutput = verbose;
	}


	unsigned int ParasiteHost::GetSize()
	{
		assert(hostFile != NULL);
		fseek(hostFile, 0, SEEK_END);
		return ftell(hostFile);
	}


	void ParasiteHost::DumpItems()
	{
		PARASITE_ITEM item;
		for(itr = itemList.begin(); itr < itemList.end(); itr++)
		{
			item = *itr;
			
			//printf("FileName\tFlags\tSize\tOffset\n");
			printf("\n");
			printf("%s\n", item.filename);
			printf("  flags:       %u\n", item.flags);
			printf("  size:        %u\n", item.size);
			printf("  offset:      %d\n", item.offset);
			printf("  lzSize:      %u\n", item.lzSize);
			printf("  hash: \t");
			for(int i = 0; i < HASH_SIZE; i++)
				printf("%x", item.hash[i]);
			printf("\n");
			printf("\n");
		}
	}


	void ParasiteHost::AddItem(PARASITE_ITEM & item)
	{
		itemList.push_back(item);
	}


	BOOL ParasiteHost::ExtractItem(char* itemName, char* path)
	{
		assert(hostFile != NULL);

		char targetPath[MAX_FILE_NAME];
		if(path == NULL)
			strcpy(targetPath, itemName);
		else
		{
			strcpy(targetPath, path);
			strcat(targetPath, itemName);
		}   

		PARASITE_ITEM item;
		for(itr = itemList.begin(); itr < itemList.end(); itr++)
		{
			item = *itr;
			if(strcmp(item.filename, itemName) == 0)
			{
				if(verboseOutput)
					printf("Extracting item %s to %s\n", item.filename, targetPath);

				FILE* dest = fopen(targetPath, "w+b");
				if(dest == NULL)
				{
					printf("Error opening file %s with write access\n", targetPath);
					return FALSE;
				}
								
				/* 
					Read the parasite file into a new work buffer 
				*/
				unsigned char* itemBuf = (unsigned char*) malloc(item.size);
				if(!itemBuf)
				{
					printf("Failed to allocate the extract buffer of %u bytes\n Aborting\n", item.size);
					fclose(dest);
					return FALSE;
				}			  
				Seek(item.offset);
				fread(itemBuf, 1, item.size, hostFile);
				
				/* 
					If the file uses compression, extract it to the new location 
				*/
				if(item.flags & FEATURE_COMPRESS)
				{
					unsigned char* out = (unsigned char*) malloc(item.lzSize);
					if(!out)
					{
						printf("Failed to allocate a decompress buffer of %u bytes\n", item.lzSize);
						free(itemBuf);
						fclose(dest);
						return FALSE;
					}
					
					LZ_Uncompress(itemBuf, out, item.size);
					fwrite(out, 1, item.lzSize, dest);
					free(out);
				}
				else 
				{
					/* 
						No compression was used on this file
					*/
					fwrite(itemBuf, 1, item.size, dest);
				}
				
				fclose(dest);
				free(itemBuf);				

				/* 
					Check the final hash against original hash value stored during injection
				*/
				CompareFileHash(targetPath, item.hash);

				return TRUE;
			}
		}

		return FALSE;
	}

	
	BOOL ParasiteHost::CompareFileHash(char* fileName, unsigned char* testHash)
	{
		unsigned char finalHash[HASH_SIZE];
		if(md5_file(fileName, finalHash) != 0)
		{
			printf("Could not calculate final hash\n");
			return FALSE;
		}
		
		for(int i = 0; i < HASH_SIZE; i++)
			if(finalHash[i] != testHash[i])
				return FALSE;		
	
		return TRUE;
	}
	
	BOOL ParasiteHost::ExtractAll(char* path)
	{
		assert(hostFile != NULL);
		
		for(itr = itemList.begin(); itr < itemList.end(); itr++)
			if(ExtractItem((PARASITE_ITEM(*itr)).filename, path) == FALSE)
				return FALSE;								

		return TRUE;
	}


	BOOL ParasiteHost::RestoreFile(char* outfile)
	{
		assert(hostFile != NULL);
		
		if(verboseOutput)
			printf("Restoring to file %s\n", outfile);

		FILE* out = fopen(outfile, "w+b");
		if(out == NULL)
		{
			printf("Could not open file %s for writing\n", outfile);
			printf("Aborting restore operation\n");
			return FALSE;
		}

		if((host.baseOffset < 0) || (host.baseOffset > host.size))
		{
			printf("Base offset [%u] seems corrupt\n", host.baseOffset);
			printf("Aborting restore operation\n");
			return FALSE;
		}
	
		Seek(0);
		for(unsigned int i = 0; i < host.baseOffset; i++)
			fputc(fgetc(hostFile), out);

		fclose(out);
		
		return TRUE;
	}


	BOOL ParasiteHost::ReadFileTable()
	{
		assert(hostFile != NULL);

		PARASITE_ITEM item;
		unsigned short bufsize = 0;
		
		for(int i = 0; i < host.items; i++)
		{			
			Read(item.size);                // Host file size  
			Read(item.lzSize);              // Size of lz compression
			Read(item.offset);              // Items location in stream realative to 0
			Read(item.flags);               // Feature flags
			Read(item.hash);                // Original file crc32 hash
			Read(bufsize);                  // Size of the file name string
			Read(item.filename, bufsize);   // File name string
		
			itemList.push_back(item);
		}
		return TRUE;
	}


	BOOL ParasiteHost::HasParasite()
	{
		assert(hostFile != NULL);

		char tag[TAG_SIZE];

		RSeek(TAG_SIZE);
		Read(tag, TAG_SIZE);		

		if(memcmp(tag, "Parasite", TAG_SIZE) == 0)
			return TRUE;
		
		return FALSE;
	}


	BOOL ParasiteHost::Infect()
	{
		assert(hostFile != NULL);
		
		if(HasParasite())
		{
			printf("This file is already infected. Try -a to add more files to this host.\n");
			return FALSE;
		}
		
		Seek(host.size);

		host.baseOffset = ftell(hostFile);
		if(verboseOutput)
			printf("Writing files starting at base offset %u\n", host.baseOffset);
		
		for(itr = itemList.begin(); itr < itemList.end(); itr++)
			if(WriteItemToHost(&*itr) == FALSE)
				return FALSE;

		return TRUE;
	}


	BOOL ParasiteHost::InfectMore(PARASITE_ITEM item)
	{
		assert(hostFile != NULL);
		
		if(HasParasite() == FALSE)
		{
			printf("This file is not already infected. Try -c to create a host using this file.\n");
			return FALSE;
		}

		itemList.push_back(item);
		
		/* 
			The appended file will overwrite the current header
		*/
		if(verboseOutput)
			printf("Seeking to old header offset : %u\n", host.headerOffset);
			
		Seek(host.headerOffset);
		
		if(WriteItemToHost(&item) == FALSE)
		{
			/*
				Failure... we should clean up make a method.
			*/
		}
				   
		return TRUE;
	}


	BOOL ParasiteHost::WriteFileTable(int startOffset)
	{
		assert(hostFile != NULL);
		
		if(startOffset != -1)
			Seek(startOffset);
		
		host.headerOffset = ftell(hostFile);
		/* Maybe a little too verbose!
		if(verboseOutput)
			printf("Header offset recorded at %u\n", host.headerOffset);
		*/

		/*
			Write version information
		*/
		fputc(MAJOR_VERSION, hostFile);
		fputc(REVISION_VERSION, hostFile);

		/*
			Write the number of files in infestation
		*/
		host.items = itemList.size();
		Write(host.items);

		/*
			Write the base offset for out infestation
		*/
		Write(host.baseOffset);

		/*
			Write all of the file items to the file stream
		*/
		PARASITE_ITEM item;
		for(itr = itemList.begin(); itr < itemList.end(); itr++)
		{
			item = *itr;

			Write(item.size);
			Write(item.lzSize);
			Write(item.offset);
			Write(item.flags);
			Write(item.hash);
			unsigned short sz = strlen(item.filename) + 1;		
			Write(sz);
			Write(item.filename, sz);
		}

		/*
			Last thing we write is the base address for the file table.
		 	We do this so that we do not need to do a linear search for the table.
		  	A simple address extraction off the tail of the file will be O(1).
		*/
		Write(host.headerOffset);
		Write(TAG_DATA, TAG_SIZE);

		return TRUE;
	} 


	BOOL ParasiteHost::ReadHeader()
	{
		assert(hostFile != NULL);

 		/*
 			First we need to get the file table base offset. 
			it is stored on the tail of the file.
		*/
		RSeek(sizeof(host.headerOffset) + TAG_SIZE);
		Read(host.headerOffset);

		/* 
			Place the file ptr at the start of the header (aka file table) 
		*/
		Seek(host.headerOffset);
		
		/* 
			Read parasite version information 
		*/
		host.version.major = fgetc(hostFile);
		host.version.revision = fgetc(hostFile);

		/* 
			Read number of infested files 
		*/
		Read(host.items);
		
        /* 
        	Read the base offset (start of our data payload)
        */
		Read(host.baseOffset);
		
		if(verboseOutput)
		{
			printf("------------------------------------------------------------\n");
			printf("|  Parasite Infestation Version: %d.%d\n", host.version.major, host.version.revision);
			printf("|  File Count: %u\n", host.items);
			printf("|  Base infestation offset: %u\n", host.baseOffset);	
			printf("------------------------------------------------------------\n");
		}

		return TRUE;
	}


	BOOL ParasiteHost::Open(char* file, const char* mode)
	{
		if(!OpenHostFile(file, mode))
			return FALSE;
		
		strcpy(host.filename, file);
		host.size = GetSize();		

		return TRUE;
	}


	BOOL ParasiteHost::OpenReadOnly(char* file)
	{
		return Open(file, "rb");
	}


	void ParasiteHost::Close()
	{
		assert(hostFile != NULL);
		fclose(hostFile);
	}

}
