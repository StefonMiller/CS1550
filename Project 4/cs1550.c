#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>

#include "cs1550.h"

//Helper functions
static struct cs1550_directory_entry * find_dir_entry(char dir_name[]);
static struct cs1550_file_entry * find_file(struct cs1550_directory_entry *, char file_name[], char extension[]);
static int check_path(const char *path);
static int get_start_block(char dir_name[]);

//Root block
struct cs1550_root_directory *root;
//.disk file
FILE *f;

/**
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not.
 *
 * `man 2 stat` will show the fields of a `struct stat` structure.
 */
static int cs1550_getattr(const char *path, struct stat *statbuf)
{
	// Clear out `statbuf` first -- this function initializes it.
	memset(statbuf, 0, sizeof(struct stat));

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	// Check if the path is the root directory.
	if (strcmp(path, "/") == 0) 
	{
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;

		return 0;
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	int size;

	// Check if the path is a file.
	if (res == 2 || res == 3) 
	{
		// Regular file
		statbuf->st_mode = S_IFREG | 0666;
	
		// Only one hard link to this file
		statbuf->st_nlink = 1;

		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			//If we find a match, attempt to find a matching file
			struct cs1550_file_entry *matching_file = find_file(matching_directory, filename, extension);
			if(!matching_file)
			{
				free(matching_directory);
				return -ENOENT;
			}
			else
			{
				size = matching_file->fsize;
			}
		}

		// Determine the file size
		statbuf->st_size = size;
		free(matching_directory);
		return 0; // no error
	}

	// Check if the path is a subdirectory.
	if (res == 1) 
	{
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			statbuf->st_mode = S_IFDIR | 0755;
			statbuf->st_nlink = 2;
			free(matching_directory);
			return 0; // no error
		}
	}

	// Otherwise, the path doesn't exist.
	return -ENOENT;
}

/**
 * Called whenever the contents of a directory are desired. Could be from `ls`,
 * or could even be when a user presses TAB to perform autocompletion.
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	//Parse data in
	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	// Check path to find directory we are listing files in
	if (strcmp(path, "/") == 0)
	{
		// Add the current and parent directories no matter what
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		// If we are at root, list all subdirectories
		for (size_t i = 0; i < root->num_directories; i++) 
		{
			filler(buf, root->directories[i].dname, NULL, 0);
		}
		return 0;
	}
	else if(res == 1)
	{
		// Add the current and parent directories no matter what
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		//If res = 1, then we are in a subdirectory and must list the files
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
		
			return -ENOENT;
		}
		//If we find a match, list all files in the directory
		else
		{
			//Initialize an array for the filename + extension(If needed). Set the size to max filename + 1 char for . + max extension + 1 char for \0
			char file[MAX_FILENAME + MAX_EXTENSION + 2];
			for (size_t i = 0; i < matching_directory->num_files; i++) 
			{
				//Copy the filename to the array
				strncpy(file, matching_directory->files[i].fname, (MAX_FILENAME + 1));
				//Check if file extension exists
				if(strcmp(matching_directory->files[i].fext, "") != 0)
				{
					//Append a .
					strncat(file, ".", 2);
					//Append the extension
					strncat(file, matching_directory->files[i].fext, (MAX_EXTENSION + 1));
				}
				
				//Write changes to buffer
				filler(buf, file, NULL, 0);
			}
			free(matching_directory);
			return 0;
			
		}
	}
	else
	{
		//Return -ENOTDIR if the path is not to a directory
		return -ENOTDIR;	
	}
}

/**
 * Creates a directory. Ignore `mode` since we're not dealing with permissions.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	(void) mode;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}
	

	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	//Ensure we are trying to make a directory in the root
	if (res == 1) 
	{
		//Loop through root directories and check if any of their names match the directory name.
		//If so, return -EEXIST
		for (size_t i = 0; i < root->num_directories; i++)
		{
			if (strcmp(directory, root->directories[i].dname) == 0)
			{
				return -EEXIST;
			}
		}

		//Ensure there is space for the new directory
		if (root->num_directories >= MAX_DIRS_IN_ROOT)
		{
			return -ENOSPC;
		}
		else
		{
			//If the directory does not exist and there is space:
			//Copy the new directory name into the next index
			strncpy(root->directories[root->num_directories].dname, directory, (MAX_FILENAME + 1));
			//Set the starting block of the new directory to the last allocated block
			root->directories[root->num_directories].n_start_block = root->last_allocated_block + 1;
			//Increment the # of directories and the last allocated block
			root->num_directories++;
			root->last_allocated_block++;
			//Seek to start of file
			fseek(f, 0, SEEK_SET);
			//Write changes to root block and return success
			fwrite(root, BLOCK_SIZE, 1, f);
			return 0;
		}
	}

	//Can't make directories in any place other than root
	return -EPERM;
}

/**
 * Does the actual creation of a file. `mode` and `dev` can be ignored.
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	
	//Return an error if we didn't parse in 2 or 3 args
	if (res == 2 || res == 3)
	{
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			//If we find a matching directory, attempt to find a matching file
			struct cs1550_file_entry *matching_file = find_file(matching_directory, filename, extension);

			//If the file doesn't exist, attempt to create a new one
			if(!matching_file)
			{
				//If there is enough space for the file, create it
				if(matching_directory->num_files < MAX_FILES_IN_DIR)
				{
					//Get starting block of directory
					int start_block = get_start_block(directory);

					//Copy file data into the next free file
					strncpy(matching_directory->files[matching_directory->num_files].fname, filename, (MAX_FILENAME + 1));
					//Add extension to file if it exists
					if(res == 3)
					{
						strncpy(matching_directory->files[matching_directory->num_files].fext, extension, (MAX_EXTENSION + 1));

					}
					matching_directory->files[matching_directory->num_files].fsize = 0;
					matching_directory->files[matching_directory->num_files].n_index_block = root->last_allocated_block + 1;

					//Seek to and read the index block into a variable of type struct cs1550_index_block. Write the value of the last_allocated_block
					// as the first entry in the index block array. Increment last_allocated_block for the first data block of the file
					struct cs1550_index_block *index = malloc(sizeof(struct cs1550_index_block));
					memset(index, 0, sizeof(struct cs1550_index_block));
					//fseek to the directory block # by starting from 0 + index block * block size
					fseek(f, ((root->last_allocated_block + 1) * BLOCK_SIZE), SEEK_SET);
					//Read data into index block
					fread(index, BLOCK_SIZE, 1, f);

					//Increment last allocated block in root
					root->last_allocated_block++;

					//Allocate a block for the first data block as well
					index->entries[0] = root->last_allocated_block + 1;

					//Increment last_allocated_block again
					root->last_allocated_block++;

					//Increment the number of files in the directory
					matching_directory->num_files++;

					//Write changes to directory entry back to disk
					fseek(f, start_block * BLOCK_SIZE, SEEK_SET);
					fwrite(matching_directory, BLOCK_SIZE, 1, f);

					//Write changes to root back to disk
					fseek(f, 0, SEEK_SET);
					fwrite(root, BLOCK_SIZE, 1, f);

					//Write changes to index block to disk
					fseek(f, ((root->last_allocated_block - 1) * BLOCK_SIZE), SEEK_SET);
					fwrite(index, BLOCK_SIZE, 1, f);

					free(matching_directory);
					free(matching_file);
					free(index);
					return 0;


				}
				//Return an error if there isn't enough space
				else
				{
					free(matching_directory);
					return -ENOSPC;
				}
			}
			//If the file exists, return an error
			else
			{
				free(matching_directory);
				return -EEXIST;
			}
		}
	}
	else
	{
		return -EPERM;
	}
}

/**
 * Read `size` bytes from file into `buf`, starting from `offset`.
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
		       struct fuse_file_info *fi)
{
	(void) fi;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	//Ensure path contains a path and file name
	if(res == 2 || res == 3)
	{
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			//If we find a matching directory, attempt to find a matching file
			struct cs1550_file_entry *matching_file = find_file(matching_directory, filename, extension);
			if(!matching_file)
			{
				//Return an error if the file doesn't exist
				free(matching_directory);
				return -ENOENT;
			}
			else
			{
				//Read the index block
				struct cs1550_index_block *index = malloc(sizeof(struct cs1550_index_block));
				fseek(f, (matching_file->n_index_block * BLOCK_SIZE), SEEK_SET);
				fread(index, BLOCK_SIZE, 1, f);

				//Read the data block
				struct cs1550_data_block *data = malloc(sizeof(struct cs1550_data_block));

				size_t temp_size = 0;
				while(temp_size != size)
				{
					
					//Use the offset parameter to determine the index inside the array of data blocks inside the index block (offset / block size)
					int curr_index = (offset + temp_size) / BLOCK_SIZE;

					//Calculate the current offset to determine the data block to access
					int curr_offset = (offset + temp_size) % BLOCK_SIZE;

					//Calculate the amount of bytes to read for the current iteration
					int curr_size = BLOCK_SIZE;
					if((size - temp_size) < BLOCK_SIZE)
					{
						if((size - temp_size) + curr_offset > BLOCK_SIZE)
						{
							curr_size = BLOCK_SIZE - curr_offset;
						}
						else
						{
							curr_size = size - temp_size;

						}
					}

					//If the index entry is empty, skip reading data
					if(index->entries[curr_index] == 0)
					{
						temp_size += curr_size;
						continue;
					}
					//Seek to current offset and read the block in
					fseek(f, (index->entries[curr_index] * BLOCK_SIZE), SEEK_SET);
					fread(data, BLOCK_SIZE, 1, f);

					//Copy the data into the buffer
					memcpy(buf + temp_size, ((char*)data) + curr_offset, curr_size);

					//Increment the number of bytes copied
					temp_size += curr_size;

				}
				
				//if you need to read the next data block(s), retrieve the block number(s) from the index block
				//(offset % block size) + size > remaining block size

				free(matching_directory);
				free(index);
				free(data);
				return size;
			}
		}
	}
	else
	{
		return -EISDIR;
	}
}

/**
 * Write `size` bytes from `buf` into file, starting from `offset`.
 */
static int cs1550_write(const char *path, const char *buf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	(void) fi;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	//Ensure path contains a path and file name
	if(res == 2 || res == 3)
	{
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			//If we find a matching directory, attempt to find a matching file
			struct cs1550_file_entry *matching_file = find_file(matching_directory, filename, extension);
			if(!matching_file)
			{
				//Return an error if the file doesn't exist
				free(matching_directory);
				return -ENOENT;
			}
			else
			{
				//Read the index block
				struct cs1550_index_block *index = malloc(sizeof(struct cs1550_index_block));
				fseek(f, (matching_file->n_index_block * BLOCK_SIZE), SEEK_SET);
				fread(index, BLOCK_SIZE, 1, f);

				//Read the data block
				struct cs1550_data_block *data = malloc(sizeof(struct cs1550_data_block));

				size_t temp_size = 0;
				while(temp_size != size)
				{
					
					//Use the offset parameter to determine the index inside the array of data blocks inside the index block (offset / block size)
					int curr_index = (offset + temp_size) / BLOCK_SIZE;

					//Calculate the offset for the current data block
					int curr_offset = (offset + temp_size) % BLOCK_SIZE;

					//Calculate the amount of bytes to read for the current iteration
					int curr_size = BLOCK_SIZE;
					if((size - temp_size) < BLOCK_SIZE)
					{
						if((size - temp_size) + curr_offset > BLOCK_SIZE)
						{
							curr_size = BLOCK_SIZE - curr_offset;
						}
						else
						{
							curr_size = size - temp_size;

						}
					}


					//If the index entry is empty, attempt allocate a new block 
					if(index->entries[curr_index] == 0)
					{
						//If there is space, allocate a new data block
						index->entries[curr_index] = root->last_allocated_block + 1;

						//Increment last_allocated_block again
						root->last_allocated_block++;

						//Write changes to root back to disk
						fseek(f, 0, SEEK_SET);
						fwrite(root, BLOCK_SIZE, 1, f);

						//Write changes to index block to disk
						fseek(f, (matching_file->n_index_block * BLOCK_SIZE), SEEK_SET);
						fwrite(index, BLOCK_SIZE, 1, f);
						
					}
					//Seek to and read the current data block in
					fseek(f, (index->entries[curr_index] * BLOCK_SIZE), SEEK_SET);
					fread(data, BLOCK_SIZE, 1, f);

					//Copy buffer contents into the data block
					memcpy(((char*)data) + curr_offset, buf + temp_size, curr_size);

					//Write changes back to data block
					fseek(f, (index->entries[curr_index] * BLOCK_SIZE), SEEK_SET);
					fwrite(data, BLOCK_SIZE, 1, f);
					 
					//Increment the number of bytes copied
					temp_size += curr_size;

				}
				
				free(index);
				free(data);
				//Increment the file size and write the changes before returning
				if(offset == 0)
				{
					//If we are at the start of the file, don't add to the size
					//This fixes the case when we overwrite the 'asdf\n' data from the first test
					//in script 3
					matching_file->fsize = size;
				}
				else
				{
					//If we aren't writing from the beginning, add to the size
					matching_file->fsize += size;
				}
				fseek(f, (get_start_block(directory) * BLOCK_SIZE), SEEK_SET);
				fwrite(matching_directory, BLOCK_SIZE, 1, f);
				free(matching_directory);
				return size;
			}
		}
	}
	else
	{
		return -EISDIR;
	}
}

/**
 * Called when we open a file.
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) fi;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];

	//Check if the path is valid
	if(check_path(path) == 0)
	{
		return -ENAMETOOLONG;
	}

	int res;
	res = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	

	if(res == 1)
	{
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		//If the directory isn't found return an error
		if(!matching_directory)
		{
			return -ENOENT;
		}
		//If the directory is found return success
		else
		{
			return 0;
		}
	}
	else if(res == 2 || res == 3)
	{
		//Attempt to find matching directory in the root block
		struct cs1550_directory_entry *matching_directory = find_dir_entry(directory);
		if(!matching_directory)
		{
			return -ENOENT;
		}
		else
		{
			//If we find a matching directory, attempt to find a matching file
			struct cs1550_file_entry *matching_file = find_file(matching_directory, filename, extension);

			//If the file doesn't exist, return an error
			if(!matching_file)
			{
				return -ENOENT;
			}
			//If the file exists, return success
			else
			{
				return 0;
			}
		}
	}
	else
	{
		//If sscanf didn't parse anything then the path is invalid
		return -ENOENT;
	}
	
}

/**
 * This function should be used to open and/or initialize your `.disk` file.
 */
static void *cs1550_init(struct fuse_conn_info *fi)
{
	(void) fi;
	//Read in first disk block(root)
	root = malloc(BLOCK_SIZE);
	f = fopen(".disk", "rb+");
	if (f != NULL)
	{
		fread(root, BLOCK_SIZE, 1, f);
	}
	return NULL;
}

/**
 * This function should be used to close the `.disk` file.
 */
static void cs1550_destroy(void *args)
{
	(void) args;
	//Free the root node and close the .disk file
	free(root);
	fclose(f);
}

/**
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush(const char *path, struct fuse_file_info *fi)
{	
	(void) path;
	(void) fi;
	// Success!
	return 0;
}

/**
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
	return 0;
}

/**
 * Called when a new file is created (with a 0 size) or when an existing file
 * is made shorter. We're not handling deleting files or truncating existing
 * ones, so all we need to do here is to initialize the appropriate directory
 * entry.
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;
	return 0;
}

/**
 * Deletes a file.
 */
static int cs1550_unlink(const char *path)
{
	(void) path;
	return 0;
}

/*
 * Register our new functions as the implementations of the syscalls.
 */
static struct fuse_operations cs1550_oper = {
	.getattr	= cs1550_getattr,
	.readdir	= cs1550_readdir,
	.mkdir		= cs1550_mkdir,
	.rmdir		= cs1550_rmdir,
	.read		= cs1550_read,
	.write		= cs1550_write,
	.mknod		= cs1550_mknod,
	.unlink		= cs1550_unlink,
	.truncate	= cs1550_truncate,
	.flush		= cs1550_flush,
	.open		= cs1550_open,
	.init		= cs1550_init,
	.destroy	= cs1550_destroy,
};

/*
 * Don't change this.
 */
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &cs1550_oper, NULL);
}

/**
	Loop though the root block and return the starting block of the directory name requested
**/
static int get_start_block(char dir_name[])
{
	//Loop through root directories
	for (size_t i = 0; i < root->num_directories; i++)
	{ 
		//Check if any of the directories match the requested one
		if (strcmp(dir_name, root->directories[i].dname) == 0)
		{
			//Copy the starting block number into an int
			return root->directories[i].n_start_block;
		}
	}
	//If no match found, return null
	return 0;
}


/**
	Loops through the root block and returns the directory entry matching the given name
**/
static struct cs1550_directory_entry * find_dir_entry(char dir_name[])
{
	//Loop through root directories
	for (size_t i = 0; i < root->num_directories; i++)
	{ 
		//Check if any of the directories match the requested one
		if (strcmp(dir_name, root->directories[i].dname) == 0)
		{
			//Copy the starting block number into an int
			int start = root->directories[i].n_start_block;
			//Pointer to matching directory entry
			struct cs1550_directory_entry *dir = malloc(sizeof(struct cs1550_directory_entry));
			//fseek to the directory block # by starting from 0 + # of blocks * block size
			fseek(f, start * BLOCK_SIZE, SEEK_SET);
			//Read data into directory entry and return it
			fread(dir, BLOCK_SIZE, 1, f);
			return dir;
		}
	}
	//If no match found, return null
	return NULL;
}

/**
	Loop through the files in the file entry and return a the matching file, if any
**/
static struct cs1550_file_entry * find_file(struct cs1550_directory_entry *dir, char file_name[], char extension[])
{
	
	//Loop through all files in the directory
	for (size_t i = 0; i < dir->num_files; i++)
	{
		//Check if the file names match
		if((strcmp(file_name, dir->files[i].fname) == 0) && (strcmp(extension, dir->files[i].fext) == 0))
		{
			//If we have a match, return the file
			return &(dir->files[i]);

		}
	}
	//If no match found, return null
	return NULL;
}

/**
	Loop through the path to ensure all arguments are the correct length
**/
static int check_path(const char *path)
{
	int valid_dir = 0;
	int delimiter_index = 0;
	int valid_file = 0;
	//Loop through the directory portion of the path(1 to MAX_FILENAME + 2) and ensure it is proper length
	for(int i = 1; i < (MAX_FILENAME + 2); i++)
	{
		//If we find a / character before exiting the loop then it is valid. Assign the index of the / character to our delimeter variable
		if(path[i] == '/')
		{
			valid_dir = 1;
			delimiter_index = i;
			break;
		}
		//If we find a null terminator before the max dir length, then the directory is also valid and we can return since the path has ended
		else if(path[i] == '\0')
		{
			return 1;
		}		
	}

	//Ensure the directory is valid before continuing
	if(valid_dir == 0)
	{
		return 0;
	}

	//Loop from the index of the first / character until the max  filename length 
	for(int i = delimiter_index + 1; i < (delimiter_index + MAX_FILENAME + 2); i++)
	{
		//If we find a . character before exiting the loop then the filename is valid. Assign the index of the . character to the delimiter variable
		if(path[i] == '.')
		{
			valid_file = 1;
			delimiter_index = i;
			break;
		}
		//If we find a null terminator before the max file length, then the file is valid and we can return since the path has ended
		else if(path[i] == '\0')
		{
			return 1;
		}
	}

	//Ensure the filename is valid before continuing
	if(valid_file == 0)
	{
		return 0;
	}

	//Loop from the index of the . until the end of the path
	for(int i = delimiter_index + 1; i < (delimiter_index + MAX_EXTENSION + 2); i++)
	{
		//If we find a null terminator before the end of the path, then the extension is valid
		if(path[i] == '\0')
		{
			return 1;
		}
	}

	//If we don't see a null terminator before exiting the loop, then the extension is invalid
	return 0;

}