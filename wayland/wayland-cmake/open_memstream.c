/*
 * Implementation of the POSIX open_memstream() function, which Linux has
 * but BSD lacks.
 *
 * Summary:
 * - Works like a file-backed FILE* opened with fopen(name, "w"), but the
 *   backing is a chunk of memory rather than a file.
 * - The buffer expands as you write more data.  Seeking past the end
 *   of the file and then writing to it zero-fills the gap.
 * - The values at "*bufp" and "*sizep" should be considered read-only,
 *   and are only valid immediately after an fflush() or fclose().
 * - A '\0' is maintained just past the end of the file. This is not included
 *   in "*sizep".  (The behavior w.r.t. fseek() is not clearly defined.
 *   The spec says the null byte is written when a write() advances EOF,
 *   but it looks like glibc ensures the null byte is always found at EOF,
 *   even if you just seeked backwards.  The example on the opengroup.org
 *   page suggests that this is the expected behavior.  The null must be
 *   present after a no-op fflush(), which we can't see, so we have to save
 *   and restore it.  Annoying, but allows file truncation.)
 * - After fclose(), the caller must eventually free(*bufp).
 *
 * This is built out of funopen(), which BSD has but Linux lacks.  There is
 * no flush() operator, so we need to keep the user pointers up to date
 * after each operation.
 *
 * I don't think Windows has any of the above, but we don't need to use
 * them there, so we just supply a stub.
 */

#define _BSD_SOURCE

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__ANDROID__) && __ANDROID_API__ < 24

__attribute__ ((visibility ("default")))
FILE* open_memstream(char**, size_t*);

#if 0
# define DBUG(x) printf x
#else
# define DBUG(x) ((void)0)
#endif

/*
 * Definition of a seekable, write-only memory stream.
 */
typedef struct {
    char**      bufp;       /* pointer to buffer pointer */
    size_t*     sizep;      /* pointer to eof */

    size_t      allocSize;  /* size of buffer */
    size_t      eof;        /* furthest point we've written to */
    size_t      offset;     /* current write offset */
    char        saved;      /* required by NUL handling */
} MemStream;

/*
 * Ensure that we have enough storage to write "size" bytes at the
 * current offset.  We also have to take into account the extra '\0'
 * that we maintain just past EOF.
 *
 * Returns 0 on success.
 */
static int ensureCapacity(MemStream* stream, int writeSize)
{
    DBUG(("+++ ensureCap off=%d size=%d\n", stream->offset, writeSize));

    size_t neededSize = stream->offset + writeSize + 1;
    if (neededSize <= stream->allocSize)
        return 0;

    size_t newSize;

    if (stream->allocSize == 0) {
        newSize = BUFSIZ;
    } else {
        newSize = stream->allocSize;
        newSize += newSize / 2;             /* expand by 3/2 */
    }

    if (newSize < neededSize)
        newSize = neededSize;
    DBUG(("+++ realloc %p->%p to size=%d\n",
        stream->bufp, *stream->bufp, newSize));
    char* newBuf = (char*) realloc(*stream->bufp, newSize);
    if (newBuf == NULL)
        return -1;

    *stream->bufp = newBuf;
    stream->allocSize = newSize;
    return 0;
}

/*
 * Write data to a memstream, expanding the buffer if necessary.
 *
 * If we previously seeked beyond EOF, zero-fill the gap.
 *
 * Returns the number of bytes written.
 */
static int write_memstream(void* cookie, const char* buf, int size)
{
    MemStream* stream = (MemStream*) cookie;

    if (ensureCapacity(stream, size) < 0)
        return -1;

    /* seeked past EOF earlier? */
    if (stream->eof < stream->offset) {
        DBUG(("+++ zero-fill gap from %d to %d\n",
            stream->eof, stream->offset-1));
        memset(*stream->bufp + stream->eof, '\0',
            stream->offset - stream->eof);
    }

    /* copy data, advance write pointer */
    memcpy(*stream->bufp + stream->offset, buf, size);
    stream->offset += size;

    if (stream->offset > stream->eof) {
        /* EOF has advanced, update it and append null byte */
        DBUG(("+++ EOF advanced to %d, appending nul\n", stream->offset));
        assert(stream->offset < stream->allocSize);
        stream->eof = stream->offset;
    } else {
        /* within previously-written area; save char we're about to stomp */
        DBUG(("+++ within written area, saving '%c' at %d\n",
            *(*stream->bufp + stream->offset), stream->offset));
        stream->saved = *(*stream->bufp + stream->offset);
    }
    *(*stream->bufp + stream->offset) = '\0';
    *stream->sizep = stream->offset;

    return size;
}

/*
 * Seek within a memstream.
 *
 * Returns the new offset, or -1 on failure.
 */
static fpos_t seek_memstream(void* cookie, fpos_t offset, int whence)
{
    MemStream* stream = (MemStream*) cookie;
    off_t newPosn = (off_t) offset;

    if (whence == SEEK_CUR) {
        newPosn += stream->offset;
    } else if (whence == SEEK_END) {
        newPosn += stream->eof;
    }

    if (newPosn < 0 || ((fpos_t)((size_t) newPosn)) != newPosn) {
        /* bad offset - negative or huge */
        DBUG(("+++ bogus seek offset %ld\n", (long) newPosn));
        errno = EINVAL;
        return (fpos_t) -1;
    }

    if (stream->offset < stream->eof) {
        /*
         * We were pointing to an area we'd already written to, which means
         * we stomped on a character and must now restore it.
         */
        DBUG(("+++ restoring char '%c' at %d\n",
            stream->saved, stream->offset));
        *(*stream->bufp + stream->offset) = stream->saved;
    }

    stream->offset = (size_t) newPosn;

    if (stream->offset < stream->eof) {
        /*
         * We're seeked backward into the stream.  Preserve the character
         * at EOF and stomp it with a NUL.
         */
        stream->saved = *(*stream->bufp + stream->offset);
        *(*stream->bufp + stream->offset) = '\0';
        *stream->sizep = stream->offset;
    } else {
        /*
         * We're positioned at, or possibly beyond, the EOF.  We want to
         * publish the current EOF, not the current position.
         */
        *stream->sizep = stream->eof;
    }

    return newPosn;
}

/*
 * Close the memstream.  We free everything but the data buffer.
 */
static int close_memstream(void* cookie)
{
    free(cookie);
    return 0;
}

/*
 * Prepare a memstream.
 */
FILE* open_memstream(char** bufp, size_t* sizep)
{
    FILE* fp;
    MemStream* stream;

    if (bufp == NULL || sizep == NULL) {
        errno = EINVAL;
        return NULL;
    }

    stream = (MemStream*) calloc(1, sizeof(MemStream));
    if (stream == NULL)
        return NULL;

    fp = funopen(stream, NULL, write_memstream,
        seek_memstream, close_memstream);
    if (fp == NULL) {
        free(stream);
        return NULL;
    }

    *sizep = 0;
    *bufp = NULL;
    stream->bufp = bufp;
    stream->sizep = sizep;

    return fp;
}

#endif
