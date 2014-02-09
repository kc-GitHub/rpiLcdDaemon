/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    server.c
 * Description: The server component. Implements a simple socket server
 *              listening on the internal loopback device
 * Author:      Dirk Hoffmann <hoffmann@vmd-jena.de> 2013
 *
 *******************************************************************************
 * Open Source Licensing
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen der
 * GNU General Public License, wie von der Free Software Foundation, Version 3
 * der Lizenz oder (nach Ihrer Option) jeder späteren veröffentlichten Version,
 * weiterverbreiten und/oder modifizieren.
 *
 * Dieses Programm wird veröffentlicht in der Hoffnung, dass es nützlich sein
 * wird, aber OHNE JEDE GEWÄHRLEISTUNG; sogar ohne die implizite Gewährleistung
 * der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK. Siehe die GNU
 * General Public License f�r weitere Details.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
 *******************************************************************************
 *
 * History:     25.05.2013 Initial version V0.0.1
 *
 *******************************************************************************/

//=== Includes =================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h>

#include "utility.h"
#include "server.h"
#include "command.h"

//==============================================================================

/**
 * Start the socket server on internal loopback device
 *
 * Parameter:	int		portNum		The port number for the connection.
 * Return:		void
 */
void startServer(int portNum) {
	int clientLen, clientSocketFD, sockFD;
	struct sockaddr_in server , client;

	int *clientSocket;

	//Create socket
	sockFD = socket(AF_INET, SOCK_STREAM, 0);


	if (sockFD < 0) {
		error("Could not create socket: %s", strerror (errno));
	}

	int val = 1;
	if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
		error("Setsockopt: %s", strerror (errno));
	}


	// no TIME_WAIT of socket connection
	struct linger so_linger;
	so_linger.l_onoff = 1;
	so_linger.l_linger = 0;
	if (setsockopt(sockFD, SOL_SOCKET, SO_LINGER, &so_linger, sizeof so_linger)) {
		error("Setsockopt: %s", strerror (errno));
	}

	syslogDebug ("Socket created.");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;

	if (allowRemote != 1) {
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);						// accept connections from loopback address only
	}

	server.sin_port = htons(portNum);

	// bind
	if (bind(sockFD, (struct sockaddr *) &server, sizeof(server)) < 0) {
		error("Port binding failed: %s", strerror (errno));
	}

	//Listen
	listen(sockFD, 3);

	syslogInfo("Listening on port %i for incoming connections.", portNum);

	clientLen = sizeof(struct sockaddr_in);

	pthread_attr_t tattr;

	/* initialized with default attributes */
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);

	while( (clientSocketFD = accept(sockFD, (struct sockaddr *) &client, (socklen_t*)&clientLen)) ) {

		pthread_t sniffer_thread;
		clientSocket = malloc(1);
		*clientSocket = clientSocketFD;

		syslogInfo("%i: Client connection from %s accepted.", clientSocketFD, inet_ntoa(client.sin_addr) );
		if (firstClient == 0) {
			firstClient = *(int*)clientSocket;
			syslogInfo("%i: This is the first client. It received the key events.", firstClient );
		}

		if( pthread_create( &sniffer_thread , &tattr , connection_Handler , (void*) clientSocket) < 0) {
			close(sockFD);
			syslogWarning("%i: Could not create thread: %s", clientSocketFD, strerror (errno));
		}
	}

	if (clientSocketFD < 0) {
		error("%i: Accept failed: %s", clientSocketFD, strerror (errno));
	}

	close(clientSocketFD);
	close(sockFD);
}

/**
 * This will handle connection for each client
 *
 * Parameter:	void	clientSock
 * Return:		void
 */
void *connection_Handler(void *clientSock) {
	//Get the socket descriptor
	int sock = *(int*)clientSock;

	struct sockaddr_in addr;
	socklen_t socketLen = sizeof(addr);
	getsockname(sock, (struct sockaddr*)&addr, &socketLen);

	int readSize;

	char *message;
	char *commandLine;
	char buffer[4096];

	char *response;
	char response2[80];

	int i;
	int exit = 0;

	// Send Greeting
	message = "RaspberryPi LCD deamon\n";
	write(sock , message , strlen(message));

	//Receive a message from client
	while( (readSize = recv(sock, buffer, sizeof(buffer), 0)) > 0){
		if (readSize > 2 && readSize < 4096) {

			int commandLineCount = 0;
			char **commandLineArray = textSplit(buffer, "\n\r", &commandLineCount);

			for (i = 0; i < commandLineCount; i++){

				commandLine = trim(commandLineArray[i]);
				syslogDebug ("%i: Command from %s: %s", sock, inet_ntoa(addr.sin_addr), buffer);

				if (strlen(commandLine) > 2) {
					if (strcmp(commandLine, "exit") == 0) {
						exit = 1;
						break;

					} else {

						if (strcmp(commandLine, "shutdown") == 0) {
							doShutdown = 1;
							break;
						}

						response = analyseCommand(commandLine);
						syslogDebug ("%i: %s", sock, response);

						//Send the message back to client
						sprintf(response2, "%s\n", response);
						write(sock , response2 , sizeof(response2));
					}
				}

			}

			if (doShutdown || exit) {
				break;
			}

		} else if (readSize >= 4096) {
			char *overloadMsg = "Command packet gerater than 4096 bytes.\n";
			syslogWarning("%i: %s: from %s", sock, overloadMsg, inet_ntoa(addr.sin_addr));
			write(sock , overloadMsg , strlen(overloadMsg));
		}

		memset(buffer,0,256);
	}

	if(readSize == 0 || exit || doShutdown) {
		syslogInfo("%i: Client (%s) disconnected", sock, inet_ntoa(addr.sin_addr));
		fflush(stdout);

	} else if(readSize == -1) {
		syslogInfo("%i: Client (%s) connection is lost unexpectedly.", sock, inet_ntoa(addr.sin_addr));
	}

	close(sock);
	free (clientSock);

	if (sock == firstClient) {
		firstClient = 0;
	}

	if (doShutdown) {
		cmd_shutdown();
	}

	return 0;
}
