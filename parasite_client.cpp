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

//#include <vector>

//#include <stdio.h>
//#include <stdlib.h>
//using namespace std;

#define parasite_static_lib
#include "parasite.h"
using namespace parasite;

BOOL verbose = FALSE;
unsigned char _flags = 0;

/**
 * Enumeration describing the major operation modes for parasite
 */
enum parasite_operation{op_none, 
						op_create, 
						op_extract, 
						op_extractall,
						op_add, 
						op_list, 
						op_restore, 
						op_remove, 
						op_multi
};

/**
 * Checks if a file exists on disk at the specified location
 */
BOOL FileExists(char* filename)
{
	FILE* file;
	BOOL result = TRUE;
	
	file = fopen(filename, "r");
	if(file == NULL)
		result = FALSE;
	fclose(file);
	
	return result;
}

/**
 * Print this parasite binary version
 */
void PrintVersion()
{
	printf("Parasite Version %d.%d [%s]\t~SowWn\n", MAJOR_VERSION, REVISION_VERSION, BUILD_DATE);
}

/**
 * Print usage help to the console
 */
void PrintUsage()
{
	PrintVersion();
	printf("Usage: parasite [-cixXalrdvz] [HOST] [ITEM(s)] [PATH]\n");
}

/**
 * Print application help to the console
 */
void PrintHelp()
{
	printf("\n");
	PrintUsage();
	printf("\n");
	printf("Parasite inserts or extracts files into the binary stream of a host binary file.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  parasite -c host.exe file1          : Injects foo.png into host.exe\n");
	printf("  parasite -c host.exe file1 file2    : Injects file1 and file2 into host.exe\n");
	printf("  parasite -l host.exe                : Lists any infected files in host.exe\n");
	printf("  parasite -x host.exe foo.png        : Extracts foo.png from host.exe\n");
	printf("  parasite -x host.exe foo.png temp\\  : Extracts foo.png from host.exe into relative path temp\n");
	printf("  parasite -X host.exe                : Extracts all parasite files from host.exe\n");
	printf("  parasite -X host.exe temp\\          : Extracts all parasite files from host.exe into relative path temp\n");
	printf("  parasite -r host.exe restore.exe    : Restores original host.exe to restore.exe\n");
	printf("\n");
	printf("Main operation mode:\n");
	printf("  -c      create a new parasite host\n");
	printf("  -l      list infected files in host\n");
	printf("  -x      extract item from host\n");
	printf("  -X      extract all items from host\n");
	printf("  -r      restore host file to original binary\n\n");
	printf("\n");
	printf("Optional operation mode:\n");
	printf("  -v      enable verbose output\n");
	printf("  -z      use compression\n");
}

/**
 * Ugly function that pulls operation switches out of the switch param
 * We do ugly slow parsing here so that we can identify multi-operation
 * switch use cases. This is all to help the user notice he/she is trying
 * to do something stupid... how nice of us.
 */
parasite_operation ParseOperation(char* operation)
{
	parasite_operation op = op_none;	

#define SetOp(id) \
		{\
            if(op != op_none) \
				return op_multi; \
			op = id;\
        }

	if(strchr(operation, 'c') != NULL)
		op = op_create;	
	
	if(strchr(operation, 'i') != NULL)
		SetOp(op_create)

	if(strchr(operation, 'x') != NULL)
		SetOp(op_extract)

	if(strchr(operation, 'X') != NULL)
		SetOp(op_extractall)

	if(strchr(operation, 'a') != NULL)
		SetOp(op_add)

	if(strchr(operation, 'l') != NULL)
		SetOp(op_list)

	if(strchr(operation, 'r') != NULL)
		SetOp(op_restore)

	if(strchr(operation, 'd') != NULL)
		SetOp(op_remove)
	
	return op;
} 

/**
 * Parse the option operation modes out of the first argument
 */
void ParseOptionFlags(char* flags)
{
	if(strchr(flags, 'v') != NULL)	
		verbose = TRUE;
	

	if(strchr(flags, 'z') != NULL)
		_flags |= FEATURE_COMPRESS;
}

/**
 * Lists all of the infected files in the specified binary in table format
 */
BOOL List(int argc, char** argv)
{
	ParasiteHost host;

	if(argc < 3)
	{
		printf("Forget the file name?\n");
		printf("Aborting\n");
		return FALSE;
	}
	
	host.OpenReadOnly(argv[2]);
	host.SetVerboseOutput(verbose);

	if(host.HasParasite() == FALSE)
	{
		printf("Specified file %s does not have a Parasite header, or its header is corrupt.\n", argv[2]);
		return FALSE;			
	}
	
	host.ReadHeader();
	host.ReadFileTable();
	host.DumpItems();
	host.Close();
	return TRUE;
}

/**
 * Infect specified binary into the host binary
 */
BOOL Infect(int argc, char** argv)
{
	ParasiteHost host;

	if(argc < 4)
	{
		printf("Missing Parametres?\n");
		PrintUsage();
		return FALSE;
	}
	
	if(!host.Open(argv[2]))
	{
		printf("Could not load Host File %s\n", argv[2]);
		host.Close();
		return FALSE;
	}
	host.SetVerboseOutput(verbose);

	PARASITE_ITEM item;
	for(int i = 3; i < argc; i++)
		if( NewItemFromFile(item, argv[i], _flags) )
			host.AddItem(item);
		else
		{
			host.Close();
			return FALSE;
		}

	host.Infect();
	host.WriteFileTable();
	
	host.Close();
	return TRUE;
}

/**
	This function is not completed (working) yet afaik.
 */
BOOL InfectMore(int argc, char** argv)
{
	ParasiteHost host;
	host.SetVerboseOutput(verbose);

	if(argc < 4)
	{
		printf("Missing Parametres?\n");
		PrintUsage();
		return FALSE;
	}
	
	if(!host.Open(argv[2]))
	{
		printf("Could not load Host File %s\n", argv[2]);
		host.Close();
		return FALSE;
	}

	if(host.ReadHeader() == FALSE)
	{
		printf("This file does not have a Parasite header, or its header is corrupt.");
		return FALSE;
	}	
	
	if(!host.ReadFileTable())
		return 1;
	
	if(verbose)
		printf("Read File Table OK.\n");

	
	if(FileExists(argv[3]) == FALSE)
	{
		host.Close();
		return FALSE;
	}   

	PARASITE_ITEM item;
	if(NewItemFromFile(item, argv[3]))
		host.InfectMore(item);

	host.WriteFileTable();
	host.Close();	
	
	return TRUE;
}

/**
 * Parasite x [hosted file] <folder>.
 * Example: Parasite x test.png /temp/
 * restores to /temp/test.png
 */
BOOL Extract(char* hostfile, char* item, char* path = NULL)
{
	ParasiteHost host;

	if(host.OpenReadOnly(hostfile) == FALSE)
	{
		printf("Could not load Host File %s\n", hostfile);
		return FALSE;
	}
	host.SetVerboseOutput(verbose);

	if(host.ReadHeader() == FALSE)
	{
		printf("This file does not have a Parasite header, or its header is corrupt.");
		return FALSE;
	}
	
	host.ReadFileTable();
	
	if(host.ExtractItem(item, path) == FALSE)
	{
		printf("Could not find %s in parasite file.\n", item);
		host.Close();
		return FALSE;
	}

	host.Close();
	return TRUE;
}

/**
 * Extract all of the infected files, optionally write them to a specified path.
 */
BOOL ExtractAll(char* hostfile, char* path = NULL)
{
	ParasiteHost host;

	if(host.OpenReadOnly(hostfile) == FALSE)
	{
		printf("Could not load Host File %s\n", hostfile);
		return FALSE;
	}
	host.SetVerboseOutput(verbose);

	if(host.ReadHeader() == FALSE)
	{
		printf("This file does not have a Parasite header, or its header is corrupt.");
		return FALSE;
	}
	
	host.ReadFileTable();

	BOOL result = host.ExtractAll(path);
	host.Close();
	return result;
}

/**
 * Rewrites the oringinal host binary to the location specified.
 */
BOOL Restore(int argc, char** argv)
{
	ParasiteHost host;

	// Restore Host File: Pull the original file out of the parasite file
	// Parasite --restore <hostfile> <restorefile>
	if(argc < 4)
	{
		printf("Missing params?\n");
		PrintUsage();
		return FALSE;
	}
	
	if(!host.OpenReadOnly(argv[2]))
	{
		printf("Could not load Host File %s\n", argv[2]);
		return FALSE;
	}
	host.SetVerboseOutput(verbose);
	
	host.ReadHeader();
	if(host.RestoreFile(argv[3]))
		printf("Restore to %s Finished\n", argv[3]);
	host.Close();

	return TRUE;
}


/**
 * Application entry
 */
int main(int argc, char** argv)
{
	// Parse command line
	//    0          1           2         3       4           N
	// parasite <operation> <hostfile> [<file1> <file2> ... <fileN>] 
	if(argc < 2)
	{
		PrintUsage();
		return 0;
	}

	if(strcmp(argv[1], "--version") == 0)
	{
		PrintVersion();
		return 0;
	}

	else if(strcmp(argv[1], "--usage") == 0)
	{
		PrintUsage();
		return 0;
	}
	
	else if(strcmp(argv[1], "--help") == 0)
	{
		PrintHelp();	
		return 0;
	}

	parasite_operation operation = ParseOperation(argv[1]);
	verbose = FALSE;
	ParseOptionFlags(argv[1]);
	
	switch(operation)
	{
		case op_create:
			return (!Infect(argc, argv));

		case op_extract:
			switch(argc)
			{
				case 4:
					return (!Extract(argv[2], argv[3]));
				case 5:
					return (!Extract(argv[2], argv[3], argv[4]));					
				default:
				{
					PrintUsage();
					return 1;
				}
			}
			break;


		case op_extractall:
			switch(argc)
			{
				case 3:
					return (!ExtractAll(argv[2]));
				case 4:
					return (!ExtractAll(argv[2], argv[3]));
				default:
				{
					PrintUsage();
					return 1;
				}				   
			}
			
		case op_list:
			return (!List(argc, argv));

		case op_restore:
			return (!Restore(argc, argv));

		case op_add:
			return (!InfectMore(argc, argv));

		case op_remove:
			printf("Not Implemented yet, sorry!");
			return -1;

		case op_multi:
			printf("You must specify only one of the '-xcalrd' operations\n");
			printf("Try 'parasite --help' or 'parasite --usage' for more information.\n");
			return -1;
		   
		case op_none:
			printf("You must specify one of the '-xcalrd' operations\n");
			printf("Try 'parasite --help' or 'parasite --usage' for more information.\n");
			return -1;			
	}
}
