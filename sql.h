#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysql.h"
#include "errmsg.h"
#include "mysqld_error.h"

void connectDatabase(MYSQL *conn, const char* host, const char* user, const char* password, const char* database);
void insertDatabase(MYSQL *conn, char *sql);
void updateDatabase(MYSQL *conn, char *sql);
void deleteDatabase(MYSQL *conn, char*sql);
void finishWithError(MYSQL *conn);
void queryDatabase(MYSQL *conn, char *sql);
