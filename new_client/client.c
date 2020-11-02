#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <common_def.h>

// main function check

int main(int argc, char*argv[])
{
	char buff[10000];
    unsigned int pt_no;
    struct sockaddr_in address_ip4, remote_address_ip4;
    int client_fd;
    char html_request[200];
    char *name_of_host;
    char *path;
    char *path_copy;
    char *path_counter;
    int count = 0, flag_end = 0;
	
	if (argc<4)// incorrect input 
	{
		system_error("CLIENT: ./client <ip> <port> <url>");
	}
	
	pt_no = atoi(argv[2]); // port number to string
    
	
	
    bzero(&remote_address_ip4, sizeof(remote_address_ip4));
	
	remote_address_ip4.sin_family = AF_INET;
    remote_address_ip4.sin_port = htons(pt_no);// same as previous mps
    if(inet_aton(argv[1], (struct in_addr *)&remote_address_ip4.sin_addr.s_addr) == 0){
        system_error(argv[1]);
        
    }
    client_fd = socket(AF_INET, SOCK_STREAM, 0); //  
    if(client_fd < 0){
        system_error("Socket Creating Error:");
        
    }
	// connect  
    if(connect(client_fd, (struct sockaddr *)(&remote_address_ip4), sizeof(struct sockaddr)) < 0){
        system_error("Connection Error:");
        
    }

// clearing the contents of the request
    memset(html_request, 0, 100);
    name_of_host = strtok(argv[3],"/");// first occurence of token return 
    path = strtok(NULL, "");// carriege return  
    sprintf(html_request,"GET /%s HTTP/1.0\r\nHost:%s\r\n\r\n", path, name_of_host);
    path_counter = path;
	//checking the end of file  
    while(*path_counter != '\0'){
        if(*path_counter == '/'){
            count++;
        }
        path_counter++;
    }
    path_copy = path;
    if(count > 0){
        while(count >= 0){
            if(flag_end == 0){
                strtok(path_copy, "/");//  
                flag_end = 1;
            }
            else
            {
                path_counter = strtok(NULL, "/");
            }
            count--;
        }
    }
    else{
        path_counter = path_copy;
    }


    if((send(client_fd, html_request, strlen(html_request), 0)) == -1){
        system_error("Send Error:");
        exit(-1);
    }

   
	

	// file pointer open check 
    FILE *ptr_file ;
    ptr_file= fopen(path_counter, "w");
    int msg_length;
	
    memset(buff,0,10000); 
    
	if((msg_length = recv(client_fd, buff, 10000,0)) <= 0){
    }
	// if the html page is non reachable  
    else if((*(buff+9) == '4') && (*(buff+10) == '0') && (*(buff+11) == '4')){
        printf("%s", buff);
        remove(path_counter); // remove function call  
    }
    else
    {// the file received seems ok, return the first occurance of any terminators.
        char * tp = strstr(buff, "\r\n\r\n");
        fwrite(tp+4, 1, strlen(tp)-4, ptr_file);
        memset(buff, 0, 10000);// write to the client end. 
        while((msg_length = recv(client_fd, buff, 10000,0)) > 0){// keep on writing unless zero is returned 
            fwrite(buff, 1, msg_length,ptr_file);
            memset(buff, 0, 10000);

        }
        printf("SUCCESS: Read from the server\n");
        printf("File received : %s\n",path_counter);
    }
    fclose(ptr_file);
    close(client_fd);
    return 0;
}




















