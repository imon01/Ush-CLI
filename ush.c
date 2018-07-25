/*
 * Author: Isaias Mondar
 * 
 * Notes:
 *      Program mirrors a shell; therefore the string have the following representation "\t\t" is not 
 *      processed for this implementation.
 */


#include "builtin.h"
#include "arg_parse.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

/*
 * 
 */
#define STDIN  0
#define STDOUT 1
#define STDERR 2

/*
 *  Constants
 */ 
#define LINELEN  1024
#define LINELEN2 2048
#define SS_Z  90
#define SS_P  80


/*  
 *  Prototypes
 */

int  count_cmdsub(char*);
int  build_stack(char*, char *, char *);
void extractstr(char*, char*, int*, int);
char * insertstr(char* , char*, char*, int*);
void processline (char *line, char *newbuf, char*, int [], int );

short badfd = 0;
short eio   = 0;

void signalhandler(int sig)
{        
    if(errno)
    {
        switch( errno)
        {
            case  EBADF:
                fprintf(stderr, "\n%%bad file descriptor");
                getchar();                
                break;
            case  EIO:
                fprintf(stderr, "\n%%bad I/O \n");
                getchar();                
                break;
            default:
                getchar();                
                perror("\nerror");
                
        }
    }
    else
        fprintf(stderr, "\n%% ");
}



/*
 *  Shell main 
 */
int main (void)
{    
    int    len;
    char   newbuf   [LINELEN2];
    char   buffer   [LINELEN2];
    char   output   [LINELEN];
          
    signal(SIGINT, signalhandler);
   
    while (1) {
                    
        /* prompt and get line */
        fprintf (stderr, "%% ");
        if (fgets (buffer, LINELEN, stdin) != buffer)
            break;

        /* Get rid of \n at end of buffer. */
        len = strlen(buffer);
        if (buffer[len-1] == '\n'){
            buffer[len-1] = 0;
        }
        /* Run it ... */
        bzero(output, LINELEN);        
        bzero(newbuf, LINELEN2);
        if( !build_stack(buffer, newbuf, output) )        
            fprintf(stderr, "invalid string\n");        
    }

    if (!feof(stdin)){
        perror ("read");
    }
    
    return 0;   /* Also known as exit (0); */
}
/* End main*/


/*
 * Function: 
 *      processline
 * 
 * Arguments:
 *      line
 * 
 * Descrption:
 *      Processes user commands
 * 
 * Notes:
 *      cmdflag indicates substituted command 
 */
void processline (char *cmd, char * newbuf, char* output, int pipes[], int cmdflag )
{    
    pid_t   cpid;
    int     status;
    int     expval;
    int     argc;
    short   dflag;
    int     nbytes;    
    char ** argsp; 
    
    
    dflag       = 0;
    argc        = 0;
    status      = 0;            
    
    expval      = expand( cmd, newbuf, LINELEN);
    argsp       = arg_parse(newbuf, &argc);    

        
    
    if( *argsp && expval> 0){
        
        
        /*  
         *  Start a new process to do the job. 
         */
        if( builtin(argsp, pipes, cmdflag, &dflag ) <0){
             
            cpid = fork();                            
            
            
            if (cpid < 0){
                perror ("fork");
                free(argsp);
                return;
            }
            
            /*  
            *  Check for who we are! 
            */
            if (cpid == 0) {
                
                signal(SIGINT, signalhandler);                      
                                
               if(cmdflag)
               {
                    close(1);
                    dup2( pipes[1], 1);
                    close(pipes[1]);
                    close(pipes[0]);
               }
                
                /*  
                *  We are the child! 
                */
                execvp( *argsp, argsp);
                perror ("exec");
                free(argsp);
                exit (127);
            }
                        
            if( cmdflag)
            {
                close(pipes[1]);
                bzero(output, sizeof(output));
                
                
                if(  (nbytes = read(pipes[0], output,  LINELEN) )  <  0){                    
                    perror("read");
                }
                output[nbytes] = 0;
                nbytes         = 0;
                close(pipes[0]);
            }
            
            /*  
            *  Have the parent wait for child to complete 
            */
            if (wait (&status) < 0)
                perror ("wait");
        }
        
        /* If output redirected elsewhere and not builtin. dflag set by aecho */
        if(dflag){
            
            close(pipes[1]);
            bzero(output, sizeof(output));
            
            
            if(  (nbytes = read(pipes[0], output,  LINELEN) )  <  0){
                perror("read");            
            }
            output[nbytes] = 0;
            nbytes = 0;
            close(pipes[0]);            
        }
    }
    
    
    if( expval == 0)
        dprintf(STDERR, "$%s\n", "invalid string");
    
    free(argsp);
}/* End processline*/


/*
 * Function:
 *      Extracts and processes all substituted commands. Extracted string are
 *      fed to processline().
 * 
 * Arguments:
 *      User input and buffer where to copy input post expand
 * 
 * Returns:
 *      Validity of user input for parenthesis matching
 * 
 * Notes:
 * 
 *      -Doesn't 
 * 
 *      stack
 *      frame = [   SYMBOL, 
 *                  START_INDEX( substring "$("  ),
 *                  END_INDEX( matched ")" ) 
 *              ]
 * 
 *      
 */

int  build_stack(char* orig, char * new, char * output)
{
        
    int i;
    int c;
    int maxsize;
    int stacksize;
    int origindex;    /* Index trails by one */
    int stackindex;    
    int cmdflag;
    char *next;
    char *origcopy;
    char line[1024];    
    
    
    int frame[3];
    int pipes[2];    
    
    short syntaxflag;
    
    i            = 0;
    c            = 0;
    cmdflag      = strlen(orig) - 1;
    stacksize    = 0;    
    stackindex   = 0;
    syntaxflag   = 1;
    origindex    = 0;
    next         = orig + 1;
    origcopy     = orig;
    maxsize      = count_cmdsub(orig);
            
    
    int stack[maxsize*3 + 1];
    bzero(stack, sizeof(stack));
    bzero(frame, sizeof(frame));
    
    /* Set up first stack frame */
    stack[stackindex++] = SS_Z;
    stack[stackindex++] = 1; /* dummy set for */
    stack[stackindex++] = maxsize;
        
    stacksize++;
    
    
    while(*next && syntaxflag)
    {
        c = *next;        
        switch(c)
        {
            case '(':
                bzero(frame, sizeof(frame));
                if( *orig == '$' && *next == '(')
                {
                    stack[stackindex++]   = SS_P;
                    stack[stackindex++]   = origindex;
                    stack[stackindex++]   = 1;
                    stacksize++;
                }
                
                break;
            case ')':
                
                if( stacksize == 1 )
                {
                    fprintf(stderr, "bad syntax \n");
                    *(next+1)             = 0;
                    stack[0]              = SS_P;
                    syntaxflag            = 0;
                    
                }
                else
                {
                    --stacksize;
                    for(i = 2; i> -1; i--)
                    {
                        frame[i]          = stack[--stackindex];
                        stack[stackindex] = 0;
                    }
                    
                    frame[2]              = origindex + 1;
                    extractstr(line, origcopy, frame, sizeof(line));    
                    pipe(pipes);            
                                        
                    processline(line, new, output, pipes, 1);
                        
                    insertstr(origcopy, output, new, frame);                    
                }
        }
        origindex++;
        orig++;
        next++;
    }
     
    /* Processing */
    if( syntaxflag){
        for(i = 2; i> -1; i--)
        {
            frame[i]          = stack[--stackindex];
            stack[stackindex] = 0;
        }
        
        if(frame[0] == SS_Z)
        {
            frame[1]          = 0;
            frame[2]          = maxsize;                
            bzero(line, sizeof(line));
            for(i = 0; i < 1024; i++)
            {
                line[i]       = *origcopy++;
            }
            processline(line, new, output, pipes, 0);
        }
    }

    
    bzero(new, LINELEN2);
    bzero(output, LINELEN);
    return syntaxflag;
}

/**************************************/
/*         START STACK OPS            */
/**************************************/

/*
 * Function:
 *      Inserts string
 * 
 *  Arguements:
 *      -the original user string represent as dest[]
 *      -output of some process
 *      -some buffer (actually new[2048]) used as a
 *          generic buffer
 * 
 *  Returns:
 *      Should return something
 * 
 */
int count_cmdsub(char* word)
{
    int    count;
    char*  next;
        
    count = 0;
    next  = word + 1;
    
    while(*next)
    {
        if( *word == '$' && *next == '(')        
            ++count;
        
        ++word;
        ++next;
    }
    
    return count;
}

/*
 * Function:
 *      Inserts string
 * 
 *  Arguements:
 *      -the original user string represent as dest[]
 *      -output of some process
 *      -some buffer (actually new[2048]) used as a
 *          generic buffer
 * 
 *  Returns:
 *      Should return something
 * 
 */
void extractstr(char* dest, char* src, int*frame, int destlen)
{    
    int   i;
    char* end;
    char* start;
     
    i       = 0;
    end     = src + *(frame + 2);        
    start   = src + *(frame + 1);    
    
    
    /* A priori: '$' already observed */
    while(*start != '(' )
    {
        *start = 32;
        start++;
    }
    *start = 32;
    
    while( start != end  && i < destlen)
    {
        *dest++ = *start;
        *start++ = 32;
    }
    *start = 32;
    *dest = 0;
}


/*
 * Function:
 *      Inserts string
 * 
 *  Arguements:
 *      -the original user string represented as dest[]
 *      -output of substituted command 
 *      -other[] buffer (actually new[2048]) used as a
 *          generic buffer
 * 
 *  Notes:
 *      -Both other[] and dest[] are of size LINELEN2 (2048)
 *      -input[] is of size LINELEN
 * 
 */
char* insertstr(char* dest, char* input, char* other, int*frame ){
        
        
        if(  (sizeof(dest) + sizeof(input) ) > LINELEN2){
            perror("Buffer overflow");
            getchar();
            exit(1);
        }
        
        
        int index;
        char * dcopy;
        
        index = 0;
        dcopy = dest;                
        
        bzero(other, LINELEN2);
        
        /* frame[2] contains relative end of command substitution */
        dest += (frame[2] + 1);
        
        /* Guaranteed to copy <= 2*1024 characters */        
        while( *dest)
            other[index++] = *dest++;
        
        other[index] = 0;        
        dest = dcopy;
        
        
        /* Terminate orig string starting at index of '$(' substring */
        dest[frame[1]] = 0;        
        dest = strncat(dest, input, strlen(input) );
        
        
        index = strlen(dest)  - 1;
        if( dest[index] == '\n')        
            dest[index] = 0;
        
        dest = strncat(dest, other, strlen(other) );
        
        index = strlen(dest)  - 1;
        if( dest[index] == '\n')        
            dest[index] = 0;        
                
        return dest;
}