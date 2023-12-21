/*
 * CS354: Shell project
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <string>

#include "command.h"
#include <fcntl.h>   
#include <sys/stat.h> 
#include <sys/types.h>

//part3
void childIsTerminated()
{
    FILE *logFile = fopen("Log.txt","a");
    time_t current_time = time(NULL);
    tm *timeformatted = gmtime(&current_time);
    fprintf(logFile,"Child is terminated at : %d %d %d\n",timeformatted->tm_hour,timeformatted->tm_min,timeformatted->tm_sec);
    fclose(logFile);
}

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) ); 
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	//here
	_append = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
			
		}
		printf( "\n" );
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	
	// Print contents of Command data structure
	print();
	
	// Save default input, output, and error because we will
	// change them during redirection and we will need to restore them
	// at the end.
	// The dup() system call creates a copy of a file descriptor.
	int defaultin = dup( 0 ); // Default file Descriptor for stdin
	int defaultout = dup( 1 ); // Default file Descriptor for stdout
	int defaulterr = dup( 2 ); // Default file Descriptor for stderr
	int fdpipe[2];
	
	if ( pipe(fdpipe) == -1) {
		perror( "pipe error\n");
		exit( 2 );
	}
	
	int input_fd, output_fd, error_fd;
	
	//part3
	for(int i = 0; i < _numberOfSimpleCommands; i++)
    	{
		int n;
		char *command = _simpleCommands[i]->_arguments[0];
		
		if(!strcasecmp(command,"exit"))
		{
		    printf("Good Bye!!\n");
		    exit(1);
		}

		if(!strcasecmp(command,"cd"))
		{
		    if(_simpleCommands[i]->_numberOfArguments > 1)
		    {
			std::string path = _simpleCommands[i]->_arguments[1];
			n = path.length();
			char pathArray[n+1];
			strcpy(pathArray, path.c_str());
			if(chdir(pathArray)) //on success, zero is returned
			{
			    printf("Path not found.\n");
			}
			break; // do not continue fork loop
		    }
		    else   // home
		    {
			//chdir("/mnt/");
			chdir(getenv("HOME"));
			break;
		    }
		}
		     
          
        if( i == 0 ) // parent, first command
        {
		// Input Redirection
		if (_inputFile)
		{
		    input_fd = open(_inputFile, O_RDONLY);
		    if (input_fd < 0)
		    {
		        perror("open input file");
		        exit(EXIT_FAILURE);
		    }
		    dup2(input_fd, 0);
		    close(input_fd);
		}
		else  // no input file specified, use default
		 {
		    	dup2( defaultin, 0 );
		 }
				         
		// Error Redirection
		if (_errFile)
		{
		    error_fd = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		    if (error_fd < 0)
		    {
		        perror("open error file");
		        exit(EXIT_FAILURE);
		    }
		    dup2(error_fd, 2);
		    close(error_fd);
		}
		else  // no output file specified, use default
		 {
		 	error_fd = dup(defaulterr);
		 }
         
         }
         else if ( i != 0 ) // child process
         {
         	dup2(fdpipe[0],0); // read input from previous pipe
         	close(fdpipe[0]);
         	close(fdpipe[1]);
         	if(pipe(fdpipe)==-1)
            {
                perror("piping error");
                exit(2);
            }
         }
 
	if(i == _numberOfSimpleCommands-1)  // last simpleCommand, no more pipes
	{
		if (_outFile)
		{

		    if (_append)
		    {
		        output_fd = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
		    }
		    else
		    {
		        output_fd = creat(_outFile, 0666);
		    }
		    if (output_fd < 0)
		    {
		        perror("open output file");
		        exit(EXIT_FAILURE);
		    }
		     dup2(output_fd, 1);
		     //close(output_fd);  // may be unnecessary
		}
		else{
		    	dup2(defaultout, 1);
		    	//close( defaultout ); // may be unnecessary

		    }
	}
	else if(i !=_numberOfSimpleCommands-1)  // not the last simpleCommand, pipes after it
	{
		dup2(fdpipe[1],1); // redirect output to pipe
		//close(fdpipe[1]);  
		//close(fdpipe[0]);
	}
         
         
	// For every simple command fork a new process

	    	/* fork a child process */
    		int pid = fork();
    		
	    	if (pid < 0) { /* error occurred */
			fprintf(stderr, "Fork Failed");
			exit(2);
	   	 }
	    	else if (pid == 0) { /* child process */
	    
	    		
	    		// close file descriptors that are not needed
			close(fdpipe[0]);
			close(fdpipe[1]);
			close(input_fd);
			close(output_fd);
			close(error_fd);
			childIsTerminated();
			close( defaultin );
			close( defaultout );
			close( defaulterr );    		
		
			// and call exec
			execvp(command, _simpleCommands[i]->_arguments);
			
			// exec() is not suppose to return, something went wrong
			perror( "Error");
			exit( 3 );
	    	}
	    	else{  // parent process
		    	
		    	// Restore input, output, and error

			dup2( defaultin, 0 );
			dup2( defaultout, 1 );
			dup2( defaulterr, 2 );
		    	
		    	if (! _background) { /* parent process */
				
				/* parent will wait for the child to complete */
				waitpid( pid,0,0);
		 	}
		 	
		 	
		
	 	}
	} 


	// Close file descriptors that are not needed
	//close(fdpipe[0]);
	//close(fdpipe[1]);
	
	if(_inputFile){
		close(input_fd);
	}
	if(_outFile){
		close(output_fd);
	}
	if(_errFile){
		close(error_fd);
	}
	//close(output_fd);
	//close(error_fd);
	
	close( defaultin );
	close( defaultout );
	close( defaulterr );

	
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

//part3
void ignoreCtrlC(int Signal)
{
    printf("\nTo exit type exit\n");
    Command::_currentCommand.prompt();
    
}
int
main()
{
    //part3
    signal(SIGINT,ignoreCtrlC);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}
