#ifndef SQL_HPP_INCLUDED
#define SQL_HPP_INCLUDED

#include <memory>
#include <winsock2.h>
#include <mysql.h>
#include <string>

namespace SQL
{
    extern MYSQL Connection;
}

bool SQL_Init();
void SQL_Close();
bool SQL_CheckConnected();

long int SQL_FetchInt(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname);
long long int SQL_FetchInt64(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname);
std::string SQL_FetchString(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname);

std::string SQL_Escape(std::string string);

void SQL_Lock();
void SQL_Unlock();

std::string SQL_Error();

void SQL_DropTables();
void SQL_CreateTables();
void SQL_CreateTableCheckpoint();

void SQL_UpdateTables();

void SQL_UpdateVersion1();

int SQL_Query(std::string query);
int SQL_NumRows(MYSQL_RES* result);
MYSQL_RES* SQL_StoreResult();
void SQL_FreeResult(MYSQL_RES* result);
MYSQL_ROW SQL_FetchRow(MYSQL_RES* result);
int SQL_AffectedRows();
unsigned long* SQL_FetchLengths(MYSQL_RES* result);
unsigned long SQL_NumFields(MYSQL_RES* result);
MYSQL_FIELD* SQL_FetchFields(MYSQL_RES* result);

struct SimpleSQL {
    std::string query;
    bool success;
    MYSQL_RES* result;

    // Execute a query. If it's a SELECT query, stores the result.
    SimpleSQL(std::string query);
    ~SimpleSQL();

    operator bool() const {
        return success;
    }
};

#endif // SQL_HPP_INCLUDED
