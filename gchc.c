/*
Copyright (c) 2018 Joachim de Groot

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mysql.h>

enum stati {
	S200_OK,
	S500_BOO_BOO,
	S503_SERVICE_UNAVAILABLE
};

const int status_codes[] = {
	200,
	500,
	503
};

const char* status_msgs[] = {
	"OK",
	"Internal Server Error",
	"Service Unavailable"
};

const char *header = "HTTP/1.1 %d %s\r\n"
	"Content-Type: text/plain\r\n"
	"Connection: close\r\n"
	"Content-Length: %d\r\n"
	"\r\n"
	"%s\r\n";

MYSQL *conn  = NULL;
MYSQL_RES *res = NULL;


void print_status(int status, const char *message) {
	int len = message ? strlen(message) : 0;
	printf(header, status_codes[status], status_msgs[status], len, message);

	if (res) mysql_free_result(res);
	if (conn) mysql_close(conn);
	mysql_library_end();

	exit(status);
}

int main() {
	char *username = getenv("GCHC_USERNAME");
	char *password = getenv("GCHC_PASSWORD");
	char *host = getenv("GCHC_HOST");
	char *port = getenv("GCHC_PORT");
	char *socket = getenv("GCHC_SOCKET");
	char *_donor_available = getenv("GCHC_DONOR_AVAILABLE");
	bool donor_available = _donor_available ? atoi(_donor_available) : false;
	char *_timeout = getenv("GCHC_TIMEOUT");
	int timeout = _timeout ? atoi(_timeout) : 10;

	conn = mysql_init(NULL);
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

	if (!mysql_real_connect(conn, host, username, password, NULL,
				port != NULL ? atoi(port) : 3306, socket, 0)
	) {
		print_status(S500_BOO_BOO, mysql_error(conn));
	}

	if (mysql_query(conn, "SHOW STATUS LIKE 'wsrep_local_state'")) {
		print_status(S500_BOO_BOO, mysql_error(conn));
	}

	res = mysql_use_result(conn);
	MYSQL_ROW row = mysql_fetch_row(res);

	if (!row) {
		print_status(S500_BOO_BOO, "Node is not clustered.");
	} else if (atoi(row[1]) == 4) {
		print_status(S200_OK, "Galera Cluster Node is synced.");
	} else if (atoi(row[1]) == 2 && donor_available) {
		print_status(S200_OK, "Galera Cluster Node is available as donor.");
	} else {
		print_status(S503_SERVICE_UNAVAILABLE, "Galera Cluster Node is not synced.");
	}

	return EXIT_FAILURE;
}
