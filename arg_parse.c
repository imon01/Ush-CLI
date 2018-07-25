#include "arg_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


/*
 *  Constants
 */
#define TRUE    1
#define FALSE   0
#define QUOTE   34
#define LINELEN 1024

/* Stack alphabet */
#define Z    90
#define LB   123
#define RB   125
#define EOL  0
#define CASH 36

/* Stack states */
#define STATE_0 0
#define STATE_1 1
#define STATE_2 2
#define STATE_3 3
#define STATE_F 1


/*
 *  Prototypes
 */
int  copypid( char*, int*  );
int  copyenv( char*, int*  );
void pop    ( char , char*, int*);
void push   ( char , char*, int*);
int  peek   ( char , char*, int*);
int  expand ( char*, char*, int );
int  append ( char*, char*, int*, int*);


/*******************************************************************************/
/*                       BEGIN ARG_PARSE FUNCTIONALITIES                       */
/*******************************************************************************/
 
char** arg_parse(char* line, int * argcp){                   
    
    /* 
     *  Indicates string satisfies even number of quotes
     *  
     *  0 NO, 1 Yes
     */
    short   evenflag;  
    
    /* 
     *  Flag indicates token currently being parsed
     * 
     *  0 NO, 1 Yes
     */
    short   tokenflag;    
    
    int     qcount;
    int     argcounter;
    
    char*   linesave;
    char*   linecopy;
    char**  argptrs;    
    char**  save_argptrs;    


    qcount        = 0;
    argcounter    = 0;
    evenflag       = FALSE;    
    tokenflag     = FALSE;
    linecopy      = line;    
    
            
    while( *line){
        qcount = ( *line == QUOTE )? (qcount + 1): qcount;
        line++;
    }    
    
    qcount = (qcount %2);
    
    /* Even number of quotes if first condition satisfied */
    if( ~qcount){
        argcounter++;
        evenflag = TRUE;
    }else{
        fprintf (stderr, "%% Error: odd number of quotes \n");
        argcounter = 1;
    }
    
    line = linecopy;
     
    /* Begin counting arguments */    
    while( *line && evenflag){
                        
        /*  Start of token */
        if( !isspace( *line) &&  !tokenflag ){
            argcounter++;
            tokenflag = TRUE;
        }
              
        /* End of token */
        if( isspace( *line) && tokenflag){
            tokenflag = FALSE;
        }
        
        line++;    
    }
    
    
    argptrs = malloc(argcounter * sizeof(char*));
    
    
    if(argcp != NULL){
            *argcp = argcounter;
    }
    
    line          = linecopy;
    tokenflag     = FALSE;    
    save_argptrs  = argptrs;
        
    /* Begin building argument array pointers  */
    while( *line && evenflag){
        
        
        /* Start of token. Encountered first non-space character */
        if( !isspace( *line) &&  !tokenflag){            
            tokenflag = TRUE;
            *argptrs  = line;
            argptrs++;     
        }
                
        /* Hit space char at end of token, flip tokenflag off*/
        if( isspace( *line) && tokenflag){
            tokenflag = FALSE;
            *line     = 0;
        }
       
        line++;    
    }
    
    *argptrs = 0;
    
    argptrs = save_argptrs;
    
    /* Quote removal */
    while( *argptrs && evenflag ){
        
        line     = *argptrs;
        linecopy = line;
        
        while( *linecopy){
            *line = *linecopy;
            linecopy++;
            if( *line != QUOTE){
                line++;
            }
        }
        *line = 0;
        ++argptrs;
    }
    
    argptrs = save_argptrs;    
    
    
    return argptrs;
}
/* End arg_parse*/





/*******************************************************************************/
/*                          BEGIN EXPAND FUNCTIONALITIES                       */
/*******************************************************************************/



/*
 * Pushdown automata implementation of expand
 */
int expand(char* orig, char* new, int newsize){
     
    char buf [256];
    
    
    /* Limit stack size*/
    char stack[1024];
    int temp;    
    int buflen;
    int stacksize;
    int currentsize;

    short n;
    short c;
    short state;    
    short expflag;
    short bufindex;
    short endinput;
    
    n           = 0;
    c           = 0;
    temp        = 0;    
    buflen      = sizeof(buf);    
    expflag     = 0;
    bufindex    = 0;
    endinput    = 0;
    stacksize   = 0;    
    currentsize = 0;        
    bzero(buf, buflen);
    bzero(stack, sizeof(stack));
        
    
    push(Z, stack, &stacksize);
    state   = STATE_0;
    
    
    
    while( !endinput && (currentsize < newsize) )
    {
        c = *orig;                
        
        switch(c){            
            case CASH:
                
                /* (q1, $, q0, eZ) */
                if( peek(CASH, stack, &stacksize))
                {
                    pop(CASH, stack, &stacksize);
                    copypid(buf, &buflen);                    
                    temp             = sizeof(buf);
                    state            = STATE_0;
                    
                    if( (currentsize + temp) < newsize)
                    {
                        n            = append(new, buf, &buflen, &temp);
                        new         += n;
                        currentsize += n;
                        n            = 0;
                    }
                    bzero(buf, buflen);
                }
                /* (q0, $, q1, $Z) */
                else
                {
                    buf[0]       = '$';                                            
                    state        = STATE_1;
                    push(CASH, stack, &stacksize);
                }
                ++orig;
                break;
                
            /* { */
            case LB:
                
                /* (q1, {, q2, BZ) */
                if(state == STATE_1)
                {
                    expflag = 1;
                    bzero(buf, sizeof(buf));
                    bufindex = 0;                    
                    state   = STATE_2;
                }
                
                /* (q2, {, q2, BB) */
                push( LB, stack, &stacksize);                
                ++orig;
                break;
            
            case RB:
                
                /* (q2, }, q3, eB) */
                if(state == STATE_2)
                {
                    state   = STATE_3;
                }
                /* (q3, }, q3, eB) */
                if( peek( LB, stack, &stacksize) )
                {
                    pop( LB, stack, &stacksize);
                }
                
                if( peek(CASH, stack, &stacksize) )
                {
                    state = STATE_0;
                    pop(CASH, stack, &stacksize);
                    buflen  = sizeof(buf);                
                    copyenv(buf, &buflen);                    
                    temp         = sizeof(buf);
                    n            = append(new, buf, &buflen, &temp);
                    new         += n;
                    currentsize += n;
                    n            = 0;                    
                    expflag      = 0;                    
                    bzero(buf, buflen);
                }
                
                ++orig;
                break;
                
            case EOL:
                endinput = 1;
                /* First condition will fall through if valid */
            default:
                if(state == STATE_1)
                {
                    state        = STATE_0;
                    pop(CASH, stack, &stacksize);
                    
                    if( (currentsize + sizeof(buf) ) < newsize)
                    {
                        n            = append(new, buf, &buflen, &newsize);                    
                        new         += n;
                        currentsize += n;
                    }
                    bzero(buf, buflen);                    
                }
                
                if( expflag && ( (currentsize + 1) < newsize) && bufindex < buflen ){
                    buf[bufindex] = *orig;
                    ++orig;
                    ++bufindex;
                }
                
                /* Stack unchanged*/                                    
                if( !expflag &&  ( (currentsize + 1) < newsize) )
                {
                    *new     = *orig;                        
                    ++new;
                    ++orig;
                    ++currentsize;
                }
        }/* End switch */
    }
    
    /* End loop*/
    
    
    state = 0;
    if( peek(Z, stack, &stacksize) )
    {        
        pop(Z, stack, &stacksize);
        state = 1;
    }
    


    return state;
}/* End expand */

/*
 *  Function:
 *      Copies process id into buffer
 * 
 *  Arguments:
 *      
 *  Return:
 *      Number of characters read that invoked this function. Length of substring "$$" or 2.
 *      Used to move ORIG pointer in expand(...)
 */
int copypid(char * buf, int *len){

    int pidn    = 0;
    pidn        = getpid();
             
    snprintf( buf, *len, "%d", pidn);
    
    *len = sizeof(buf);
    
    /*
    *
    */ 
    return 2;
}/* End Copypid */


/*
 *  Function:
 *      Copies environment variable into buf 
 * 
 *  Arguments:
 *      
 *  Return:
 *      Number of characters written to buf
 */
int copyenv(char* buf, int* buflen)
{   
    int   i;
    int   len;
    char* str;
    
    i     = 0;
    len   = 0;
    str   = 0;
    if( getenv(buf) )
    {        
        str  = getenv(buf);
        len  = strlen(str);
    }
    
    *buf = 0;
    
    if(  len  < *buflen   )
    {
        for(; i < len ; i++)
        {
            *buf++ = *str++;
        }
        *buf = 0;
    }         
    return i;
}/* End Copying */


void push(char sym, char* stack, int* stacksize )
{    
    *(stack + *stacksize) = sym;
    ++(*stacksize);
    
}

void pop(char sym, char * stack, int* stacksize )
{
    --(*stacksize);
    *(stack + *stacksize) = 0;
    
}

int peek(char sym, char * stack, int* stacksize)
{
    return ( sym ==  *(stack + (*stacksize -1) ) );
}


int append(char *buf, char *token, int *buflen, int *tokenlen)
{
    int i;
    
    for( i = 0; i < *tokenlen && *token; i++)
    {
        *buf++ = *token++;
    }
     
    return i;
}



