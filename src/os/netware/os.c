/* ====================================================================
 * Copyright (c) 1995-2000 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "httpd.h"
#include "ap_config.h"
#include <dirent.h>

void ap_os_dso_init(void)
{
}

void *ap_os_dso_load(const char *path)
{
    unsigned int nlmHandle;
    char *moduleName = NULL;
    
    moduleName = strrchr(path, '/');

    if (moduleName) {
        moduleName++;
    }
    
    nlmHandle = FindNLMHandleInAddressSpace((char*)moduleName, NULL);

    if (nlmHandle == NULL) {
        spawnlp(P_NOWAIT | P_SPAWN_IN_CURRENT_DOMAIN, path, NULL);        
        nlmHandle = FindNLMHandleInAddressSpace((char*)moduleName, NULL);
    }

    return (void *)nlmHandle;
}

void ap_os_dso_unload(void *handle)
{
	KillMe(handle);
}

void *ap_os_dso_sym(void *handle, const char *symname)
{
    return ImportSymbol((int)GetNLMHandle(), (char *)symname);
}

const char *ap_os_dso_error(void)
{
    return NULL;
}

char *remove_filename(char* str)
{
    int i, len = strlen(str);    
  
    for (i=len; i; i--) {
        if (str[i] == '\\' || str[i] == '/') {
            str[i] = NULL;
            break;
        }
    }
    return str;
}

char *bslash2slash(char* str)
{
    int i, len = strlen(str);    
  
    for (i=0; i<len; i++) {
        if (str[i] == '\\') {
            str[i] = '/';
            break;
        }
    }
    return str;
}

void init_name_space()
{
    UnAugmentAsterisk(TRUE);
    SetCurrentNameSpace(NW_NS_LONG);
    SetTargetNameSpace(NW_NS_LONG);
}

/*  Perform complete canonicalization.  On NetWare we are just
	lower casing the file name so that the comparisons will match.
	NetWare assumes that all physical paths are fully qualified.  
	Each file path must include a volume name.
 */
char *ap_os_canonical_filename(pool *pPool, const char *szFile)
{
    char *pNewName;
    
    pNewName = ap_pstrdup(pPool, szFile);
    strlwr(pNewName);
    bslash2slash(pNewName);
    return pNewName;
}

/*
 * ap_os_is_filename_valid is given a filename, and returns 0 if the filename
 * is not valid for use on this system. On NetWare, this means it fails any
 * of the tests below. Otherwise returns 1.
 *
 * The tests are:
 *
 * 1) total path length greater than MAX_PATH
 * 
 * 2) the file path must contain a volume specifier and no / or \
 *     can appear before the volume specifier.
 *
 * 3) anything using the octets 0-31 or characters " < > | :
 *    (these are reserved for Windows use in filenames. In addition
 *     each file system has its own additional characters that are
 *     invalid. See KB article Q100108 for more details).
 *
 * 4) anything ending in "." (no matter how many)
 *    (filename doc, doc. and doc... all refer to the same file)
 *
 * 5) any segment in which the basename (before first period) matches
 *    one of the DOS device names
 *    (the list comes from KB article Q100108 although some people
 *     reports that additional names such as "COM5" are also special
 *     devices).
 *
 * If the path fails ANY of these tests, the result must be to deny access.
 */

int ap_os_is_filename_valid(const char *file)
{
    const char *segstart;
    unsigned int seglength;
    const char *pos;
	char *colonpos, *fslashpos, *bslashpos;
    static const char * const invalid_characters = "?\"<>*|:";
    static const char * const invalid_filenames[] = { 
		"CON", "AUX", "COM1", "COM2", "COM3", 
		"COM4", "LPT1", "LPT2", "LPT3", "PRN", "NUL", NULL 
    };

	// First check to make sure that we have a file so that we don't abend
	if (file == NULL)
		return 0;

    /* Test 1 */
    if (strlen(file) >= _MAX_PATH) {
		/* Path too long for Windows. Note that this test is not valid
		 * if the path starts with //?/ or \\?\. */
		return 0;
    }

    pos = file;

    /* Skip any leading non-path components. This can be either a
     * drive letter such as C:, or a UNC path such as \\SERVER\SHARE\.
     * We continue and check the rest of the path based on the rules above.
     * This means we could eliminate valid filenames from servers which
     * are not running NT (such as Samba).
     */

	colonpos = strchr (file, ':');
	fslashpos = strchr (file, '/');
	bslashpos = strchr (file, '\\');

	// The path must contain a volume specifier and the volume
	//  specifier must appear before in slashes.
	if (colonpos) {
		// If a slash appears before the colon then the path
		//  is invalid until we support remotes server file
		//  access
		if (fslashpos && (fslashpos < colonpos))
			return 0;
		if (bslashpos && (bslashpos < colonpos))
			return 0;
	}
	else {
		return 0;
	}

	pos = ++colonpos;
   	if (!*pos) {
		/* No path information */
		return 0;
    }

    while (*pos) {
		unsigned int idx;
		unsigned int baselength;

		while (*pos == '/' || *pos == '\\') {
    	    pos++;
		}
		if (*pos == '\0') {
		    break;
		}
		segstart = pos;	/* start of segment */
		while (*pos && *pos != '/' && *pos != '\\') {
		    pos++;
		}
		seglength = pos - segstart;
		/* 
		 * Now we have a segment of the path, starting at position "segstart"
		 * and length "seglength"
		 */

		/* Test 2 */
		for (idx = 0; idx < seglength; idx++) {
	    	if ((segstart[idx] > 0 && segstart[idx] < 32) ||
				strchr(invalid_characters, segstart[idx])) {
				return 0;
	    	}
		}

		/* Test 3 */
		if (segstart[seglength-1] == '.') {
		    return 0;
		}

		/* Test 4 */
		for (baselength = 0; baselength < seglength; baselength++) {
		    if (segstart[baselength] == '.') {
				break;
		    }
		}

		/* baselength is the number of characters in the base path of
		 * the segment (which could be the same as the whole segment length,
		 * if it does not include any dot characters). */
		if (baselength == 3 || baselength == 4) {
	    	for (idx = 0; invalid_filenames[idx]; idx++) {
				if (strlen(invalid_filenames[idx]) == baselength &&
				    !strnicmp(invalid_filenames[idx], segstart, baselength)) {
				    return 0;
				}
	    	}
		}
    }

    return 1;
}
