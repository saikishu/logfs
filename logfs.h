/*
 * Header for logfs
 * 
 * More info:
 * https://github.com/saikishu/logfs/blob/master/README.md
 *
 * Copyright (c) 2015 Sai Kishore
 * Free to use under the MIT license
 * https://github.com/saikishu/logfs/blob/master/LICENSE
 *
 */

/* Includes */

#include <iostream>
#include <string>
#include <map>
#include <regex.h>
#include <pthread.h>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
#include <cstring>

using namespace std;

/*Global Variables and constants*/

string currentDir = "/"; //We start with root as current directory

struct file {
	string path;
	unsigned long long allocatedBlocks;
	unsigned long long allocatedFileSize;
};

map<unsigned long long, file> files; //key: non negative file id; value : fileinfo
long long *memory; //Diskspace divided into blocks 0,1,2 reserved for system. >2 is file id. -1 is empty.

unsigned long long diskSize;
unsigned long long blockSize;
string diskUnit = "";
string blockUnit = "";
unsigned long long blocksCount;

unsigned long long currentFileId = 3; // 0,1,2 reserved for system
unsigned long long currentPos = 0; //current write position

map<string, string> initializeCommands() {
	map < string, string > m;
	m["diskCapacity"] = "diskCapacity";
	m["blockSize"] = "blockSize";
	m["mkdir"] = "mkdir";
	m["chdir"] = "chdir";
	m["read"] = "read";
	m["write"] = "write";
	return m;
}
map<string, string> commandsList = initializeCommands();
string validCommandStartPattern = "abcdefghijklmnopqrstuvwxyz"; //Extend this if commands increase.

map<string, string> directoryMap;

/* Prototypes */

/* Main */
void init();
void setDiskCapacity(string args);
void setBlockSize(string args);
void createDirectory(string args);
void changeDirectory(string args);
void writeFile(string args);
void commitFile(string file, unsigned long long fileSize, string unit);
void defragment();
void resetMemory(unsigned long long fileId);
void readFile(string args);

/* Validators */
bool isComment(string line);
bool isValidCommand(string command);
bool isValidSyntax(string line, string &command, string &args);
bool isNumber(string number);

/* Helpers */
void removeSpaces(string &line);
void ltrim(string &line);
string moveUpDir(string path, int levels);
string getAbsolutePath(string path);
unsigned long long convertSize(unsigned long long size, string fromUnit,
		string toUnit);
bool isMemoryFull();
bool isMemoryEmpty();
unsigned long long findFile(string filepath);
unsigned long long getTotalAvailableBlocks();
void getStartingAddress(unsigned long long fileId, unsigned long long &address);

/* Cleanup */
void terminate(string message);

