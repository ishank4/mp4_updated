#include <stdio.h>
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
// Main function call
int main(int argc, char *argv[])
{
// variables 
	char ack_req[1024];
	int max_fd;
	int socket_ITR;
	char ac_Buffer[BUFFER_SIZE], ac_Host[NAME_LENGTH],ac_u_r_l[NAME_LENGTH], ac_Name[NAME_LENGTH];
	int index_in_cache = -1;
	int port_http = PORT_NO;
	int proxy_fd;
	int i, old_destination_entry =0;
	FILE *read_filepointer;
	char *expire = NULL;
	char *pc_token = NULL;
	int read_len =0;
	char modified [100];
	char m_req[BUFFER_SIZE];
	time_t now;
	struct tm *timenow;
	struct addrinfo hint, *res;
	int s_len;
	FILE *fp;  //filepointer
	int rcv_len = -1;
	int listener_fd, client_fd;
	struct sockaddr_storage remote_addr;
	struct sockaddr_in addr;
	socklen_t sin_size;
	fd_set set_1;
	fd_set set_2;

	memset(cache,0,10*sizeof(_cache));
	if(argc < 3)  //incorrect arg usage
	{
		printf("\n INCORRECT USAGE: ./<server> <host> <port> \n");
		exit(-1);
	}
	memset(&remote_addr,0, sizeof remote_addr);
	memset(&addr,0,sizeof addr);

	port_http = atoi(argv[2]);
	addr.sin_port = htons(port_http);
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_family = AF_INET;
// listening to fd
	if((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) <0)
	{
		system_error("ERROR:  socket");
	}
	if(bind(listener_fd,(struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
	{
		system_error("ERROR: In bind socket");
		exit(-1);
	}
	if(listen(listener_fd,10) <0)
	{
		system_error("ERROR: In listen");
		exit(-1);
	}

	FD_SET(listener_fd, &set_1);
	max_fd= listener_fd;

	memset(cache,0,10*sizeof(_cache));
//search for client
	printf("Looking for active client \n");
	sin_size = sizeof(remote_addr);

	while(1)
	{
		set_2= set_1;
		if(select(max_fd+1, &set_2, NULL, NULL, NULL) == -1){
			system_error("ERROR: In select");
			exit(-1);
		}  // loop to reach maximum fd
		for (socket_ITR = 0; socket_ITR <=max_fd; socket_ITR++)
		{
			if(FD_ISSET(socket_ITR, &set_2))
			{
				if(socket_ITR == listener_fd)
				{
					client_fd= accept(listener_fd,(struct sockaddr*)&remote_addr,&sin_size);
					max_fd= client_fd>max_fd ? client_fd : max_fd;
					FD_SET(client_fd, &set_1);
					if(client_fd == -1)
					{
						system_error("ERROR: In accept");
						continue;
					}
				}
				else
				{
					memset(ac_Buffer, 0, BUFFER_SIZE);
					memset(ac_Host, 0, NAME_LENGTH);
					memset(ac_u_r_l, 0, NAME_LENGTH);
					memset(ac_Name, 0, NAME_LENGTH);
//client check
					client_fd= socket_ITR;
					if(recv(client_fd, ac_Buffer, sizeof(ac_Buffer),0) < 0)
					{
						system_error("ERROR: In recv");
						close(client_fd);
						return 1;
					}
					printf("Message rcvd from  CLIENT : \n%s \n", ac_Buffer);
					if(parse_read_request(ac_Buffer, ac_Host, &port_http, ac_u_r_l, ac_Name)!=4)
					{
						send_error_message(400, client_fd);
						close(client_fd);
						return 1;
					}
					if((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) <0){
						system_error("ERROR: In PROXYSOCKET ERROR");
						close(client_fd);
						return 1;
					}

					memset(&hint, 0, sizeof(hint));
					hint.ai_family = AF_INET;
					hint.ai_socktype = SOCK_STREAM;

					if(getaddrinfo(ac_Host,"80", &hint, &res)!=0)
					{
						printf("ERROR: GETADDRINFO\n");
						send_error_message(404, client_fd);
						close(client_fd);
						return 1;
					}
					if(connect(proxy_fd, res->ai_addr, res->ai_addrlen) < 0)
					{
						close(proxy_fd);
						system_error("ERROR: IN connect");
						send_error_message(404, client_fd);
						close(client_fd);
						return 1;
					}
					time(&now);
					timenow = gmtime(&now);
// time check from GMT 
					index_in_cache = check_if_cache_entry_present(ac_u_r_l);
					if(index_in_cache == -1)
					{
						memset(ack_req, 0, 1024);
						printf("FILE NOT IN CACHE ...\n Downloading from %s:%d\n",ac_Host, port_http );
						printf("\n SENDING ... Request sent to the main server");
						sprintf(ack_req, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", ac_Name, ac_Host);
						puts(ack_req);
						if((s_len = send(proxy_fd, ack_req, strlen(ack_req),0)) <0)
						{
							system_error("ERROR: in send request");
							close(proxy_fd);
							return 1;
						}
						printf("REQUEST rcvd by main SERVER \n");
						memset(ac_Buffer ,0, BUFFER_SIZE);
						for (i = 0; i< 10; i++)
						{
							if(cache[i].Is_filled == 0)
							{
								old_destination_entry = i;
								break;
							}
							else
							{
								if(time_comparison(cache[i].Last_Modfd, cache[old_destination_entry].Last_Modfd) <= 0)
								{
									old_destination_entry = i;
								}

							}
						}
						memset(&cache[old_destination_entry], 0, sizeof(_cache));
						cache[old_destination_entry].Is_filled = 1;

						pc_token= strtok(ac_Name,"/");

						while(pc_token != NULL)
						{
							strcpy(cache[old_destination_entry].Fname, pc_token);
							pc_token = strtok(NULL, "/");
						}
// memmory copy 
						memcpy(cache[old_destination_entry].u_r_l, ac_u_r_l, NAME_LENGTH);
						sprintf(cache[old_destination_entry].Last_Modfd,"%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, timenow->tm_min, timenow->tm_sec  );

						remove(cache[old_destination_entry].Fname);
						fp = fopen(cache[old_destination_entry].Fname, "w");
						if(fp == NULL)
						{
							printf("ERROR : Failed to make CACHE database\n");
							return 1;
						}
						while((rcv_len = recv(proxy_fd, ac_Buffer, BUFFER_SIZE, 0)) > 0)
						{
							if(send(client_fd, ac_Buffer, rcv_len,0) <0)
							{
								system_error("ERROR: Client send");
								return 1;
							}
							fwrite(ac_Buffer, 1, rcv_len, fp);
							memset(ac_Buffer, 0, BUFFER_SIZE);
						}
						send(client_fd, ac_Buffer, 0, 0);
						printf("RESPONSE rcvd from SERVER\n");
						printf("FILE sent to the client requester \n ");
//closing the file pointer
						fclose(fp);

						read_filepointer = fopen(cache[old_destination_entry].Fname, "r");
						printf("old : %d\n",old_destination_entry);
						fread(ac_Buffer, 1, 2048, read_filepointer);
						fclose(read_filepointer);

						expire = strstr(ac_Buffer, "EXPIRY in : ");
						if(expire != NULL)
						{
							memcpy(cache[old_destination_entry].Exp, expire+9, 29);
						}
						else
						{
							sprintf(cache[old_destination_entry].Exp, "%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, (timenow->tm_min)+2, timenow->tm_sec );			
						}
					}
					else
					{
// if time out happens 	
					if(check_if_cache_entry_expire(ac_u_r_l, timenow) >=0)
						{
							printf("FILE PRESENT: in cache AND NOT EXPIRED\n Sending file to client..\n");
							sprintf(cache[index_in_cache].Last_Modfd,"%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, timenow->tm_min, timenow->tm_sec  );
							read_filepointer = fopen(cache[index_in_cache].Fname, "r");
							printf("Index in cache:  %d\n", index_in_cache);
							memset(ac_Buffer, 0, BUFFER_SIZE);
							while((read_len= fread(ac_Buffer,1 , BUFFER_SIZE, read_filepointer)) > 0)
							{	//printf("Inside while before send\n");
								send(client_fd, ac_Buffer, read_len, 0);
							}
							printf("SUCCESS: SENT FILE TO CLIENT");
							fclose(read_filepointer);
						}
						else
						{
							printf("FILE has been expired. SENDING request for latest file to main SERVER \n");
							memset(modified, 0, 100);
							sprintf(modified, "If_MODIFIED_ SINCE : %s\r\n\r\n", cache[index_in_cache].Last_Modfd);
							memset(m_req, 0, BUFFER_SIZE);
							memcpy(m_req, ac_Buffer, strlen(ac_Buffer)-2);

							strcat(m_req, modified);
							printf("SENDING HTTP QUERY to the  WEB SERVER\n");
							printf("%s\n", m_req);
							send(proxy_fd, m_req, strlen(m_req),0);
							memset(ac_Buffer,0, BUFFER_SIZE);
							rcv_len = recv(proxy_fd, ac_Buffer, BUFFER_SIZE, 0);
							expire =  strstr(ac_Buffer, "Expires at : ");
							if(expire != NULL)
							{
								memcpy(cache[index_in_cache].Exp, expire+9, 29);
							}
							else
							{
								sprintf(cache[index_in_cache].Exp, "%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, timenow->tm_min, timenow->tm_sec );

							}
							if(rcv_len > 0)
							{
								printf("HTTP RESPONSE: \n %s\n",ac_Buffer );
								if((*(ac_Buffer + 9) == '3') && (*(ac_Buffer + 10) == '0') && (*(ac_Buffer + 11) == '4')) //3040response <---BONUS IMPLEMENTED
								{
									printf("FILE is UPTO the DATE. sending file from cache\n");
									read_filepointer = fopen(cache[index_in_cache].Fname, "r");
									memset(ac_Buffer, 0, BUFFER_SIZE);
									while((read_len = fread(ac_Buffer, 1, BUFFER_SIZE, read_filepointer)) >0)
									{
										send(client_fd, ac_Buffer, read_len, 0);
									}
									fclose(read_filepointer);
								}	
								else if ((*(ac_Buffer+9) == '4') && (*(ac_Buffer +10 ) =='0') &&(*(ac_Buffer+11) == '4'))
								{
									send_error_message(404, client_fd);
								}
								else if((*(ac_Buffer+9) =='2') && (*(ac_Buffer +10) == '0') && (*(ac_Buffer +11) == '0'))
								{
									printf("LATEST FILE RECEIVED FROM SERVER.\n UPDATED CACHE ENTRY AND SENT FILE TO CLIENT\n");
									send(client_fd, ac_Buffer, rcv_len, 0);
									remove(cache[index_in_cache].Fname);

									expire = NULL;
									expire = strstr(ac_Buffer, "EXPIRES:");
									if(expire != NULL)
									{
										memcpy(cache[index_in_cache].Exp, expire+9,29);
									}
									else
									{
										sprintf(cache[index_in_cache].Exp, "%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, timenow->tm_min, timenow->tm_sec );

									}
									sprintf(cache[index_in_cache].Last_Modfd, "%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday], timenow ->tm_mday, month[timenow ->tm_mon], timenow->tm_year+ 1900, timenow->tm_hour, timenow->tm_min, timenow->tm_sec );
									fp = fopen(cache[index_in_cache].Fname, "w");
									fwrite(ac_Buffer, 1, rcv_len, fp);

									memset(ac_Buffer, 0, BUFFER_SIZE);
									while((rcv_len = recv(proxy_fd, ac_Buffer, BUFFER_SIZE, 0)) > 0)
									{
										send(client_fd, ac_Buffer, rcv_len,0);
										fwrite(ac_Buffer, 1, rcv_len, fp);
									}
									fclose(fp);

								}
							}
							else
							{
								system_error("ERROR: In rcv");
							}
						}
					}
					FD_CLR(client_fd, &set_1);
					close(client_fd);
					close(proxy_fd);
					printf("\n CACHE TABLE \n");
					for (i=0;i<10;i++)
					{
						if(cache[i].Is_filled)
						{
							printf("Cache INDEX NUMBER %d\n", i + 1 );
							printf("URL              : %s\n",cache[i].u_r_l);
							printf("Last Access AT   : %s\n",cache[i].Last_Modfd);
							printf("Expiry in        : %s\n", cache[i].Exp );
							printf("File             : %s\n", cache[i].Fname);
						}
					}
				}
			}
		}

	}
}
