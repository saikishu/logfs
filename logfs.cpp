/*
 * Log structured file system simulation
 * 
 * More info:
 * https://github.com/saikishu/logfs/blob/master/README.md
 *
 * Copyright (c) 2015 Sai Kishore
 * Free to use under the MIT license
 * https://github.com/saikishu/logfs/blob/master/LICENSE
 *
 */
#include "logfs.h"

/************************************************************************
 Function: main
 Description: Entry point.
 Args: none
 Returns: 0 on successful termination and non 0 on failure.
 Notes:
 1. Initialize to check if first two commands are in order.
 2. Validate each command.
 2.a Illegal inputs: Wrong order, Syntax error, Invalid commands - Terminates the program.
 2.b Other invalid inputs: Skips to next command.
 3. Executes given command.
 ************************************************************************/

int main() {

	init();

	string command = "";
	string line = "";
	string args = "";

	while (std::getline(std::cin, line)) {
		removeSpaces(line);

		if (!isValidSyntax(line, command, args)) {
			terminate("Critical error: Invalid Syntax detected for: " + line);
		}

		if (!isValidCommand(command)) {
			terminate(
					"Error: Invalid command entered:" + command
							+ "\nNot a supported command. Check syntax and list of commands.");
		}

		//Prevent setting diskcapacity and blocksize again
		if (commandsList["diskCapacity"].compare(command) == 0) {
			cout << "Error: Disk Capacity already set. " << endl;
			cout << "Skipping to next command..." << endl;
		} else if (commandsList["blockSize"].compare(command) == 0) {
			cout << "Error: Block Size already set. " << endl;
			cout << "Skipping to next command..." << endl;
		} else if (commandsList["mkdir"].compare(command) == 0) {
			createDirectory(args);
		} else if (commandsList["chdir"].compare(command) == 0) {
			changeDirectory(args);
		} else if (commandsList["read"].compare(command) == 0) {
			readFile(args);
		} else if (commandsList["write"].compare(command) == 0) {
			writeFile(args);
		}
	}

	//Handle memory leaks
	delete[] memory;
	return 0;
}

/************************************************************************
 Function: init
 Description:
 Initializer makes sure first two commands are diskCapacity() & blockSize()
 Initializes memory block which is used for reading and writing.
 Args: none.
 Returns: none.
 Notes: Either terminates or initializes.
 ************************************************************************/

void init() {

	int i = 0; //i=2 then done with first two commands.
	string line = "";
	string command = "";
	string args = "";

	while (std::getline(std::cin, line)) {
		removeSpaces(line); // Normalize string. data can have space in args. Eg: 4 MB instead of 4MB

		//Ignore if line is comment
		if (!isComment(line)) {

			if (!isValidSyntax(line, command, args)) {
				terminate(
						"Critical error: Invalid Syntax detected for: " + line
								+ "\nFirst two commands must be diskCapacity and blockSize with valid syntax.");
			} else {
				if (i == 0) {
					//first command must be diskCapacity
					if (commandsList["diskCapacity"].compare(command) != 0) {
						terminate(
								"Critical error: Invalid command entered: "
										+ command
										+ "\nFirst command must be: diskCapacity(<size> <MB|GB|TB>)");
					} else {
						setDiskCapacity(args);
					}

				} else {
					//second command must be blockSize
					if (commandsList["blockSize"].compare(command) != 0) {
						terminate(
								"Critical error: Invalid command entered: "
										+ command
										+ "\nSecond command must be: blockSize(<size> <KB|MB>)");
					} else {
						setBlockSize(args);
					}
				}

			}
			i++;
		}
		if (i == 2) {
			//Done with first two commands
			break;
		}
	}
	//save initial dir
	directoryMap[currentDir] = currentDir;

	//Initialize block array
	memory = new long long[blocksCount];
	std::fill_n(memory, blocksCount, -1);

	return;

}

/************************************************************************
 Function: setDiskCapacity
 Description: Sets Disk Capacity from args passed to diskCapacity() command.
 Args:
 args    string      disk capacity with units (format: <size> <MB|GB|TB>)
 Returns: none
 Notes:
 Validates arguments to make sure they are syntactically correct.
 On success, outputs: disk size set message.
 On Failure, terminates program.
 TODO: Consider fraction sizes, Refactor
 ************************************************************************/

void setDiskCapacity(string args) {

	//Validate args: <size><MB|GB|TB>
	//Get last two as unit
	size_t len = args.length();
	if (len == 1 || len == 2) {
		terminate(
				"Critical error: Invalid syntax for diskCapacity: diskCapacity(<size> <MB|GB|TB>). Cannot set diskCapacity");
	}
	string unit = args.substr(len - 2, 2);
	string size = args.substr(0, len - 2);

	if (!isNumber(size)) {
		//This rules out invalid size input. Eg: AAMB, -19GB, 2.1TB
		//Critical syntax error for diskcapacity
		terminate(
				"Critical error: Invalid syntax for diskCapacity: Size must be a whole number. Cannot set diskCapacity");
	}

	if ((string::npos == unit.find("MB")) && (string::npos == unit.find("GB"))
			&& (string::npos == unit.find("TB"))) {
		terminate(
				"Critical error: Invalid syntax for diskCapacity: Unit must be MB|GB|TB. Cannot set diskCapacity");
	}

	diskUnit = unit;
	diskSize = std::stoull(size);

	if (diskSize == 0) {
		//0 Disk error
		terminate(
				"Critical error: diskCapacity cannot be 0. Cannot set diskCapacity");
	}

	cout << "Disk Size set to: " << diskSize << diskUnit << endl;

	return;
}

/************************************************************************
 Function: setBlockSize
 Description: Sets Block size from args passed to blockSize() command
 Args:
 args    string      block size with units (format: <size> <KB|MB>)
 Returns: none
 Notes:
 Validates arguments to make sure they are syntactically correct.
 Makes sure block size <= disk size
 Calculates and sets total number of blocks
 On success, outputs: block size set message.
 On Failure, terminates program.
 TODO: Consider fraction sizes, Refactor
 ************************************************************************/

void setBlockSize(string args) {

	//Validate args: <size><KB|MB>
	//Get last two as unit
	size_t len = args.length();

	if (len == 1 || len == 2) {
		terminate(
				"Critical error: Invalid syntax for blockSize: blockSize(<size> <KB|MB>). Cannot set blockSize.");
	}

	string unit = args.substr(len - 2, 2);
	string size = args.substr(0, len - 2);

	if (!isNumber(size)) {
		//This rules out invalid size input. Eg: AAMB, -19MB, 2.1KB
		//Critical syntax error for diskcapacity
		terminate(
				"Critical error: Invalid syntax for setBlockSize: Size must be a whole number. Cannot set blockSize");
	}

	if ((string::npos == unit.find("KB"))
			&& (string::npos == unit.find("MB"))) {
		terminate(
				"Critical error: Invalid syntax for blockSize: Unit must be KB|MB. Cannot set diskCapacity");
	}

	blockUnit = unit;
	blockSize = std::stoull(size);

	//Block bounds check
	//Block size cannot be greater than disk size.

	unsigned long long temp = 0;
	//Diff units

	//Disk unit is made sure MB/GB/TB and block size is made sure KB/MB.
	temp = convertSize(diskSize, diskUnit, blockUnit);

	if (blockSize == 0) {
		//0 block size error : eliminates divide by 0 error.
		terminate(
				"Critical error: blockSize cannot be 0. Cannot set blockSize");
	}

	if (blockSize > temp) {
		//Critical error
		terminate(
				"Critical error: Block size cannot be greater than disk capacity. ");
	} else {
		if (temp % blockSize != 0) {
			//Critical error
			terminate(
					"Critical error: Invalid block size. Block size should be able to divide disk into integral blocks.");
		}
		blocksCount = temp / blockSize;
	}

	cout << "Block Size set to: " << blockSize << blockUnit << endl;
	cout << "Number of Blocks: " << blocksCount << endl;

	return;

}

/************************************************************************
 Function: createDirectory
 Description: Creates all paths passed as args to mkdir() command
 Args:
 args    string      one or more paths (format: <path> {, <path>})
 Returns:    none
 Notes:
 Stores created dirs in directoryMap
 Creates one or more dirs from absolute or relative paths unless dir already exists.
 On success, outputs each created directory message.
 On failure, skips to next command.
 TODO: Handle paths containing ',' and intentional spaces
 ************************************************************************/

void createDirectory(string args) {

	string temp = "";
	//Check for single or multiple paths
	size_t commapos = args.find_first_of(",");
	if (commapos != string::npos) {
		//multiple paths
		char *cstr_args = new char[args.length() + 1];
		std::strcpy(cstr_args, args.c_str());
		char *token = std::strtok(cstr_args, ",");
		while (token != NULL) {
			//Syntax allows a space after ',''
			temp = token;
			ltrim(temp);
			temp = getAbsolutePath(temp);

			if (temp.find_last_of("/") != temp.length() - 1) {
				temp = temp + "/";
			}

			if (directoryMap.count(temp) == 0) {
				directoryMap[temp] = temp;
				cout << "Created directory: " << temp << endl;
			} else {
				cout << "Directory already exists: " << temp << endl;
			}

			token = strtok(NULL, ",");
		}
		//handle memory leaks
		delete[] cstr_args;
		delete[] token;
	} else {
		//single path
		temp = getAbsolutePath(args);
		;
		if (temp.find_last_of("/") != temp.length() - 1) {
			temp = temp + "/";
		}
		if (directoryMap.count(temp) == 0) {
			directoryMap[temp] = temp;
			cout << "Created directory: " << temp << endl;
		} else {
			cout << "Directory already exists: " << temp << endl;
		}

	}
	return;

}
/************************************************************************
 Function: changeDirectory
 Description: sets current directory to path given in chdir() command
 Args:
 args    string      path (format: <path>)
 Returns:    none
 Notes:
 Considers absolute and relative path
 Checks if directory exists. If yes, changes current dir else skips
 to next command.
 On success, outputs new current dir
 On failure, skips to next command.
 ************************************************************************/

void changeDirectory(string args) {
	//@notes: spaces at end/begin of path is legal and valid

	string temp = getAbsolutePath(args);

	if (temp.find_last_of("/") != temp.length() - 1) {
		temp = temp + "/";
	}

	if (directoryMap.count(temp) == 0) {
		cout << "Directory doesn't exist: " << temp << endl;
		cout << "Skipping to next command..." << endl;
		return;
	}

	currentDir = temp;

	cout << "Current dir: " << currentDir << endl;
	return;
}

/************************************************************************
 Function: writeFile
 Description: Allocates given size to file in memory from args passed to write() command.
 Args:
 args    string      file and size (format: <file>, <size> <B|KB|MB|GB>)
 Returns: none
 Notes:
 Validates syntactical correctness of write() command arguments.
 Considers absolute and relative file path.
 Executes method to commit to memory. (Only simulation)
 On Success, calls method to commit to memory
 On Failure,
 Syntax error: Terminates program
 ************************************************************************/

void writeFile(string args) {

	string unit = "";
	unsigned long long fileSize = 0;

	//Validate args
	size_t commapos = args.find_first_of(",");
	if (commapos == 0 || commapos == string::npos
			|| commapos != args.find_last_of(",")) {
		//Critical error: Bad syntax. Either no commas or more than one comma present.
		terminate(
				"Critical error: Invalid Syntax detected for: write command: write(<file>, <size><B|KB|MB|GB>)");
	}

	char *cstr_args = new char[args.length() + 1];
	std::strcpy(cstr_args, args.c_str());

	//It is guaranteed at this stage that we have only one ','. So 2 tokens.
	char *token = std::strtok(cstr_args, ",");
	//first token is file name
	string file = token;
	token = strtok(NULL, ",");
	//second token is size.
	string size = "";
	if (token) {
		size = token;
		ltrim(size);
	} else {
		terminate(
				"Critical error: Invalid Syntax detected for: write command: write(<file>, <size><B|KB|MB|GB>)");
	}

	delete[] cstr_args;

	//validating size attrib
	size_t len = size.length();

	//only size = 0 is allowed.
	if (len == 1) {
		if (!isNumber(size) || ((isNumber(size)) && (std::stoi(size) != 0))) {
			terminate(
					"Critical error: Invalid Syntax detected for: write command: write(<file>, <size><B|KB|MB|GB>). Only 0 is allowed without units.");
		}

	} else {
		//Unit can be B | KB | MB | GB; 1 or 2 chars
		//Assume it is B
		unit = size.substr(len - 1, 1);

		if (unit.compare("B") != 0) {
			//Fail safe. Everyunit ends with B.
			terminate(
					"Critical error: Invalid syntax for write command: write(<file>, <size><B|KB|MB|GB>). Unit must be B|KB|MB|GB.");
		}

		string tempSize = size.substr(0, len - 1); //Everything except last char

		if (!isNumber(tempSize)) {
			//Two possibilities: either it is invalid + B or assumption is wrong. It can be MB|KB|GB.
			string tempSize2 = size.substr(0, len - 2); //Everything except last two chars
			if (!isNumber(tempSize2)) {
				terminate(
						"Critical error: Invalid syntax for write command: write(<file>, <size><B|KB|MB|GB>). Size must be whole number.");
			} else {
				//Assumption is wrong. However string like 789XB can escape above case.
				unit = size.substr(len - 2, 2);
				if ((string::npos == unit.find("KB"))
						&& (string::npos == unit.find("MB"))
						&& (string::npos == unit.find("GB"))) {
					terminate(
							"Critical error: Invalid syntax for write command: write(<file>, <size><B|KB|MB|GB>): Unit must be B|KB|MB|GB.");
				}
				fileSize = std::stoull(tempSize2);
			}
		} else {
			//Assumption is valid
			fileSize = std::stoull(tempSize);
		}
	}

	file = getAbsolutePath(file);
	commitFile(file, fileSize, unit);

	return;
}

/************************************************************************
 Function: commitFile
 Description: Commits file to memory. File info is passed from write() method.
 Args:
 filepath    string              Absolute file path
 fileSize    unsigned long long  file size to write
 Returns: none
 Notes:
 Performs delete operation if size = 0
 Removes file info from file list
 Marks memory occupied to empty
 Checks for available space to accommodate given file.
 If not continuous but enough space is available calls defragment()
 Writes sequentially
 Writes new file to memory (simulation => stores info in heap)
 If file exists, marks existing memory as empty and sequentially
 writes a new file with same meta data.
 On success, outputs written file info
 On failure, skips to next command.
 ************************************************************************/
void commitFile(string filepath, unsigned long long fileSize, string unit) {

	//if file size = 0 then delete operation on existing file.
	unsigned long long searchFileId = findFile(filepath);
	unsigned long long fileId = currentFileId;
	if (fileSize == 0) {
		//Existing file operation
		if (searchFileId == 0) {
			cout << "No such file exists to write. " << endl;
			cout << "Skipping to next command..." << endl;
			return;
		}
		resetMemory(searchFileId);
		files.erase(searchFileId);
		cout << filepath << ", " << searchFileId << ", " << "DELETED" << ", 0"
				<< blockUnit << endl;
		return;
	}

	//Bounds check
	string normalizedUnit = "B"; //least of <B|KB\MB|GB> vs <MB|GB|TB>
	long double normalizedFileSize = convertSize(fileSize, unit,
			normalizedUnit);
	long double normalizedDiskSize = convertSize(diskSize, diskUnit,
			normalizedUnit);
	long double normalizedBlockSize = convertSize(blockSize, blockUnit,
			normalizedUnit);

	//Check if filesize is greater than total capacity
	if ((normalizedDiskSize / normalizedFileSize) < 1) {
		cout << "Error: Cannot write files greater than disk capacity. "
				<< endl;
		cout << "Skipping to next command..." << endl;
		return;
	}

	unsigned long long requiredBlocks = ceil(
			normalizedFileSize / normalizedBlockSize);
	unsigned long long allocatedFileSize = requiredBlocks * blockSize; //in block units

	//if end is reached then try defragmenting before writing.
	if (currentPos == blocksCount) {
		//either memory full or need defragmentation
		defragment();
	}

	unsigned long long availableBlocks = blocksCount - currentPos; //defragmentation done. If 0 then memory full.

	if (requiredBlocks > availableBlocks) {
		//May not be continuously available
		if (getTotalAvailableBlocks() >= requiredBlocks) {
			//defragmentation will get desired blocks continuously.
			defragment();
			//now, currentPos should point to a continuous memory till end or points to end if full.
			//@todo refactor and bring in a way to check continous memory availability
			availableBlocks = blocksCount - currentPos;
			if (requiredBlocks > availableBlocks) {
				//defrag didnt help. Disk is really full.
				cout << "Not enough memory to write. " << endl;
				cout << "Skipping to next command..." << endl;
				return;
			}
		} else {
			cout << "Not enough memory to write. " << endl;
			cout << "Skipping to next command..." << endl;
			return;
		}
	}

	//At this stage there is enough memory to write

	unsigned long long i = 0;
	file f1 = { };
	f1.path = filepath;
	f1.allocatedBlocks = requiredBlocks;
	f1.allocatedFileSize = allocatedFileSize;

	//check if file exists first
	if (searchFileId != 0) {
		//file exists and id is searchFileId.

		//reset previous memory
		resetMemory(searchFileId);

		//continue to create new block from current pos
		for (i = 0; i < requiredBlocks; i++) {
			//currentPos+requiredBlocks is never out of bounds. Since requiredBlocks <= availableBlocks
			memory[currentPos + i] = searchFileId;
		}

		//update file map
		files[searchFileId] = f1;
		fileId = searchFileId;

	} else {
		//new file
		for (i = 0; i < requiredBlocks; i++) {
			//currentPos+requiredBlocks is never out of bounds. Since requiredBlocks <= availableBlocks
			memory[currentPos + i] = currentFileId;
		}

		files[currentFileId] = f1;
		fileId = currentFileId;
		currentFileId++;
	}

	currentPos = currentPos + requiredBlocks;

	//Print file info
	unsigned long long startAddress = 0;
	getStartingAddress(fileId, startAddress);
	cout << filepath << ", " << fileId << ", 0x" << std::hex << startAddress
			<< ", " << std::dec << allocatedFileSize << blockUnit << endl;

	return;

}

/************************************************************************
 Function: defragment
 Description: Defragment and compacts disk space by adjusting memory blocks
 Args: none
 Returns: none
 Notes:
 Realigns memory blocks such that free space is available from
 current position to end unless memory is fully occupied.
 ************************************************************************/
void defragment() {

	if (isMemoryFull()) {
		//cannot defragment done
		return;
	}
	if (isMemoryEmpty()) {
		//defragment not needed
		return;
	}

	//Design notes: After current position, it is either free space or end of memory.

	long long i = 0;
	long long j = 0;

	for (i = currentPos - 1; i >= 0; i--) {

		if ((memory[i] == -1) && (currentPos == i + 1)) {
			//immediate empty space, pull the currentPos back
			currentPos = i;
		} else {
			if (memory[i] == -1) {
				for (int j = i; j < currentPos; j++) {
					if (currentPos == j + 1) {
						//prevent boundary overflow
						memory[j] = -1;
						currentPos--;
					} else {
						memory[j] = memory[j + 1];
					}
				}

			}
		}
	}

	return;

}

/************************************************************************
 Function: resetMemory
 Description: Sets the blocks occupied by a given file to empty.
 Args:
 fileId  unsigned long long      File Id of the file (from write command with 0 size)
 Returns: none
 Notes:
 All blocks occupied by file are flagged empty.
 ************************************************************************/

void resetMemory(unsigned long long fileId) {
	unsigned long long i = 0;
	for (i = 0; i < blocksCount; i++) {
		if (memory[i] == fileId) {
			memory[i] = -1;
		}
	}
}

/************************************************************************
 Function: readFile
 Description: Reads file info of file from read() command
 Args:
 file    string      path to string from read() command
 Returns: none
 Notes:
 Accepts relative and absolute file paths
 Searches if file exists
 On success, outputs file info.
 On failure, skips to next command.
 ************************************************************************/

void readFile(string file) {
	file = getAbsolutePath(file);
	unsigned long long searchFileId = findFile(file);
	if (searchFileId == 0) {
		cout << "File not found: " << file << endl;
		cout << "Skipping to next command..." << endl;
		return;
	}

	unsigned long long startAddress = 0;
	getStartingAddress(searchFileId, startAddress);

	cout << files[searchFileId].path << ", " << searchFileId << ", 0x"
			<< std::hex << startAddress << ", " << std::dec
			<< files[searchFileId].allocatedFileSize << blockUnit << endl;

	return;
}

/************** Validators ************************************************/

/************************************************************************
 Function: isComment
 Description: Checks if given input string/line is a comment
 Args:
 line    string      input line to check
 Returns:
 true if comment
 false if not a comment
 Notes:
 Comments start with #
 ************************************************************************/

bool isComment(string line) {
	if (line.find_first_of("#") == 0) {
		return true;
	}
	return false;
}

/************************************************************************
 Function: isValidCommand
 Description: Checks if given command is valid.
 Args:
 command     string      input command to validate
 Returns:
 true if command is valid
 false if command is invalid
 Notes:
 Searches global commands list.
 It is a map with command name as unique key
 Todo:
 ************************************************************************/

bool isValidCommand(string command) {

	if (commandsList.count(command) == 0) {
		//Not a supported command
		return false;
	}
	return true;
}

/************************************************************************
 Function: isValidSyntax
 Description: General check for command syntax.
 Args:
 line        string   input full string/command to validate
 command     string&  if validated: command part is stored in this var
 args        string&  if validated: args part is stored in this var
 Returns:
 true if syntax is valid
 false if syntax is invalid
 Notes:
 Arguments check is done at respective command methods.
 It checks for following syntax format: <string>(<string(s)>)
 Rules:
 1. Each command should start with char specified in
 validCommandStartPattern global var
 2. Each command should have '(' and ')' and
 position('(') < position(')')
 3. Between ( and ) are command args which is a string
 and rules within varies based on command.
 4. After ')' only a comment is valid. Rest renders invalid syntax.
 On success,
 dissects the command and extracts command and arguments.
 ************************************************************************/

bool isValidSyntax(string line, string &command, string &args) {

	if (line.find_first_of(validCommandStartPattern) != 0) {
		cout
				<< "Invalid character found at beginning. Check for valid commands list."
				<< endl;
		return false;
	}

	size_t lpos = line.find_first_of('(');
	size_t rpos = line.find_first_of(')');

	if (string::npos == lpos || string::npos == rpos) {
		//either ( or ) missing
		cout << "Bad syntax: Missing parenthesis" << endl;
		return false;
	} else if (lpos > rpos) {
		//'(' occurs after ')'
		cout << "Bad syntax: Bad parenthesis order." << endl;
		return false;
	}
	if (string::npos != rpos) {
		//Something exists after ')'
		string tail = line.substr(rpos + 1);
		ltrim(tail);
		//Ignore if it is only a whitespace or comment
		if ("" != tail && !isComment(tail)) {
			//This means command didn't terminate after ')'
			cout
					<< "Bad syntax: Only comments allowed after closing parenthesis."
					<< endl;
			return false;
		}
	}

	//Valid syntaxt dissect the command.
	command = line.substr(0, lpos);
	args = line.substr(lpos + 1, rpos - lpos - 1);

	return true;
}

/************************************************************************
 Function: isNumber
 Description: Checks if a given string is a whole number in string representation
 Args:
 number  string      input string to check
 Returns:
 true if number
 false if not number
 Notes:
 Checks existence of (0-9)* digits in given string.
 Todo: Extend to fractions, negatives etc.
 ************************************************************************/

/*
 Checks if given string is a number in string representation.
 */
bool isNumber(string number) {

	if (number.find_first_not_of("0123456789") == string::npos) {
		return true;
	}

	return false;
}

/************** Helper Methods ********************************************/

/************************************************************************
 Function: removeSpaces
 Description: Removes space in a given string and compacts it.
 Args:
 line    string&     input string to remove spaces
 Returns: none
 Notes:
 Helps in whitespace free formate convention.
 Todo: Extend to consider valid/volutary spaces.
 ************************************************************************/
void removeSpaces(string &line) {
	line.erase(remove(line.begin(), line.end(), ' '), line.end());
}

/************************************************************************
 Function: ltrim
 Description: Trims all white space present on left of input string
 Args:
 line    string&     input string to trim
 Returns: none
 Notes: Helps in whitespace free formate convention.
 Todo: Used initially before removeSpaces() was introduced. Try to deprecate.
 ************************************************************************/

void ltrim(string &line) {
	line.erase(0, line.find_first_not_of(" \t"));
}

/************************************************************************
 Function: moveUpDir
 Description: Gets the parent directory present desired levels up the given dir.
 Args:
 path    string      parent/reference path as string
 levels  int         number of levels to move up
 Returns:
 string  returns parent path after moving giving levels up
 Notes:
 Helps in organizing and moving around directories.
 Helps dealing with relative paths.
 ************************************************************************/

string moveUpDir(string path, int levels) {
	if ((path.compare("/") == 0) || (levels == 0)) {
		//no moving up.
		return path;
	}
	//Make sure path terminates with "/"
	if (path.find_last_of("/") != path.length() - 1) {
		path = path + "/";
	}
	int n = 0;
	for (size_t pos = path.find("/"); pos != std::string::npos;
			pos = path.find("/", pos + 1)) {
		++n;
	}
	if (levels >= n - 1) {
		return "/";
	} else {
		for (int i = 0; i < levels + 1; i++) {
			size_t pos = path.find_last_of("/");
			path = path.substr(0, pos);
		}
		return path + "/";
	}
	return path;
}

/************************************************************************
 Function: getAbsolutePath
 Description: Gets absolute path for a given path based on current dir
 Args:
 path    string      input path to get absolute path
 Returns:
 string  Absolute path for given path
 Notes:
 If already absolute then returns the same path
 Handles relative paths
 Uses moveUpDir() method to process relative paths.
 Handles special paths ., ..
 Helps in path processing and handling files and dirs.
 Todo: Handle voluntary space in path
 ************************************************************************/
string getAbsolutePath(string path) {
	//Remove leading space.
	ltrim(path);
	if (path.find_first_of("/") == 0) {
		//already absolute path
		return path;
	} else if (path.find_first_of(".") == 0) {
		//relative path with .
		//Cases: ., .., ../../..(n times), if n exceeds limit then root directory and .<string>: dot followed by string.
		if (path.compare(".") == 0) {
			return currentDir;
		} else if (path.compare("..") == 0) {
			return moveUpDir(currentDir, 1);
		} else if (path.substr(0, 3).compare("../") == 0) {
			int levels = 0;
			size_t lastpos = 0;
			for (size_t pos = path.find("../"); pos != std::string::npos; pos =
					path.find("../", pos + 3)) {
				++levels;
				lastpos = pos;
			}
			return moveUpDir(currentDir, levels) + "" + path.substr(lastpos + 3); //append rest of string after final ../
		} else {
			//valid paths like "..hello" here...
			return currentDir + "" + path;
		}
	} else {
		//simple relative path
		return currentDir + "" + path;
	}

}

/************************************************************************
 Function: convertSize
 Description: Converts higher order memory sizes to lower order sizes
 Args:
 size    unsigned long long      Numeric size to convert
 fromUnit    string              One of B|KB\MB\GB\TB
 toUnit      string              One of B|KB\MB\GB\TB
 Returns:
 converted size in toUnit specified
 0 if unsupported conversion
 Notes:
 From unit is higher than or equal to To unit
 Helps in normalizations, memory operations, address calculations etc..,
 Todo:
 Extend to provide conversions from any to any order.
 Refactor
 ************************************************************************/

unsigned long long convertSize(unsigned long long size, string fromUnit,
		string toUnit) {

	if (toUnit.compare("B") == 0) {
		if (fromUnit.compare("B") == 0) {
			return size;
		} else if (fromUnit.compare("KB") == 0) {
			return size * 1024;
		} else if (fromUnit.compare("MB") == 0) {
			return size * 1024 * 1024;
		} else if (fromUnit.compare("GB") == 0) {
			return size * 1024 * 1024 * 1024;
		} else if (fromUnit.compare("TB") == 0) {
			return size * 1024 * 1024 * 1024 * 1024;
		} else {
			//unsupported conversion
			return 0;
		}
	}

	if (toUnit.compare("KB") == 0) {
		if (fromUnit.compare("KB") == 0) {
			return size;
		} else if (fromUnit.compare("MB") == 0) {
			return size * 1024;
		} else if (fromUnit.compare("GB") == 0) {
			return size * 1024 * 1024;
		} else if (fromUnit.compare("TB") == 0) {
			return size * 1024 * 1024 * 1024;
		} else {
			return 0;
		}
	}

	if (toUnit.compare("MB") == 0) {
		if (fromUnit.compare("MB") == 0) {
			return size;
		} else if (fromUnit.compare("GB") == 0) {
			return size * 1024;
		} else if (fromUnit.compare("TB") == 0) {
			return size * 1024 * 1024;
		} else {
			return 0;
		}
	}

	if (toUnit.compare("GB") == 0) {
		if (fromUnit.compare("GB") == 0) {
			return size;
		} else if (fromUnit.compare("TB") == 0) {
			return size * 1024;
		} else {
			return 0;
		}
	}

	if (toUnit.compare("TB") == 0) {
		if (fromUnit.compare("TB") == 0) {
			return size;
		} else {
			return 0;
		}
	}

	return 0;
}

/************************************************************************
 Function: isMemoryFull
 Description: Checks if all memory blocks are filled.
 Args: none
 Returns:
 true if full
 false if not full
 Notes:
 Stops checking if it finds any empty block
 If full, current position is set to number of blocks.
 Helps in writing and defragmentation.
 ************************************************************************/
bool isMemoryFull() {

	unsigned long long firstHit = blocksCount;
	long long i = 0;
	for (i = blocksCount - 1; i >= 0; i--) {
		if ((memory[i] == -1) && (firstHit == blocksCount)) {
			firstHit = i;
			break;
		}
	}
	if (firstHit == blocksCount) {
		currentPos = blocksCount;
		return true; //memory full
	}
	return false;
}

/************************************************************************
 Function: isMemoryEmpty
 Description: Checks if entire memory blocks are empty.
 Args: none
 Returns:
 true if empty
 false if not empty
 Notes:
 Stops checking if it finds any occupied block
 If empty, current position is set to start.
 Helps in reading, writing and defragmentation.
 ************************************************************************/

bool isMemoryEmpty() {

	unsigned long long firstHit = blocksCount;
	long long i = 0;

	for (i = blocksCount - 1; i >= 0; i--) {

		if ((memory[i] != -1)) {
			firstHit = i;
			break;
		}
	}

	if (firstHit == blocksCount) {
		currentPos = 0;
		return true; //memory empty
	}

	return false;
}

/************************************************************************
 Function: findFile
 Description: Finds file in file list
 Args:
 filepath    string      Absolute path to file
 Returns:
 unsigned long long      id of file found
 0 if not found
 Notes:
 File list is a map with file id's as unique keys.
 Helps in writing, reading and updating files.
 Todo:
 ************************************************************************/

unsigned long long findFile(string filepath) {

	for (map<unsigned long long, file>::iterator i = files.begin();
			i != files.end(); ++i) {
		if ((((*i).second.path).compare(filepath)) == 0) {
			return (*i).first;
		}
	}
	return 0;
}

/************************************************************************
 Function: getTotalAvailableBlocks
 Description: Gets the total number of available blocks in memory
 Args: none
 Returns:
 unsigned long long      number of blocks available
 Notes:
 Independent of fragmentation.
 Counts all empty blocks and resulting number may not be continuous memory.
 ************************************************************************/

unsigned long long getTotalAvailableBlocks() {
	if (isMemoryFull()) {
		return 0;
	}
	unsigned long long i = 0;
	unsigned long long count = 0;
	for (i = 0; i < blocksCount; i++) {
		if (memory[i] == -1) {
			count++;
		}
	}
	return count;
}

/************************************************************************
 Function: getStartingAddress
 Description: Gets the starting address of a file
 Args:
 fileId      unsigned long long      Id of the file to get.
 address     unsigned long long&     stores the starting address
 Returns: none
 Notes:
 Searches memory blocks and calculates address based on position
 Helps in file info output
 ************************************************************************/
void getStartingAddress(unsigned long long fileId,
		unsigned long long &address) {

	unsigned long long blockPosition = 0;

	for (unsigned long long i = 0; i < blocksCount; i++) {
		if (memory[i] == fileId) {
			//get the first position of file id in memory
			blockPosition = i;
			break;
		}
	}

	unsigned long long blockSizeInBytes = convertSize(blockSize, blockUnit,
			"B");
	address = blockPosition * blockSizeInBytes;
	return;
}

/*******************  Cleanup  **************************************************/

/************************************************************************
 Function: terminate
 Description: Terminates the program and cleanups memory.
 Args:
 message     string      message to output before terminating
 Returns: none
 Notes:
 Controlled termination
 Cleanups memory and prevents leaks.
 Exits with EXIT_FAILURE
 ************************************************************************/

void terminate(string message) {
	cout << message << endl;
	cout << "Terminating..." << endl;
	if (memory) {
		delete[] memory;
	}
	exit (EXIT_FAILURE);
}

