#include "sql.h" 
 
void connectDatabase(MYSQL *conn, const char* host, const char* user, const char* password, const char* database) {
	mysql_init(conn);
 
	if (mysql_real_connect(conn, host, user, password, database, 0, NULL, 0)) {
		printf("Connection success!\n");
	} else {
		fprintf(stderr, "Connection failed!\n");
		if (mysql_errno(conn)) {
			fprintf(stderr, "Connection error %d: %s\n", mysql_errno(conn), mysql_error(conn));
		}
		exit(EXIT_FAILURE);
	}
}
 

void insertDatabase(MYSQL *conn, char *sql) {
	int res = mysql_query(conn, sql);
	if (!res) {
		printf("Inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
	} else {
		fprintf(stderr, "Insert error %d: %s\n", mysql_errno(conn), mysql_error(conn));
	}
}
 
void updateDatabase(MYSQL *conn, char *sql) {
	int res = mysql_query(conn, sql);
	if (!res) {
		printf("Update %lu rows\n", (unsigned long)mysql_affected_rows(conn));
	} else {
		fprintf(stderr, "Update error %d: %s\n", mysql_errno(conn), mysql_error(conn));
	}
}
 
void deleteDatabase(MYSQL *conn, char*sql) {
	int res = mysql_query(conn, sql);
	if (!res) {
		printf("Delete %lu rows\n", (unsigned long)mysql_affected_rows(conn));
	} else {
		fprintf(stderr, "Delete error %d: %s\n", mysql_errno(conn), mysql_error(conn));
	}
}
//spit out mysql_error for connection
void finishWithError(MYSQL *conn)
{
    fprintf(stderr, "%s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

void queryDatabase(MYSQL *conn, char *sql)
{
    if (mysql_query(conn, sql))
    {
        finishWithError(conn);
    }
}

int main (int argc, char *argv[]) {
 
    MYSQL *conn;
	connectDatabase(conn, "localhost", "root", "cldai-gpu123--", "shopdb");
    char *sql = "select ad_id, mac from products";

    queryDatabase(conn, sql);
    //store result
  MYSQL_RES *result = mysql_store_result(conn);
  if(result == NULL)
  {
    finishWithError(conn);
  }

  //get number of fields
  int num_fields = mysql_num_fields(result);

  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result)))
  {
    for(int i = 0; i < num_fields; i++)
    {
      //fprintf(file, "%s, ", row[i] ? row[i] : "NULL");
      printf("%s ", row[i] ? row[i] : "NULL");
    }
      //fprintf(file, "\n");
      printf("\n");
  }

	mysql_close(conn);
    //ptest t[10];

	exit(EXIT_SUCCESS);
}
