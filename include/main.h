#pragma once
#include <gtk/gtk.h>
#include <gtk-4.0/gtk/gtkwindow.h>
#include <gtk-4.0/gtk/gtkbox.h>
#include <gtk/gtkbox.h>
#include <gobject/gsignal.h>
#include <gtk/gtkentry.h>
#include <gtk-4.0/gtk/gtkeditable.h>
#include <gtk-4.0/gtk/gtkentry.h>
#include <gtk-4.0/gtk/gtkscrolledwindow.h>

#include <regex>
#include <string>

#include <sqlite3.h>

#include <iostream>
#include <time.h>
#include <string>
#include <format>
#include <vector>

extern sqlite3* db;

struct LoginResult {
    int user_id;
    std::string full_name;
    int role_id;
};

struct MessageObject {
    int id;
    std::string sender;
    std::string receiver;
    std::string message;
    time_t timestamp;
};

struct UserRow {
    int user_id;
    std::string username;
    std::string full_name;
    std::string email;
    int role_id;
};

struct ServiceRow {
    int service_id;
    std::string client_name;
    std::string phone_number;
    std::string email;
    std::string equipment;
    std::string problem_report;
    int created_by_id; 
    std::string status;
};

struct LogRow {
    int change_id;
    int service_id;
    int user_id;
    std::string field;
    std::string old_value;
    std::string new_value;
    std::string created_at;
};

bool connect(std::string username_, sqlite3 *&db);

void execute_query(const std::string &query, sqlite3 *db);

void initDatabase(sqlite3 *db);

bool user_exists(const std::string &username, sqlite3 *db);
    
bool add_user(const std::string &full_name,
    const std::string &email,
    const std::string &username,
    const std::string &access_code_hash,
    const int &role_id, sqlite3 *db);

std::optional<UserRow> get_user_by_id(int user_id, sqlite3 *db);
std::optional<LoginResult> try_login(const std::string& username, const std::string& access_code);
bool edit_user(int user_id,
               const std::string &full_name,
               const std::string &email,
               const std::string &username,
               int role_id,
               sqlite3 *db);
bool delete_user(int user_id, sqlite3 *db);
std::vector<UserRow> get_users(sqlite3 *db);
std::vector<ServiceRow> get_services(sqlite3 *db, int only_assigned_to);
bool add_service(const std::string& client_name,
                 const std::string& client_phone,
                 const std::string& client_email,
                 const std::string& equipment_desc,
                 const std::string& problem_report,
                 int created_by_user_id,
                 sqlite3 *db);
bool edit_service(int service_id,
                  const std::string& client_name,
                  const std::string& phone,
                  const std::string& email,
                  const std::string& equipment,
                  const std::string& problem_report,
                  int technician_id,
                  const std::string& status,
                  sqlite3 *db);
bool delete_service(int service_id, sqlite3 *db);


bool add_log(
                const std::string& log_type,
                const int service_id,
                const std::string& client_name,
                 const std::string& client_phone,
                 const std::string& client_email,
                 const std::string& equipment_desc,
                 const std::string& problem_report,
                 int created_by_user_id,
                 sqlite3 *db);

bool assign_technician(int technician_id, int service_id);

// std::optional<UserRow> get_service_by_id(int user_id, sqlite3 *db);
std::optional<UserRow> get_user_by_name(std::string full_name, sqlite3 *db);

