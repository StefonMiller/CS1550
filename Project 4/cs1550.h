#ifndef CS1550_H
#define CS1550_H

#include <assert.h>
#include <stdlib.h>

/* Size of a disk block */
#define BLOCK_SIZE	512

/* We'll use 8.3 filenames */
#define MAX_FILENAME	8
#define MAX_EXTENSION	3

/* PACKED structures must not have any alignment bytes. */
#define PACKED		__attribute__((packed))


/*
 * Regular files and subdirectories.
 */

#define MAX_FILES_IN_DIR ((BLOCK_SIZE - sizeof(size_t)) / sizeof(struct cs1550_file_entry))
#define DIRECTORY_PADDING (BLOCK_SIZE - sizeof(size_t) - MAX_FILES_IN_DIR*sizeof(struct cs1550_file_entry))

struct PACKED cs1550_file_entry {
	/* File name, plus extra space for the null terminator */
	char fname[MAX_FILENAME + 1];

	/* File extension, plus extra space for the null terminator */
	char fext[MAX_EXTENSION + 1];

	/* Size of the file, in bytes */
	size_t fsize;

	/* Block number of the file's index block in the `.disk` file */
	size_t n_index_block;
};

struct cs1550_directory_entry {
	/* Number of files in directory. Must be less than MAX_FILES_IN_DIR */
	size_t num_files;

	/* The actual files */
	struct cs1550_file_entry files[MAX_FILES_IN_DIR];

	/* Padding so the entry is one block large. Don't use this field. */
	char __padding[DIRECTORY_PADDING];
};



/*
 * The root directory and all of its subdirectories.
 */

#define MAX_DIRS_IN_ROOT ((BLOCK_SIZE - 2*sizeof(size_t)) / sizeof(struct cs1550_directory))
#define ROOT_PADDING (BLOCK_SIZE - 2*sizeof(size_t) - MAX_DIRS_IN_ROOT*sizeof(struct cs1550_directory))

struct PACKED cs1550_directory {
	/* Directory name, plus extra space for the null terminator */
	char dname[MAX_FILENAME + 1];

	/* Block number of the directory block in the `.disk` file */
	size_t n_start_block;
};

struct cs1550_root_directory {
	/* Block number of the last block that was allocated */
	size_t last_allocated_block;

	/* Number of subdirectories under the root */
	size_t num_directories;

	/* All subdirectories of the root */
	struct cs1550_directory directories[MAX_DIRS_IN_ROOT];

	/* Padding so the entry is one block large. Don't use this field. */
	char __padding[ROOT_PADDING];
};



/*
 * Index and file data blocks.
 */

#define MAX_ENTRIES_IN_INDEX_BLOCK	(BLOCK_SIZE / sizeof(size_t))
#define MAX_DATA_IN_BLOCK		BLOCK_SIZE

struct cs1550_index_block {
	/* Block numbers for each data block. */
	size_t entries[MAX_ENTRIES_IN_INDEX_BLOCK];
};

struct cs1550_data_block {
	/* All space in the block can be used to store file data. */
	char data[MAX_DATA_IN_BLOCK];
};



/*
 * Ensure everything is sized exactly as it should be.
 */

static_assert(sizeof(struct cs1550_directory_entry) == BLOCK_SIZE, "wrong size");
static_assert(sizeof(struct cs1550_root_directory)  == BLOCK_SIZE, "wrong size");
static_assert(sizeof(struct cs1550_index_block)     == BLOCK_SIZE, "wrong size");
static_assert(sizeof(struct cs1550_data_block)      == BLOCK_SIZE, "wrong size");

#endif // CS1550_H
