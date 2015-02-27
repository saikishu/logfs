# logfs
simulation of log structured file system

# Build instructions

- Run `make` to compile and generate `logfs` binary
- `./logfs` to run the program

# Commands
- First two commands should set disk capacity and allowed block size once in following order.

> Set disk capacity: size is integer and units are MB or GB or TB. 

> Eg: `diskCapacity(5GB)` Output: `Disk Size set to: 5GB`

```
diskCapacity(<size> <MB|GB|TB>)

```
> Set block size: size is an integer and units are KB or MB. Eg: `blockSize(4KB)` Output: `Block Size set to: 4KB Number of Blocks: 1310720`

> Block size cannot exceed disk capacity
 
```
blockSize(<size> <KB|MB>)
```

- Rest of the commands can be used any number of times and can follow any order

> Create directory: To create dir with one or more paths. Eg: `mkdir(hello,world)` Output: `Created directory: /hello/ Created directory: /world/`

> Paths can be absolute or relative 

```
mkdir(<path> {, <path>})
```

> Change directory: To set current dir to a given path. Eg: `chdir(hello)` Output: `Current dir: /hello/`

```
chdir(<path>)
```

> Write a file: To allocate memory to a file. `file` is string file name, `size` is integer followed by units 
> Eg: `write(magic,10MB)` Output: `/hello/magic, 3, 0x0, 10240KB` Displays: file name, file id, memory address, file size

> Defragmentation is performed if continuous memory is not available.
 
```
write(<file>, <size> <B|KB|MB|GB>)
```
> Read file info: Shows file name, file id, memory address, file size
> Eg: `read(magic)` Output: `/hello/magic, 3, 0x0, 10240KB`

```
read(<file>)
```

# Notes
- Current directory starts with the root `/`
- Syntax is strictly checked.
- Paths can be relative or absolute.

