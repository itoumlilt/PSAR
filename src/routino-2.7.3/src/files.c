/***************************************
 Functions to handle files.

 Part of the Routino routing software.
		      ******************//******************
 This file Copyright 2008-2014 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "files.h"


/*+ A structure to contain the list of memory mapped files. +*/
struct mmapinfo {
	const char *filename;	/*+ The name of the file (the index of the list). + */
	int fd;			/*+ The file descriptor used when it was opened. + */
	void *address;		/*+ The address the file was mapped to. + */
	size_t length;		/*+ The length of the file. + */
};

/*+ The list of memory mapped files. +*/
static struct mmapinfo *mappedfiles;

/*+ The number of mapped files. +*/
static int nmappedfiles = 0;


#define BUFFLEN 4096

/*+ A structure to contain the list of file buffers. +*/
struct filebuffer {
	int fd;			/*+ The file descriptor used when it was opened. + */
	char buffer[BUFFLEN];	/*+ The data buffer. + */
	size_t pointer;		/*+ The read/write pointer for the file buffer. + */
	size_t length;		/*+ The read pointer for the file buffer. + */
	int reading;		/*+ A flag to indicate if the file is for reading. + */
};

/*+ The list of file buffers. +*/
static struct filebuffer **filebuffers = NULL;

/*+ The number of allocated file buffer pointers. +*/
static int nfilebuffers = 0;


/* Local functions */

static void CreateFileBuffer(int fd, int read_write);


/*++++++++++++++++++++++++++++++++++++++
  Return a filename composed of the dirname, prefix and name.

  char *FileName Returns a pointer to memory allocated to the filename.

  const char *dirname The directory name.

  const char *prefix The file prefix.

  const char *name The main part of the name.
  ++++++++++++++++++++++++++++++++++++++*/

char *FileName(const char *dirname, const char *prefix, const char *name)
{
	char *filename =
	    (char *) malloc((dirname ? strlen(dirname) : 0) + 1 +
			    (prefix ? strlen(prefix) : 0) + 1 +
			    strlen(name) + 1);

	sprintf(filename, "%s%s%s%s%s", dirname ? dirname : "",
		dirname ? "/" : "", prefix ? prefix : "",
		prefix ? "-" : "", name);

	return (filename);
}


/*++++++++++++++++++++++++++++++++++++++
  Open a file read-only and map it into memory.

  void *MapFile Returns the address of the file or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

void *MapFile(const char *filename)
{
	int fd;
	struct stat buf;
	off_t size;
	void *address;

	/* Open the file */

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Get its size */

	if (stat(filename, &buf)) {
		fprintf(stderr, "Cannot stat file '%s' [%s].\n", filename,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	size = buf.st_size;

	/* Map the file */

	address = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

	if (address == MAP_FAILED) {
		close(fd);

		fprintf(stderr,
			"Cannot mmap file '%s' for reading [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_mmap(size);

	/* Store the information about the mapped file */

	mappedfiles =
	    (struct mmapinfo *) realloc((void *) mappedfiles,
					(nmappedfiles +
					 1) * sizeof(struct mmapinfo));

	mappedfiles[nmappedfiles].filename = filename;
	mappedfiles[nmappedfiles].fd = fd;
	mappedfiles[nmappedfiles].address = address;
	mappedfiles[nmappedfiles].length = size;

	nmappedfiles++;

	return (address);
}


/*++++++++++++++++++++++++++++++++++++++
  Open a file read-write and map it into memory.

  void *MapFileWriteable Returns the address of the file or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

void *MapFileWriteable(const char *filename)
{
	int fd;
	struct stat buf;
	off_t size;
	void *address;

	/* Open the file */

	fd = open(filename, O_RDWR);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading and writing [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Get its size */

	if (stat(filename, &buf)) {
		fprintf(stderr, "Cannot stat file '%s' [%s].\n", filename,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	size = buf.st_size;

	/* Map the file */

	address =
	    mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (address == MAP_FAILED) {
		close(fd);

		fprintf(stderr,
			"Cannot mmap file '%s' for reading and writing [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_mmap(size);

	/* Store the information about the mapped file */

	mappedfiles =
	    (struct mmapinfo *) realloc((void *) mappedfiles,
					(nmappedfiles +
					 1) * sizeof(struct mmapinfo));

	mappedfiles[nmappedfiles].filename = filename;
	mappedfiles[nmappedfiles].fd = fd;
	mappedfiles[nmappedfiles].address = address;
	mappedfiles[nmappedfiles].length = size;

	nmappedfiles++;

	return (address);
}


/*++++++++++++++++++++++++++++++++++++++
  Unmap a file and close it.

  void *UnmapFile Returns NULL (for similarity to the MapFile function).

  const void *address The address of the mapped file in memory.
  ++++++++++++++++++++++++++++++++++++++*/

void *UnmapFile(const void *address)
{
	int i;

	for (i = 0; i < nmappedfiles; i++)
		if (mappedfiles[i].address == address)
			break;

	if (i == nmappedfiles) {
		fprintf(stderr,
			"The data at address %p was not mapped using MapFile().\n",
			address);
		exit(EXIT_FAILURE);
	}

	/* Close the file */

	close(mappedfiles[i].fd);

	/* Unmap the file */

	munmap(mappedfiles[i].address, mappedfiles[i].length);

	log_munmap(mappedfiles[i].length);

	/* Shuffle the list of files */

	nmappedfiles--;

	if (nmappedfiles > i)
		memmove(&mappedfiles[i], &mappedfiles[i + 1],
			(nmappedfiles - i) * sizeof(struct mmapinfo));

	return (NULL);
}


/*++++++++++++++++++++++++++++++++++++++
  Open an existing file on disk for reading.

  int SlimMapFile Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

int SlimMapFile(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	CreateFileBuffer(fd, 0);

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Open an existing file on disk for reading or writing.

  int SlimMapFileWriteable Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

int SlimMapFileWriteable(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_RDWR);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading and writing [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	CreateFileBuffer(fd, 0);

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Close a file on disk.

  int SlimUnmapFile returns -1 (for similarity to the UnmapFile function).

  int fd The file descriptor to close.
  ++++++++++++++++++++++++++++++++++++++*/

int SlimUnmapFile(int fd)
{
	close(fd);

	return (-1);
}


/*++++++++++++++++++++++++++++++++++++++
  Open a new file on disk for writing (with buffering).

  int OpenFileBufferedNew Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to create.
  ++++++++++++++++++++++++++++++++++++++*/

int OpenFileBufferedNew(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for writing [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	CreateFileBuffer(fd, -1);

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Open a new or existing file on disk for appending (with buffering).

  int OpenFileBufferedAppend Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to create or open.
  ++++++++++++++++++++++++++++++++++++++*/

int OpenFileBufferedAppend(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_WRONLY | O_CREAT | O_APPEND,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for appending [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	CreateFileBuffer(fd, -1);

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Open an existing file on disk for reading (with buffering).

  int ReOpenFileBuffered Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

int ReOpenFileBuffered(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	CreateFileBuffer(fd, 1);

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Write data to a file descriptor (via a buffer).

  int WriteFileBuffered Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to write to.

  const void *address The address of the data to be written.

  size_t length The length of data to write.
  ++++++++++++++++++++++++++++++++++++++*/

int WriteFileBuffered(int fd, const void *address, size_t length)
{
	logassert(fd != -1, "File descriptor is in error - report a bug");

	logassert(fd < nfilebuffers
		  && filebuffers[fd],
		  "File descriptor has no buffer - report a bug");

	logassert(!filebuffers[fd]->reading,
		  "File descriptor was not opened for writing - report a bug");

	/* Write the data */

	if ((filebuffers[fd]->pointer + length) > BUFFLEN) {
		if (write
		    (fd, filebuffers[fd]->buffer,
		     filebuffers[fd]->pointer) !=
		    (ssize_t) filebuffers[fd]->pointer)
			return (-1);

		filebuffers[fd]->pointer = 0;
	}

	if (length >= BUFFLEN) {
		if (write(fd, address, length) != (ssize_t) length)
			return (-1);

		return (0);
	}

	memcpy(filebuffers[fd]->buffer + filebuffers[fd]->pointer, address,
	       length);

	filebuffers[fd]->pointer += length;

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Read data from a file descriptor (via a buffer).

  int ReadFileBuffered Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to read from.

  void *address The address the data is to be read into.

  size_t length The length of data to read.
  ++++++++++++++++++++++++++++++++++++++*/

int ReadFileBuffered(int fd, void *address, size_t length)
{
	logassert(fd != -1, "File descriptor is in error - report a bug");

	logassert(fd < nfilebuffers
		  && filebuffers[fd],
		  "File descriptor has no buffer - report a bug");

	logassert(filebuffers[fd]->reading,
		  "File descriptor was not opened for reading - report a bug");

	/* Read the data */

	if ((filebuffers[fd]->pointer + length) > filebuffers[fd]->length)
		if (filebuffers[fd]->pointer < filebuffers[fd]->length) {
			memcpy(address,
			       filebuffers[fd]->buffer +
			       filebuffers[fd]->pointer,
			       filebuffers[fd]->length -
			       filebuffers[fd]->pointer);

			address +=
			    filebuffers[fd]->length -
			    filebuffers[fd]->pointer;
			length -=
			    filebuffers[fd]->length -
			    filebuffers[fd]->pointer;

			filebuffers[fd]->pointer = 0;
			filebuffers[fd]->length = 0;
		}

	if (length >= BUFFLEN) {
		if (read(fd, address, length) != (ssize_t) length)
			return (-1);

		return (0);
	}

	if (filebuffers[fd]->pointer == filebuffers[fd]->length) {
		ssize_t len = read(fd, filebuffers[fd]->buffer, BUFFLEN);

		if (len <= 0)
			return (-1);


		filebuffers[fd]->length = len;
		filebuffers[fd]->pointer = 0;
	}

	if (filebuffers[fd]->length == 0)
		return (-1);

	memcpy(address, filebuffers[fd]->buffer + filebuffers[fd]->pointer,
	       length);

	filebuffers[fd]->pointer += length;

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Seek to a position in a file descriptor that uses a buffer.

  int SeekFileBuffered Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to seek within.

  off_t position The position to seek to.
  ++++++++++++++++++++++++++++++++++++++*/

int SeekFileBuffered(int fd, off_t position)
{
	logassert(fd != -1, "File descriptor is in error - report a bug");

	logassert(fd < nfilebuffers
		  && filebuffers[fd],
		  "File descriptor has no buffer - report a bug");

	/* Seek the data - doesn't need to be highly optimised */

	if (!filebuffers[fd]->reading)
		if (write
		    (fd, filebuffers[fd]->buffer,
		     filebuffers[fd]->pointer) !=
		    (ssize_t) filebuffers[fd]->pointer)
			return (-1);

	filebuffers[fd]->pointer = 0;
	filebuffers[fd]->length = 0;

	if (lseek(fd, position, SEEK_SET) != position)
		return (-1);

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Skip forward by an offset in a file descriptor that uses a buffer.

  int SkipFileBuffered Returns 0 if OK or something else in case of an error.

  int fd The file descriptor to skip within.

  off_t skip The amount to skip forward.
  ++++++++++++++++++++++++++++++++++++++*/

int SkipFileBuffered(int fd, off_t skip)
{
	logassert(fd != -1, "File descriptor is in error - report a bug");

	logassert(fd < nfilebuffers
		  && filebuffers[fd],
		  "File descriptor has no buffer - report a bug");

	logassert(filebuffers[fd]->reading,
		  "File descriptor was not opened for reading - report a bug");

	/* Skip the data - needs to be optimised */

	if ((filebuffers[fd]->pointer + skip) > filebuffers[fd]->length) {
		skip -= filebuffers[fd]->length - filebuffers[fd]->pointer;

		filebuffers[fd]->pointer = 0;
		filebuffers[fd]->length = 0;

		if (lseek(fd, skip, SEEK_CUR) == -1)
			return (-1);
	} else
		filebuffers[fd]->pointer += skip;

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Get the size of a file.

  off_t SizeFile Returns the file size if OK or exits in case of an error.

  const char *filename The name of the file to check.
  ++++++++++++++++++++++++++++++++++++++*/

off_t SizeFile(const char *filename)
{
	struct stat buf;

	if (stat(filename, &buf)) {
		fprintf(stderr, "Cannot stat file '%s' [%s].\n", filename,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	return (buf.st_size);
}


/*++++++++++++++++++++++++++++++++++++++
  Get the size of a file from a file descriptor.

  off_t SizeFileFD Returns the file size if OK or exits in case of an error.

  int fd The file descriptor to check.
  ++++++++++++++++++++++++++++++++++++++*/

off_t SizeFileFD(int fd)
{
	struct stat buf;

	if (fstat(fd, &buf)) {
		fprintf(stderr, "Cannot stat file descriptor '%d' [%s].\n",
			fd, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return (buf.st_size);
}


/*++++++++++++++++++++++++++++++++++++++
  Check if a file exists.

  int ExistsFile Returns 1 if the file exists and 0 if not.

  const char *filename The name of the file to check.
  ++++++++++++++++++++++++++++++++++++++*/

int ExistsFile(const char *filename)
{
	struct stat buf;

	if (stat(filename, &buf))
		return (0);
	else
		return (1);
}


/*++++++++++++++++++++++++++++++++++++++
  Close a file on disk (and flush the buffer).

  int CloseFileBuffered returns -1 (for similarity to the *OpenFileBuffered* functions).

  int fd The file descriptor to close.
  ++++++++++++++++++++++++++++++++++++++*/

int CloseFileBuffered(int fd)
{
	logassert(fd < nfilebuffers
		  && filebuffers[fd],
		  "File descriptor has no buffer - report a bug");

	if (!filebuffers[fd]->reading)
		if (write
		    (fd, filebuffers[fd]->buffer,
		     filebuffers[fd]->pointer) !=
		    (ssize_t) filebuffers[fd]->pointer)
			return (-1);

	close(fd);

	free(filebuffers[fd]);

	filebuffers[fd] = NULL;

	return (-1);
}


/*++++++++++++++++++++++++++++++++++++++
  Open an existing file on disk for reading (in a simple mode).

  int OpenFile Returns the file descriptor if OK or exits in case of an error.

  const char *filename The name of the file to open.
  ++++++++++++++++++++++++++++++++++++++*/

int OpenFile(const char *filename)
{
	int fd;

	/* Open the file */

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for reading [%s].\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return (fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Close a file on disk (that was opened in simple mode).

  int fd The file descriptor to close.
  ++++++++++++++++++++++++++++++++++++++*/

void CloseFile(int fd)
{
	close(fd);
}


/*++++++++++++++++++++++++++++++++++++++
  Delete a file from disk.

  int DeleteFile Returns 0 if OK.

  const char *filename The name of the file to delete.
  ++++++++++++++++++++++++++++++++++++++*/

int DeleteFile(const char *filename)
{
	unlink(filename);

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Rename a file on disk.

  int RenameFile Returns 0 if OK.

  const char *oldfilename The old name of the file before renaming.

  const char *newfilename The new name of the file after renaming.
  ++++++++++++++++++++++++++++++++++++++*/

int RenameFile(const char *oldfilename, const char *newfilename)
{
	rename(oldfilename, newfilename);

	return (0);
}


/*++++++++++++++++++++++++++++++++++++++
  Create a file buffer.

  int fd The file descriptor.

  int read_write A flag set to 1 for reading, -1 for writing and 0 for unbuffered.
  ++++++++++++++++++++++++++++++++++++++*/

static void CreateFileBuffer(int fd, int read_write)
{
	if (nfilebuffers <= fd) {
		int i;

		filebuffers =
		    (struct filebuffer **) realloc((void *) filebuffers,
						   (fd +
						    1) *
						   sizeof(struct filebuffer
							  *));

		for (i = nfilebuffers; i <= fd; i++)
			filebuffers[i] = NULL;

		nfilebuffers = fd + 1;
	}

	if (read_write) {
		filebuffers[fd] =
		    (struct filebuffer *) calloc(sizeof(struct filebuffer),
						 1);

		filebuffers[fd]->reading = (read_write == 1);
	}
}
