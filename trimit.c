// file: trimit.c
// source: http://www.xappsoftware.com/wordpress/2013/11/14/a-simple-c-function-to-trim-leading-and-trailing-white-spaces-from-a-string/
// author: "GG1"
// downloaded by: bksteele, bks@cs.rit.edu
// modified by: T Fossum tvf@cs.rit.edu Sat Feb 15 10:41:40 EST 2020
// since: 3/1/2017
// // // // // // // // // // // // // // // // // // // // // // // //

#include <stdio.h>
#include <ctype.h>   // isspace
#include <string.h>  // strlen
#include <assert.h>

#include "trimit.h"

/*+-----------------------------------------------------------+
  | Function name : trim.                                     |
  | Parameters    :                                           |
  |   str         : the string to trim.                       |
  | Return  value : the pointer to the original string
  | Description   : Trims leading and trailing white spaces   |
  |                 and return the modified string.           |
  |                 The pointer to the string can be used to  |
  |                 free memory.                              |
  +-----------------------------------------------------------+*/
/// trim leading and trailing whitespace {' ' \t \n \r} *** IN PLACE ***
char * trim(char *str) {
    if( str == NULL || *str == '\0' ) // NULL or empty
        return str;

    size_t   len      = strlen( str );
    char     *frontp  = str;   // point to the front of the string
    char     *endp    = str + len; // points to the string's null terminator

    /* Move the front and back pointers to address the first
       non-whitespace characters from each end.
     */
    while(*frontp && isspace(*frontp))
        ++frontp;
    // at this point, frontp is either endp (end of string)
    // or points to the first non-whitespace character
    // at the front of the string

  if ( frontp == endp ) {
        // no non-whitespace characters found
        *str = '\0'; // null terminate
        return str;
    }

    // if we get here, the string has at least one non-whitespace character
    // and frontp points to the first one,
    // so we next eliminate the rightmost whitespace

    while( isspace(*(endp-1)))
        --endp;
    // at this point, endp-1 points
    // to the rightmost non-whitespace character in the string,
    // and clearly frontp < endp

    assert (frontp < endp);

    *endp = '\0'; // null terminate

    // now move the characters from frontp to endp down
    // to the beginning of the string
    char * cp = str;
    while ( frontp <= endp ) {
        *cp++ = *frontp++; // also copies the nul byte
    }
    return str;
}
