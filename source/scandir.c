/*
 * $Id: scandir.c,v 1.4 2002-03-09 17:49:13 f Exp $
 */

#include "irc.h"
#include "ircaux.h"

#ifndef HAVE_SCANDIR

#if (!defined(ultrix) && !defined(__386BSD__) && !defined(_HPUX_SOURCE)) || defined(HPUX7)

/*
 * Copyright (c) 1983 Regents of the University of California. All rights
 * reserved. 
 *
 * Redistribution and use in source and binary forms are permitted provided that
 * the above copyright notice and this paragraph are duplicated in all such
 * forms and that any documentation, advertising materials, and other
 * materials related to such distribution and use acknowledge that the
 * software was developed by the University of California, Berkeley.  The
 * name of the University may not be used to endorse or promote products
 * derived from this software without specific prior written permission. THIS
 * SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. 
 */

# if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scandir.c	5.3 (Berkeley) 6/18/88";
# endif /* LIBC_SCCS and not lint */

# ifndef NULL
#  define NULL 0
# endif

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp. Returns
 * the number of entries and a pointer to a list of pointers to struct direct
 * (through namelist). Returns -1 if there were any errors. 
 */

/*
 * Mike Sandrof added this for HPUX compatibility in
 * IRCII on the advice of some HPUX users - 5/11/90 
 * this is no longer valid, as hpux has its own scandir(), june 1993, phone.
 * at least, for some versions of hpux.. oct 1994, mrg.
 *
 * Brett Sivies added POSIX here.
 */

# if defined(XD88) || defined(__SVR4) || defined(POSIX) || defined(__linux__) \
  || defined(SVR3) || defined(__osf__) || defined(M_UNIX) || defined(_SEQUENT_) \
  || defined(__QNX__)

/*
**  SCANDIR
**  Scan a directory, collecting all (selected) items into a an array.
*/
#include "scandir.h"

int
scandir(Name, dirlist, Selector, Sorter)
#if defined(__linux__) || defined(__sgi)
    const char            *Name;
#else
    char		  *Name;
#endif
    struct dirent		***dirlist;
    int			 (*Selector)();
    int			 (*Sorter)();
{
    struct dirent	 **names;
    static  struct dirent	  *E;
    register DIR	  *Dp;
    register int	   i;
    register int	   size = INITIAL_SIZE;

    if (!(names = (struct dirent **) new_malloc(size * sizeof names[0])) || access(Name, R_OK | X_OK) || !(Dp = opendir(Name)))
	return(-1);

    /* Read entries in the directory. */

    for (i = 0; (E = readdir(Dp)); )
	if (Selector == NULL || (*Selector)(E))
	{
	    /* User wants them all, or he wants this one. */
	    if (++i >= size)
	    {
		size <<= 1;
		names = (struct dirent **) new_realloc((char *)names, size * sizeof names[0]);
		if (names == NULL)
		{
		    closedir(Dp);
		    new_free(&names);
		    return(-1);
		}
	    }

	    /* Copy the entry. */
	    if ((names[i - 1] = (struct dirent *) new_malloc(DIRSIZ(E))) == NULL)
	    { 
		closedir(Dp);
		new_free(&names);
		return(-1);
	    }
#ifndef __QNX__
	    names[i - 1]->d_ino = E->d_ino;
	    names[i - 1]->d_reclen = E->d_reclen;
#endif
	    (void) strcpy(names[i - 1]->d_name, E->d_name);
	}

    /* Close things off. */
    names = (struct dirent **) new_realloc((char *) names, (i + 1) * sizeof names[0]);
    names[i] = 0;
    *dirlist = names;
    closedir(Dp);

    /* Sort? */
    if (i && Sorter)
	qsort((char *)names, i, sizeof names[0], Sorter);

    return(i);
}
#else
/* End of Mike Sandrof's change */
#include "scandir.h"

int scandir(dirname, namelist, select, dcomp)
#ifdef NeXT
const char *dirname;
#else
char *dirname;
#endif /* NeXT */
struct direct *(*namelist[]);
int (*select) (), (*dcomp) ();
{
    register struct direct *d,
          *p,
         **names;
    register int nitems;
    register char *cp1,
        *cp2;
    struct stat stb;
    long arraysz;
    DIR *dirp;

    if (access(dirname, R_OK|X_OK))
	return (-1);
    if ((dirp = opendir(dirname)) == NULL)
	return (-1);
    if (fstat(dirp->dd_fd, &stb) < 0)
	return (-1);

    /*
     * estimate the array size by taking the size of the directory file and
     * dividing it by a multiple of the minimum size entry. 
     */
    arraysz = (stb.st_size / 24);
    names = (struct direct **) new_malloc(arraysz * sizeof(struct direct *));
    if (names == NULL)
	return (-1);

    nitems = 0;
    while ((d = readdir(dirp)) != NULL)
    {
	if (select != NULL && !(*select) (d))
	    continue;		/* just selected names */
	/*
	 * Make a minimum size copy of the data 
	 */
	p = (struct direct *) new_malloc(DIRSIZ(d));
	if (p == NULL)
	    return (-1);
	p->d_ino = d->d_ino;
	p->d_reclen = d->d_reclen;
# if ! defined(ISC22) && ! defined(ESIX)
	p->d_namlen = d->d_namlen;
# endif /* ! defined(ISC22) && ! defined(ESIX) */
	for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++;);
	/*
	 * Check to make sure the array has space left and realloc the
	 * maximum size. 
	 */
	if (++nitems >= arraysz)
	{
	    if (fstat(dirp->dd_fd, &stb) < 0)
		return (-1);	/* just might have grown */
	    arraysz = stb.st_size / 12;
	    names = (struct direct **) new_realloc((char *) names,
					 arraysz * sizeof(struct direct *));
	    if (names == NULL)
		return (-1);
	}
	names[nitems - 1] = p;
    }
    closedir(dirp);
    if (nitems && dcomp != NULL)
	qsort(names, nitems, sizeof(struct direct *), dcomp);
    *namelist = names;
    return (nitems);
}


/*
 * Alphabetic order comparison routine for those who want it. 
 */
int alphasort(d1, d2)
struct direct **d1,
     **d2;
{
    return (strcmp((*d1)->d_name, (*d2)->d_name));
}
#endif

#endif	/* ultrix || __386BSD__ || BSD */
#endif /* !HAVE_SCANDIR */
