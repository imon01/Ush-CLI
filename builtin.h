#ifndef BUILTIN
#define BUILTIN



/*
 * Function: 
 *      Verifies user command is builtin
 * 
 * Arguments:
 *      User command and arguments represented as string tokens in array
 * 
 * Return:
 *      1 if builtin command. < 1 otherwise.
 */
int builtin( char **, int [], int, short * );



/*
 * Function: 
 *      aexit command. Exits ush if argument is valid. Otherwise nothing.
 * 
 * Arguments:
 *      User command and arguments represented as string tokens in array.
 * 
 * Return:
 */
void aexit( char** );



/*
 * Function: 
 *      aecho command that mirrors interpreter echo
 * 
 * Arguments:
 *      User command and arguments represented as string tokens in array.
 * 
 * Return:
 */
void aecho( char**);


/*
 * Function: 
 *      Changes directory
 * 
 * Arguments:
 *      Pathname of user requested directory change
 * 
 * Return:
 */
void cd( char   **);

/*
 * Function: 
 *      Sets environment variable with NAME VALUE syntax
 * 
 * Arguments:
 *      User string containing the paramters to setenv
 * 
 * Return:
 */
void envset(char **);


/*
 * Function: 
 *      Removes variable from environment variable set
 * 
 * Arguments:
 *      Environment variable name 
 * 
 * Return: 
 */
void envunset(char **);




#endif