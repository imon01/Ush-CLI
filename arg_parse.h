#ifndef ARG_PARSE
#define ARG_PARSE

#include "builtin.h"


/*
 * Function: 
 *      arg_parse
 * 
 * Arguments:
 *      Pointer to user inputs
 * 
 * Return:
 *      An array of pointers to start of token
 *
 */
char** arg_parse(char*, int *);


/* Expand
* orig    The input string that may contain variables to be expanded
* new     An output buffer that will contain a copy of orig with all 
*         variables expanded
* newsize The size of the buffer pointed to by new.
* returns 1 upon success or 0 upon failure. 
*
* Example: "Hello, ${PLACE}" will expand to "Hello, World" when the environment
* variable PLACE="World". 
*/

int expand( char*, char*, int);

#endif