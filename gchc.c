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
	printf(header, status, status_msgs[status], len, message);

	if (res) mysql_free_result(res);
	if (conn) mysql_close(conn);
	mysql_library_end();

	exit(status);
}

int main(int argc, char* argv[]) {
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
	} else if (row[1] == "4") {
		print_status(S200_OK, "Galera Cluster Node is synced.");
	} else if (row[1] == "2" && donor_available) {
		print_status(S200_OK, "Galera Cluster Node is available as donor.");
	} else {
		print_status(S503_SERVICE_UNAVAILABLE, "Galera Cluster Node is not synced.");
	}

	return EXIT_FAILURE;
}