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
 *	@file parasite.h
 *	This is the main header file for working with Parasite file injection projects.
 */

#ifndef __PARASITE_H__
#define __PARASITE_H__

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "lz.h"		// Compression Lib
#include "md5.h"	// Hash Lib

#ifdef parasite_export
#define parasite_api __declspec(dllexport)
#else
#define parasite_api __declspec(dllimport)
#endif

#ifdef parasite_static_lib
#define parasite_api
#endif

// Make sure some of the common defines exist
#ifndef BOOL
#define BOOL bool
#endif
#ifndef TRUE
#define TRUE true;
#endif
#ifndef FALSE
#define FALSE false
#endif


/* Make can pass these defines in. MSVC cant. */
#ifndef BUILD_DATE
#define BUILD_DATE "03/18/08"
#endif
#ifndef MAJOR_VERSION
#define MAJOR_VERSION 0
#endif
#ifndef REVISION_VERSION
#define REVISION_VERSION 40
#endif

#define MAX_FILE_NAME 255
#define TAG_SIZE 8			///< Size of special tag string in chars
#define TAG_DATA "Parasite"	///< Text value of the special tag
#define HASH_SIZE 16		///< Size of calculated item hash value

/* Define some feature bits */
#define FEATURE_COMPRESS 0x01 ///< Feature flag bit to enable LZ compression

/**
 * The namespace for out parasite classes.
 */
namespace parasite
{

	/**
	* Enumeration representing a suggested file open mode
	*/
	enum host_access{
		read,		///< Read only access. Use this to read headers etc.
		write,		///< Read and write access. Use this to add data to a file.
		append,		///< Open for append
		full		///< Open for read write and append
	};

	/**
	* A structure that describes an item(or file) stored in parasite host.
	*/
	typedef struct _PARASITE_ITEM
	{
		unsigned char	flags;						///< Implementation specific flags for 
		char			localpath[MAX_FILE_NAME];	///< Local file path
		char			filename[MAX_FILE_NAME];	///< Original file name of item
		unsigned int	offset;						///< Stream offset position for the first byte of this item
		unsigned int	size;						///< Size of the item in bytes
		unsigned int	lzSize;						///< Size of the item when decompressed with lz (if compression used)
		unsigned char	hash[16];					///< Holds MD5 sum of the original file
	} PARASITE_ITEM;

	/**
	* A structure that holds the version information for parasite.
	*/
	typedef struct _PARASITE_VERSION
	{
		int		major;       ///< Major program version number
		int		revision;    ///< Program Revision number
	} PARASITE_VERSION;


	/**
	* A structure that describes a parasite host file.
	* A Parasite host file is a file that has been previosly
	* injected with items using Parasite.
	*/
	typedef struct _PARASITE_HOST_FILE
	{
		PARASITE_VERSION	version;
		char				filename[MAX_FILE_NAME];	///< Host file name
		unsigned short		items;						///< Number of injected items in host
		unsigned int		size;						///< Size of the host file when opened
		unsigned int		baseOffset;					///< Base offset of the parasite files appended data
		unsigned int		headerOffset;				///< Offset that points to the start of the Parasite file table
	} PARASITE_HOST_FILE;


	/**
	* Simple utility function to extract the file name from a path
	* @param path Path represented as a string to extract the file name from
	* @return The filename without leading path text
	*/
	parasite_api char* ExtractFileName(char* path);

	/**
	* Handy function to make a #PARASITE_ITEM out of a filename, and do some error checking.
	* @param item Reference to item we will stroe the results in.
	* @param fileName Char array holding address of name of file to create item out of.
	* @param flags Used to pass in feature flags(compression etc) to us for this file item
	* @return TRUE if the operation was successful.
	*/
	parasite_api BOOL NewItemFromFile(PARASITE_ITEM& item, char* fileName, unsigned char flags = 0);

	/**
	* A Class that provides a simple interface to interacting with a Parasite host file.
	* This class can be used to open and existing Parasite file and perform operations, or to
	* create a new parasite host from an existing file.
	*/
	class parasite_api ParasiteHost
	{
		private:
			PARASITE_VERSION version;	///< Holds a structure to describe what version of Parasite was used to create this host file. 
			PARASITE_HOST_FILE host;	///< Describes the host file properties
	
			FILE* hostFile; ///< The file pointer for the classes instance of an open host file.

			std::vector<PARASITE_ITEM> itemList;      ///< Holds a list of items that are injected into the host file.
			std::vector<PARASITE_ITEM>::iterator itr; ///< An iterator for the itemList
		
			/* Class options */
			BOOL verboseOutput; ///< If this is set TRUE members will display more debugging information at runtime
	
			char LastError[255]; ///< Buffer that holds the last error in ParasiteHost
		
			/**
			*  Utility function for storing text describing the last internal error
			*  @param error String the describes the last ParasiteHost error
			*/
			void SetLastError(const char* error);
	
			/**
			* Opens specified file to use as host (reading and writing in binary).
			* @param file Path to file that will be the host.
			* @param mode File access mode to pass to fopen
			* @return TRUE if we opened the file with permissions
			*/
			BOOL OpenHostFile(char* file, const char* mode);
	
			/**
			* Closes the open host file.
			* @return TRUE if we closed the file without errors
			*/
			BOOL CloseHostFile();
	
			/**
			* Wrapper around fseek. Moves the read pointer to specified offest in hostFile.
			* @param offset Position to move read pointer to in the hostFile stream
			* @return TRUE if the read pointer was set to the specified offset
			*/
			BOOL Seek(unsigned int offset);
	
			/**
			* Wrapper around fseek. Moves the read pointer to the specified offset in hostFile.
			* @param offset Position to move read pointer to in the hostFile stream
			* @return TRUE if the read pointer was set to the specified offset
			*/
			BOOL RSeek(unsigned int offset);

			/**
			*  Wrapper template class to make reading a chumk of data simple.
			*  @param buf A variable to read data into. Size of read is sizeof(buf)
			*              unless size is specified.
			*  @param size By default the fuction calculates size based on the passed
			*               in buffer variable. Optionally pass size to force a 
			*               specific number of bytes to be read. Handy for strings.
			*  @return The size of the data read from the file stream.
			*/
			template <class T>
			size_t Read(T & buf, int size = sizeof(T))
			{
				assert(hostFile != NULL);
				return fread(&buf, size, 1, hostFile);
			}

			/**
			*  Wrapper template class to make writing a chumk of data simple.
			*  @param buf Source Variable to write to file. Size of read is sizeof(buf)
			*              unless size is specified.
			*  @param size By default the fuction calculates size based on the passed
			*               in buffer variable. Optionally pass size to force a 
			*               specific number of bytes to be read. Handy for strings.
			*  @return The size of the data read from the file stream.
			*/
			template <class T>
			size_t Write(T & buf, int size = sizeof(T))
			{
				assert(hostFile != NULL);
				return fwrite(&buf, size, 1, hostFile);
			}

			/**
			*	Writes an item to the Host, and stores the file offset used in the item structure.
			*	WriteItemToHost will write the specified item to the host starting at the current
			*	strean pointer position. Make sure you do a #Seek or #RSeek to get the file in the
			*	propper poistion before calling this function.
			*	@param item Pointer to an item object to write to the host file.
			*	@return TRUE if the item was correctly written to host
			*/
			BOOL WriteItemToHost(PARASITE_ITEM* item);

			/**
			*/
			BOOL CompareFileHash(char* fileName, unsigned char* testHash);

	public:
			/**
			* Constructor
			*/
			ParasiteHost():verboseOutput(true)
			{}

			/**
			* Destructor
			*/
			~ParasiteHost()
			{}
	
			/**
			*  Call this to get some text describing the last ParasiteHost internal failure
			*/
			char* GetLastError();

			/**
			* Public setter for the #verboseOutput flag.
			* Set this to true to get piles of useless data about internal operations.
			* @param verbose How should we set the #verboseOutput flag
			*/
			void SetVerboseOutput(BOOL verbose);

			/**
			* Returns the size of loaded #hostFile
			*/
			unsigned int GetSize();   

			/**
			*  Writes debug information about a PARASITE_ITEM to stdout.
			*/
			void DumpItems();

			/**
			* Adds a #PARASITE_ITEM to the private #itemList
			* @param item Item that will be added to the list
			*/
			void AddItem(PARASITE_ITEM & item);

			/**
			* Pulls an item out of infected host file and saves it in a new file.
			* If the file was infected with compression, ExtractItem will automatically
			* decompress the file while extracting. Extract item will also perform
			* an MD5 checksum test to make sure the extracted file data is accurate.
			* @param itemName Name of the file to extract from the infected host
			* @param path Optional target path to extract items to.
			* @return TRUE if file was extracted
			*/
			BOOL ExtractItem(char* itemName, char* path = NULL);
	
			/**
			* Unpacks all the injected files to the specified path, or .
			* @param path Option path to extract the files into.
			* @return TRUE if files where extracted without error.
			*/
			BOOL ExtractAll(char* path = NULL);	

			/**
			* Restores the original host file to specified new file.
			* @param outfile Target filename to store restored file in
			* @return TRUE if file was restored correctly
			*/
			BOOL RestoreFile(char* outfile);

			/**
			* Parses the filetable from hostFile stream starting at current position.
			*/
			BOOL ReadFileTable();

			/**
			* Tries to detect the presence of a parasite in specified file.
			* Searches the last #TAG_SIZE bytes of the open file for a copy of
			* #TAG_DATA. If this #TAG_DATA is found, we can assume that this file is
			* infected.
			* @returns TRUE if a parasite is found
			*/
			BOOL HasParasite();

			/**
			* Infects the hostFile with a vector of items representing files.
			* @return TRUE if all files where injected into hostFile stream
			*/
			BOOL Infect();

			/**
			* Inserts more items(files) into an exisiting parasite host,
			*/
			BOOL InfectMore(PARASITE_ITEM item);

			/**
			* Generates a parasite file table from a vector of PARASITE_ITEM and writes it to fileHost stream.
			* @param startOffset Stream position to write file table at, or defaults to current position
			* @return TRUE if a valid table was written to the stream.
			*/
			BOOL WriteFileTable(int startOffset = -1);

			/**
			* Extracts the parasite header from infected hostFile.
			*/
			BOOL ReadHeader();

			/**
			* Opens the specified file as hostFile stream.
			* @param file File to open as host file stream
			* @param mode File access mode to pass to fopen.
			* @return TRUE if the file was opened without errors
			*/
			BOOL Open(char* file, const char* mode = "r+b");

			/**
			* Opens the specified file as hostFile for reading only.
			* @param file File to open as host file stream
			* @return TRUE if the file was opened without errors
			*/
			BOOL OpenReadOnly(char* file);
	
			/**
			* Closes the open hostFile stream.
			*/
			void Close();
	};

} // namespace parasite

#endif // __PARASITE_H__
