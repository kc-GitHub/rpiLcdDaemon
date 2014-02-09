/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    client.c
 * Description: This is a simple socket Client for communication with the
 *              rpiLcdDaemon
 * Author:      Dirk Hoffmann <hoffmann@vmd-jena.de> 2013-2014
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
 *******************************************************************************/

//=== Includes =================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

//=============================================================================

/**
 * Print error message and exit
 *
 * Parameter:	char	*msg	The error message
 * Return:		void
 */
void error(const char *msg) {
	perror(msg);
	exit(0);
}

/**
 * The main function
 *
 * Parameter:	int		argc
 * Parameter:	char	argv
 * Return:		int
 */
int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	if (argc < 3) {
		fprintf(stderr,"usage %s port message\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	server = gethostbyname("localhost");
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
	}

//	buffer = argv[2];
	n = write(sockfd,argv[2],strlen(argv[2]));
	if (n < 0) {
		error("ERROR writing to socket");
	}

	bzero(buffer,256);
	n = read(sockfd,buffer,255);
	if (n < 0) {
		error("ERROR reading from socket");
	}

	printf("%s\n",buffer);
	close(sockfd);
	return 0;
}
