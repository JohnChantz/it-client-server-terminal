#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char parse(char *vector[10], char *line){
        int i;
        char * pch;
        
        pch = strtok (line," ");
        i=0;
        while (pch != NULL)
        {
                vector[i]=pch;
                printf ("%s\n",pch);
                pch = strtok (NULL, " ");
                i++;
        }
        printf("i = %d\n ", i);
        vector[i]=NULL;
        int k=0;
        for(k=0; k<=i; k++) {
                printf("vector %d = %s \n",k,vector[k]);
        }
        return vector;
}
main()
{
        char line[1024];
        int i;
        char *vector[10];
        char *buffer;
        size_t bufsize = 32;
        size_t characters;
        int pid,status;
        if ((pid=fork())==-1) /*check for error*/
        {
                perror("fork");
                exit(1);
        }
        if (pid!=0) /*The parent process*/
        {
                printf("I am parent process %d\n",getpid());
                while (wait(&status)!=pid) ;  /*Wait for child*/
                printf("Child terminated with exit code %d\n",
                       status>>8);
        }
        else /*The child process*/
        {

                printf("Shell -> "); /*   display a prompt             */
                gets(line);    /*   read in the command line     */
                printf("\n");
                printf("You typed %s\n",line);
                parse(vector,line);
                printf("I am child Process %d \n",getpid());
                printf("and i will replace myself by %s\n",line);
                execvp(vector[0],vector); /*Execute date*/
                perror("execvp");
                exit(1);
        }
}
