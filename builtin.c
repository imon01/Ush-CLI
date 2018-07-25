#include "builtin.h"


#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


/*
 *  Local macros
 */
#define SPACE    " "
#define NEWLINE  "-n"
#define NEWLCHAR "\n"



#define HASH  35
#define TRUE  1
#define FALSE 0

int OUT = 1;
int STDERR = 2;


/*
 *  Prototype
 */
int string_isdigit( char*);
void setoutput(int );
void resetoutput();

/* 
 *  Function mapping structure
 */
typedef struct fmap
{
    char *name;
    void (*func) (char **);
}fmap;





/*
 *  Function dispatch table
 */
const fmap f_table [] = 
{
    {"exit", aexit},
    {"aecho", aecho},
    {"cd",  cd},
    {"envset", envset},
    {"envunset",  envunset},
    {NULL, NULL}
};

int tablesize  = 5;



int builtin( char ** args, int pipes[], int cflag, short * dflag )
{
    
    int i;
    int tempout;
    int value;
    int foundflag;
    
    
    i         = 0;
    value     = -1;
    tempout   = OUT;
    foundflag = 0;
    
    

    
    for(; i < tablesize && f_table[i].name && !foundflag; i++)
    {
        if( strcmp(*args, f_table[i].name) == 0){
            if( cflag)
            {
                tempout = OUT;
                OUT = pipes[1];
                *dflag = 1;
            }
            f_table[i].func(args);
            value = 1;
            foundflag = 1;
        }
    }
        
    OUT = tempout;
    
    return value;
}


/*
 * Notes:
 *      Ensures string is numeric. Does not exit otherwise.
 */
void aexit(char ** argsp)
{
    
    int status;
    char *strcopy;
        
    
    status = 0;    
    if( *(argsp+1) )
    {
        argsp++;
        strcopy = *argsp;
        status = string_isdigit( *argsp ) ? atoi( strcopy) : 0;
    }
        
    exit(status);        
}
/* End aexit*/


void aecho(char** argsp)
{        
    int i;
    char* line;
    short hashflag;
    
    /*  
     * -n option indictor. Assuming its not provided unless found     
     */    
    short  newlineflag;
                
    ++argsp;
    
    i             = 0;
    hashflag      = 0;
    newlineflag   = 1;
    
    
    if(*argsp &&  strcmp( *argsp, NEWLINE) == 0){
        newlineflag = 0;                
        ++argsp;
    }
    

    while(*argsp && !hashflag )
    {
        line = *argsp;
        
        while( *line && *line != '#')
        {
            hashflag = ( *line == '#') ? 1: 0;
            if( !hashflag)        
                dprintf(OUT, "%c", *line);                
            
            line++;
        }
        
        if( !hashflag)
        {
            dprintf(OUT, "%s", SPACE);        
            argsp++;
        }
    }
    
    /*/
    
    /*  Printing last argument if not null and
     *  handling '-n' option 
     */
    if(*argsp && ~hashflag )
        dprintf(OUT, "%s", *argsp);        
    
    
    if( newlineflag )
        dprintf(OUT, "\n");
    
}
/* End aecho */



void cd(char ** argv)
{
    
    
    int err;
    int stder;
    char* d;
    
    err = 0;
        
    if( *(argv+1))
        err = chdir( *(argv+1));
    
    if(err)    
        dprintf(STDERR, "%s", "invalid directory path\n");
            
}


void envset(char ** argv)
{    
    int len;    
    int setval;
    char * getenvstr;
    
    
    setval      = 0;
    getenvstr   = NULL;
    
    if( *(argv +1) && *(argv+2))
    {        
        len = strlen( *(argv+1) );
        setval = setenv( *(argv+1), *(argv+2), len);                                               
    }
}/* End envset*/


void envunset(char ** argv)
{
    
    int unsetval;
    
    unsetval = 0;
    
    if( *(argv +1))
    {
        ++argv;
        unsetval = unsetenv(*argv);
    }
    
}/* End envunset*/




int string_isdigit(char *line)
{
    
    int value;    
    value = 1;
    
    while( *line && value ){
        value =  isdigit(*line);
        ++line;
    }
    
    return value;
}/* */

