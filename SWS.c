/* 	
	Simple Web Server
	Necula Alexandru
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>

#define PORT 8080
#define logFile "serverLog.log"

char * getFileName(char *filePath)
{
	// Get the string after the last / in file path
	char *filePathCopy = malloc(strlen(filePath));
	strcpy(filePathCopy, filePath);
	char *token = strtok(filePathCopy, "/");
	char *fileName = malloc(10);
	strcpy(fileName, token);
	while((token = strtok(NULL, "/")))
		strcpy(fileName, token);

	free(filePathCopy);
	return fileName;
}

char * sendResponseContent(char *filePath, int client)
{
	// Check which main file is available
	if(!strcmp(filePath, "/"))
	{
		struct stat checkFile;
		if(!stat("index.html", &checkFile))
			strcpy(filePath, "/index.html");
		else if(!stat("main.html", &checkFile))
			strcpy(filePath, "/main.html");
		else if(!stat("index.htm", &checkFile))
			strcpy(filePath, "/index.htm");
		else if(!stat("main.htm", &checkFile))
			strcpy(filePath, "/main.htm");
		else if(!stat("index.php", &checkFile))
			strcpy(filePath, "/index.php");
		else if(!stat("main.php", &checkFile))
			strcpy(filePath, "/main.php");
		else
		{
			perror("[ERROR] Starting file is needed.");
			exit(-3);
		}
	}
	char *fileName = getFileName(filePath);
	// Build the header
	char header[255];
	strcpy(header, "HTTP/1.1 200 OK\n");
	strcat(header, "Server: C Web Server\n");
	// Find the extension of the file
	char *fileNameCopy = malloc(strlen(fileName));
	strcpy(fileNameCopy, fileName);
	char *fakeFileExtension = strtok(fileNameCopy, ".");
	char fileExtension[10];
	while(fakeFileExtension != NULL)
	{
		strcpy(fileExtension, fakeFileExtension);
		fakeFileExtension = strtok(NULL, ".");
	}
	// Setting the MIME Type
	bool binary = false, php = false;
	if(!strcmp(fileExtension, "html") || !strcmp(fileExtension, "htm") || !strcmp(fileExtension, "htmls"))
		strcat(header, "Content-Type: text/html; charset=utf-8\n");
	else if(!strcmp(fileExtension, "css"))
		strcat(header, "Content-Type: text/css; charset=utf-8\n");
	else if(!strcmp(fileExtension, "js"))
		strcat(header, "Content-Type: text/javascript; charset=utf-8\n");
	else if(!strcmp(fileExtension, "php"))
	{
		strcat(header, "Content-Type: text/html; charset=utf-8\n");
		php = true;
	}
	else if(!strcmp(fileExtension, "jpg") || !strcmp(fileExtension, "jpeg"))
	{
		strcat(header, "Content-Type: image/jpeg\n");
		binary = true;
	}
	else if(!strcmp(fileExtension, "png"))
	{
		strcat(header, "Content-Type: image/png\n");
		binary = true;
	}
	else if(!strcmp(fileExtension, "mpeg"))
	{
		strcat(header, "Content-Type: audio/mpeg\n");
		binary = true;
	}
	else if(!strcmp(fileExtension, "mp3"))
	{
		strcat(header, "Content-Type: audio/x-mpeg-3\n");
		binary = true;
	}
	else if(!strcmp(fileExtension, "ogg"))
	{
		strcat(header, "Content-Type: audio/ogg\n");
		binary = true;
	}
	else if(!strcmp(fileExtension, "ico"))
	{
		strcat(header, "Content-Type: image/x-icon\n");
		binary = true;
	}
	else
		strcat(header, "Content-Type: text/plain; charset=utf-8\n");
	strcat(header, "Connection: close\n");
	strcat(header, "Content-Length: ");
	// Reading from file
	FILE *fd = fopen(filePath + 1, "rb");
	if(fd == NULL)
	{
		perror("[ERROR] Could not open file.\n");
		exit(-1);
	}
	fseek(fd, 0, SEEK_END);
	int fileContentLength = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	char *fileContent;
	if(!php)
		fileContent = malloc(fileContentLength);
	// If it's a binary file, we will read char by char
	if(binary)
		for(int i = 0; i < fileContentLength; i++)
	    	fileContent[i] = fgetc(fd);
	// If it's a php file, we'll execute it in a child process and output the result in a file
	else if(php)
	{
		pid_t pid = fork();
		if(pid == -1)
		{
			perror("[ERROR] Fork error.");
			exit(-1);
		}
		else if(pid == 0)
		{
			int temp = open("temp.txt", O_RDWR | O_CREAT);
			dup2(temp, 1);
			execlp("php", "php", fileName, NULL);
			close(temp);
			exit(0);
		}
		else
		{
			wait(NULL);
			FILE *tempfile = fopen("temp.txt", "r");
			fseek(tempfile, 0, SEEK_END);
			fileContentLength = ftell(tempfile);
			fseek(tempfile, 0, SEEK_SET);
			fileContent = malloc(fileContentLength);
			fread(fileContent, fileContentLength, 1, tempfile);
			fclose(tempfile);
			//remove("temp.txt");
		}
	}
	else
		if(fread(fileContent, fileContentLength, 1, fd) != 1)
		{
			perror("[ERROR] Could not read from file.\n");
			exit(-4);
		}
	// Add the number to content-length
	char fileContentLengthString[15];
	sprintf(fileContentLengthString, "%d", fileContentLength);
	strcat(header, fileContentLengthString);
	strcat(header, "\n\n");
	// Send the response
	send(client, header, strlen(header), 0);
	send(client, fileContent, fileContentLength, 0);
	// Build the reply
	char *reply = malloc(strlen(header) + fileContentLength);
	strcpy(reply, header);
	strcat(reply, "File path: ");
	strcat(reply, filePath);
	strcat(reply, "\n\n");
	if(binary)
		strcat(reply, "Binary file.");
	else
		strcat(reply, fileContent);

	fclose(fd);
	free(fileNameCopy);
	free(fileContent);
	return reply;
}

int main()
{
	
	char *reply;
	// TCP socket
	int sd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverStruct;
	struct sockaddr_in clientStruct;
	bzero(&serverStruct, sizeof(serverStruct));
	bzero(&clientStruct, sizeof(clientStruct));
	serverStruct.sin_family = AF_INET;
	serverStruct.sin_port = htons(PORT);
	serverStruct.sin_addr.s_addr = INADDR_ANY;

	if(bind(sd,(struct sockaddr*) &serverStruct,sizeof(serverStruct)) == -1)
	{
	    perror("[ERROR] Could not bind.\n");
	    exit(-1);
	}

	if(listen(sd, 5) == -1)
	{
	    perror("[ERROR] Could not listen.\n");
	    exit(-2);
	}
	// Remove the log file
	int connectionNR = 0;
	if(remove(logFile) == -1)
		perror("[ERROR] Could not remove log file.");
	while(1)
	{
		fflush (stdout);
		int len;
	    unsigned int size = sizeof(serverStruct);
	    int client = accept(sd, (struct sockaddr *)&clientStruct, &size);

	    if(client == -1)
	    {
	    	printf("Client failed to connect.\n");
	    	continue;
	    }
	    else
	    {
	    	int pid;
	    	++connectionNR;
	    	if ((pid = fork()) == -1)
	    	{
	    		close(client);
	    		continue;
	    	}
	    	else if (pid > 0)
	    	{
	    		close(client);
	    		while(waitpid(-1, NULL, WNOHANG));
	    		continue;
	    	}
	    	else if (pid == 0)
	    	{
	    		close(sd);
	    		FILE *log = fopen(logFile, "a");
	    		if(log == NULL)
	    			perror("[ERROR] Could not log to file.\n");
		    	char request[1024];
		        printf("********************** Connection number %d *************\n\n", connectionNR);
		        fprintf(log, "********************** Connection number %d *************\n\n", connectionNR);
		        bzero(request, sizeof(request));
		        // Receive the request
		        if((len = recv(client, request, sizeof(request), 0)))
		        {
		        	request[len] = 0;
		        	printf("********************** Request %d ***********************\n%s\n", connectionNR, request);
		        	fprintf(log, "********************** Request %d ***********************\n%s\n", connectionNR, request);
		        	char *filePath = strtok(request, " \n");
		        	filePath = strtok(NULL, " \n");
					reply = sendResponseContent(filePath, client);
					printf("********************** Response %d **********************\n%s\n", connectionNR, reply);
					fprintf(log, "********************** Response %d **********************\n%s\n", connectionNR, reply);
		        }
		        fclose(log);
		        close(client);
		        exit(0);
		    }
	    }
	    printf("\n\n");
	}
	return 0;
}