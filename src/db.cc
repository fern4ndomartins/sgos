#include "../include/main.h"
#include <sqlite3.h>

sqlite3 *db = nullptr;

bool connect(std::string username_, sqlite3 *&db) {
    std::string db_path = std::format("../{}.db", username_);
    int rc = sqlite3_open(db_path.data(), &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    return true;
}

void execute_query(const std::string &query, sqlite3 *db) {
    char *err_msg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}

void initDatabase(sqlite3 *db) {
    std::string query = R"(PRAGMA foreign_keys = ON;

-- ======================
-- ROLES
-- ======================
CREATE TABLE IF NOT EXISTS roles (
    role_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

INSERT OR IGNORE INTO roles (name) VALUES
('admin'),
('commercial'),
('technician');


-- ======================
-- USERS
-- ======================
CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name TEXT NOT NULL,
    email TEXT UNIQUE,
    username TEXT UNIQUE NOT NULL,
    access_code_hash TEXT NOT NULL,   -- hashed pass
    role_id INTEGER NOT NULL,
    created_at DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (role_id) REFERENCES roles(role_id)
);


-- ======================
-- SERVICE REQUESTS
-- ======================
CREATE TABLE IF NOT EXISTS services (
    service_id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_name TEXT NOT NULL,
    client_phone TEXT,
    client_email TEXT,
    equipment_desc TEXT,              
    problem_report TEXT,
    created_by_id INTEGER NOT NULL,
    status TEXT NOT NULL DEFAULT 'open'
        CHECK(status IN ('open','diagnosing','repair','done','delivered','canceled')),
    created_at DEFAULT CURRENT_TIMESTAMP,
    closed_at,
    FOREIGN KEY (created_by_id) REFERENCES users(user_id)
);


-- ======================
-- SERVICE HISTORY (log of service stage changes or text notes)
-- ======================
CREATE TABLE IF NOT EXISTS service_history (
    history_id INTEGER PRIMARY KEY AUTOINCREMENT,
    service_id INTEGER NOT NULL,
    user_id INTEGER,
    note TEXT,
    status TEXT CHECK(status IN ('open','diagnosing','repair','done','delivered','canceled')),
    created_at DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(service_id) REFERENCES services(service_id),
    FOREIGN KEY(user_id) REFERENCES users(user_id)
);


-- ======================
-- ACTION LOGS (who did what)
-- ======================
CREATE TABLE IF NOT EXISTS action_logs (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    action TEXT NOT NULL,
    details TEXT,
    created_at DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id)
);


-- ======================
-- CHANGE LOGS
-- Stores old → new value per field
-- ======================
CREATE TABLE IF NOT EXISTS change_logs (
    change_id INTEGER PRIMARY KEY AUTOINCREMENT,
    change_type TEXT,
    service_id INTEGER,
    user_id INTEGER,
    field TEXT NOT NULL,
    old_value TEXT,
    new_value TEXT,
    created_at DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(service_id) REFERENCES services(service_id),
    FOREIGN KEY(user_id) REFERENCES users(user_id)
);

CREATE TABLE IF NOT EXISTS service_technicians (
    service_id INTEGER NOT NULL,
    technician_id INTEGER NOT NULL,
    assigned_at DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (service_id, technician_id),
    FOREIGN KEY (service_id) REFERENCES services(service_id) ON DELETE CASCADE,
    FOREIGN KEY (technician_id) REFERENCES users(user_id) ON DELETE CASCADE
);

)";

    execute_query(query, db);
}

bool user_exists(const std::string &username, sqlite3 *db) {
    std::string sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt *stmt;
    bool exists = false;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = true;
    }

    sqlite3_finalize(stmt);
    return exists;
}




bool add_user(const std::string &full_name,
              const std::string &email,
              const std::string &username,
              const std::string &access_code_hash,
              const int &role_id,
              sqlite3 *db)
{
    if (user_exists(username, db)) {
        std::cout << "User already exists: " << username << std::endl;
        return -1;
    }

    const char *sql =
        "INSERT INTO users (full_name, email, username, access_code_hash, role_id) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "prepare failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    sqlite3_bind_text(stmt, 1, full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, access_code_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, role_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error inserting user: " << sqlite3_errmsg(db) << "\endl";
    } else {
        std::cout << "✅ User inserted: " << username << std::endl;
    }

    sqlite3_finalize(stmt);
    return 1;
}

std::optional<UserRow> get_user_by_name(std::string full_name, sqlite3 *db) {
    const char* sql = "SELECT user_id, username, full_name, email, role_id FROM users WHERE full_name = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "get_user_by_full_name prepare failed: " << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, full_name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRow u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt,3) : (const unsigned char*)"");
        u.role_id = sqlite3_column_int(stmt, 4);
        sqlite3_finalize(stmt);
        return u;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<UserRow> get_user_by_id(int user_id, sqlite3 *db) {
    const char* sql = "SELECT user_id, username, full_name, email, role_id FROM users WHERE user_id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "get_user_by_id prepare failed: " << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRow u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt,3) : (const unsigned char*)"");
        u.role_id = sqlite3_column_int(stmt, 4);
        sqlite3_finalize(stmt);
        return u;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

// std::optional<UserRow> get_service_by_id(int user_id, sqlite3 *db) {
//     const char* sql = "SELECT service_id, client, full_name, email, role_id FROM users WHERE user_id = ? LIMIT 1;";
//     sqlite3_stmt *stmt = nullptr;
//     if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
//         std::cerr << "get_user_by_id prepare failed: " << sqlite3_errmsg(db) << "\n";
//         return std::nullopt;
//     }
//     sqlite3_bind_int(stmt, 1, user_id);
//     if (sqlite3_step(stmt) == SQLITE_ROW) {
//         UserRow u;
//         u.user_id = sqlite3_column_int(stmt, 0);
//         u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
//         u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
//         u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt,3) : (const unsigned char*)"");
//         u.role_id = sqlite3_column_int(stmt, 4);
//         sqlite3_finalize(stmt);
//         return u;
//     }
//     sqlite3_finalize(stmt);
//     return std::nullopt;
// }


std::optional<LoginResult> try_login(const std::string& username, const std::string& access_code) {
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT user_id, full_name, role_id "
        "FROM users "
        "WHERE username = ? AND access_code_hash = ? "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL error (prepare): " << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, access_code.c_str(), -1, SQLITE_TRANSIENT);

    LoginResult result;

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        result.user_id  = sqlite3_column_int(stmt, 0);
        result.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.role_id  = sqlite3_column_int(stmt, 2);

        sqlite3_finalize(stmt);
        return result;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}


bool edit_user(int user_id,
               const std::string &full_name,
               const std::string &email,
               const std::string &username,
               int role_id,
               sqlite3 *db) {
    const char *sql = "UPDATE users SET full_name=?, email=?, username=?, role_id=? WHERE user_id=?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "edit_user prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, role_id);
    sqlite3_bind_int(stmt, 5, user_id);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "edit_user step error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool delete_user(int user_id, sqlite3 *db) {
    const char *sql = "DELETE FROM users WHERE user_id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "delete_user prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "delete_user error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<UserRow> get_users(sqlite3 *db) {
    std::vector<UserRow> out;
    const char *sql = "SELECT user_id, username, full_name, email, role_id FROM users ORDER BY username;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "get_users prepare failed: " << sqlite3_errmsg(db) << "\n";
        return out;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRow u;
        u.user_id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt,3) : (const unsigned char*)"");
        u.role_id = sqlite3_column_int(stmt, 4);
        out.push_back(std::move(u));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<ServiceRow> get_services(sqlite3 *db, int only_assigned_to) {
    std::vector<ServiceRow> out;
    std::string sql = "SELECT s.service_id, s.client_name, s.client_phone, s.client_email, s.equipment_desc, s.problem_report, s.status FROM services s";
    if (only_assigned_to > 0) {
        sql += " JOIN service_technicians st ON st.service_id = s.service_id WHERE st.technician_id = ? ";
    }
    sql += " ORDER BY created_at DESC;";
    sqlite3_stmt *stmt = nullptr;
    std::cout << "getting service \n";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "get_services prepare failed: " << sqlite3_errmsg(db) << "\n";
        return out;
    }
    if (only_assigned_to > 0) sqlite3_bind_int(stmt, 1, only_assigned_to);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ServiceRow s;
        s.service_id = sqlite3_column_int(stmt, 0);
        s.client_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        s.phone_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        s.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt,3));
        s.equipment = reinterpret_cast<const char*>(sqlite3_column_text(stmt,4));
        s.problem_report = reinterpret_cast<const char*>(sqlite3_column_text(stmt,5) ? sqlite3_column_text(stmt,5) : (const unsigned char*)"");
        s.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt,6));
        out.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    std::cout <<"got it \n";
    return out;
}

bool add_service(const std::string& client_name,
                 const std::string& client_phone,
                 const std::string& client_email,
                 const std::string& equipment_desc,
                 const std::string& problem_report,
                 int created_by_user_id,
                 sqlite3 *db) {
    const char *sql =
        "INSERT INTO services (client_name, client_phone, client_email, equipment_desc, problem_report, created_by_id) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "add_service prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, client_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, client_phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, client_email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, equipment_desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, problem_report.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, created_by_user_id);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "add_service step error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool edit_service(int service_id,
                  const std::string& client_name,
                  const std::string& phone,
                  const std::string& email,
                  const std::string& equipment,
                  const std::string& problem_report,
                  int technician_id,
                  const std::string& status,
                  sqlite3 *db) {
    const char *sql = "UPDATE services SET client_name=?, client_phone=?, client_email=?, equipment_desc=?, problem_report=?, status=? WHERE service_id=?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "edit_service prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, client_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, equipment.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, problem_report.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, service_id);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "edit_service step error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool delete_service(int service_id, sqlite3 *db) {
    const char *sql = "DELETE FROM services WHERE service_id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "delete_service prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1, service_id);
    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "delete_service error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool add_log(
                const std::string& log_type,
                const int service_id,
                const std::string& client_name,
                 const std::string& client_phone,
                 const std::string& client_email,
                 const std::string& equipment_desc,
                 const std::string& problem_report,
                 int created_by_user_id,
                 sqlite3 *db) {
    const char *sql =
        "INSERT INTO logs (log_type, client_name, client_phone, client_email, equipment_desc, problem_report) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "add_service prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, client_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, client_phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, client_email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, equipment_desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, problem_report.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "add_service step error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool assign_technician(int technician_id, int service_id) {
    const char *sql =
        "INSERT INTO service_technicians (service_id, technician_id) "
        "VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "assign_technician prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1,service_id);
    sqlite3_bind_int(stmt, 2, technician_id);

    bool ok = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "assign_technician step error: " << sqlite3_errmsg(db) << "\n";
        ok = false;
    }
    sqlite3_finalize(stmt);
    return ok;
    }