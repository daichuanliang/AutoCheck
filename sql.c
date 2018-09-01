#include "sql.h" 
 
void connectDatabase(MYSQL *conn, const char* host, const char* user, const char* password, const char* database) {
	//mysql_init(conn);
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
 

void insertDatabase(MYSQL *conn, const char *sql) {
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
 
void deleteDatabase(MYSQL *conn, const char *sql) {
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

void queryDatabase(MYSQL *conn, const char *sql)
{
    if (mysql_query(conn, sql))
    {
        finishWithError(conn);
    }
}

#if 0
typedef struct ip_mac{
    int id;
    char mac[20];
}_ip_mac;
struct ip_mac ipMac[5];
int main (int argc, char *argv[]) {
 
    MYSQL *conn;
    conn = mysql_init(NULL);
	connectDatabase(conn, "localhost", "root", "cldai-gpu123--", "shopdb");
    const char *sql = "select ad_id, mac from products";

    queryDatabase(conn, sql);
    //store result
  MYSQL_RES *result = mysql_store_result(conn);
  if(result == NULL)
  {
    finishWithError(conn);
  }

  //get number of fields
  int num_fields = mysql_num_fields(result);
  printf("cldai test num_fields: %d\n", num_fields);
  MYSQL_ROW row;
  
  int i=0;
  while ((row = mysql_fetch_row(result)))
  {
    #if 0
    for(int i = 0; i < num_fields; i++)
    {
      //fprintf(file, "%s, ", row[i] ? row[i] : "NULL");
     // printf("%s ", row[i] ? row[i] : "NULL");
    //printf("id:%s\n", row[0]);
    //printf("mac:%s\n", row[1]);
    }
      //fprintf(file, "\n");
      //printf("\n");
    #endif
#if 1
    printf("i:%d\n", i);
    ipMac[i].id = atoi(row[0]);
    memset(ipMac[i].mac, 0, 20);
    strcpy(ipMac[i].mac, row[1]);
    i++;
#endif
  }
    for(int i=0; i<5; i++)
    {
        printf("id[%d]:%d\n", i, ipMac[i].id);
        printf("mac[%d]:%s\n", i, ipMac[i].mac);
    }
	mysql_close(conn);
    //ptest t[10];

	exit(EXIT_SUCCESS);
}
#endif
