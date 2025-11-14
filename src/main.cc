#include <gtkmm.h>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <stack>
#include <vector>
#include "../include/main.h"
#include "gtkmm/alertdialog.h"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "gtkmm/comboboxtext.h"
#include "gtkmm/entry.h"
#include "gtkmm/label.h"
#include "gtkmm/object.h"

static void clear_container(Gtk::Box &box) {
    auto children = box.get_children();
    for (auto &child : children) {
        box.remove(*child);
    }
}

class AdminUserRow : public Gtk::Box {
public:
    AdminUserRow(const UserRow& u, std::function<void(int)> on_edit, std::function<void(int)> on_delete)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 6), user(u)
    {
        get_style_context()->add_class("row");
        
        auto label = Gtk::make_managed<Gtk::Label>(u.username + " — " + u.full_name);
        label->set_halign(Gtk::Align::START);
        label->set_hexpand(true);
        append(*label);
        
        auto edit_btn = Gtk::make_managed<Gtk::Button>("Edit");
        edit_btn->get_style_context()->add_class("primary");
        auto del_btn  = Gtk::make_managed<Gtk::Button>("Delete");
        del_btn->get_style_context()->add_class("danger");
        append(*edit_btn);
        append(*del_btn);

        edit_btn->signal_clicked().connect([on_edit, u] { on_edit(u.user_id); });
        del_btn->signal_clicked().connect([on_delete, u] { on_delete(u.user_id); });
    }
    UserRow user;
};

class ServiceRowWidget : public Gtk::Box {
public:
    ServiceRowWidget(const ServiceRow& s, std::function<void(int)> on_edit, std::function<void(int)> on_delete)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 6), service(s)
    {
        get_style_context()->add_class("row");
        
        auto label = Gtk::make_managed<Gtk::Label>("#" + std::to_string(s.service_id) + "   " + s.client_name + "   " + s.status);
        label->set_halign(Gtk::Align::START);
        label->set_hexpand(true);
        append(*label);
        
        auto assign_btn = Gtk::make_managed<Gtk::Button>("Assign");
        assign_btn->get_style_context()->add_class("primary");
        auto edit_btn = Gtk::make_managed<Gtk::Button>("Edit");
        edit_btn->get_style_context()->add_class("primary");
        auto del_btn  = Gtk::make_managed<Gtk::Button>("Delete");
        del_btn->get_style_context()->add_class("danger");

        

        append(*assign_btn);
        append(*edit_btn);
        append(*del_btn);
        assign_btn->signal_clicked().connect([s, this] { on_assign_technician_clicked(s.service_id); });
        edit_btn->signal_clicked().connect([on_edit, s] { on_edit(s.service_id); });
        del_btn->signal_clicked().connect([on_delete, s] { on_delete(s.service_id); });
    }
    void on_assign_technician_clicked(int service_id) {
        auto win = Gtk::make_managed<Gtk::Window>();
        win->set_title("Assign Technician");
        win->set_modal(true);
        auto box = Gtk::make_managed<Gtk::Box>();
        auto uv = get_users(db);
        auto technicians_box = Gtk::make_managed<Gtk::ComboBoxText>();
        for (auto &u: uv) {
            if (u.role_id == 3) {
                technicians_box->append(u.full_name);
            }
        }
        auto confirm_assign_btn = Gtk::make_managed<Gtk::Button>("Confirm");

        box->append(*technicians_box);
        box->append(*confirm_assign_btn);

        win->set_child(*box);

        confirm_assign_btn->signal_clicked().connect([technicians_box, service_id, win](){
            auto tech = get_user_by_name(technicians_box->get_active_text(), db);
            assign_technician(tech->user_id, service_id);
            std::cout << "assigned.\n";
            win->close();
        });

        win->show();        
    }

    ServiceRow service;
};

class ServiceHistoryRowWidget : public Gtk::Box {
public:
    ServiceHistoryRowWidget(const ServiceRow& s, std::function<void(int)> on_edit)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 6), service(s)
    {
        get_style_context()->add_class("row");
        
        auto label = Gtk::make_managed<Gtk::Label>("#" + std::to_string(s.service_id) + "   " + s.client_name + "   " + s.status);
        label->set_halign(Gtk::Align::START);
        label->set_hexpand(true);
        append(*label);
        
        auto edit_btn = Gtk::make_managed<Gtk::Button>("Edit");
        edit_btn->get_style_context()->add_class("primary");
        append(*edit_btn);

        edit_btn->signal_clicked().connect([on_edit, s] { on_edit(s.service_id); });
    }
    ServiceRow service;
};

class ChangeLogWidget : public Gtk::Box {
public:

    ChangeLogWidget(const LogRow& l, std::function<void(int)> on_edit)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 6), log(l)
    {
        get_style_context()->add_class("row");
        
        auto label = Gtk::make_managed<Gtk::Label>("#" + std::to_string(l.service_id) + "   " + l.new_value + "   " + l.old_value);
        label->set_halign(Gtk::Align::START);
        label->set_hexpand(true);
        append(*label);
        
        auto info_btn = Gtk::make_managed<Gtk::Button>("Info");
        info_btn->get_style_context()->add_class("primary");
        append(*info_btn);

        info_btn->signal_clicked().connect([this, l]() {on_log_info_clicked(l.change_id);});
    }

    void on_log_info_clicked(int change_id) {
        auto uopt = get_user_by_id(change_id, db);
        if (!uopt) return;
        
        auto win = Gtk::make_managed<Gtk::Window>();
        win->set_title("Info");
        win->set_modal(true);
        auto box = Gtk::make_managed<Gtk::Box>();
        win->set_child(*box);

        win->show();
    }

    LogRow log;

};



class MyWindow : public Gtk::Window {
public:
    MyWindow(sqlite3 *db);
protected:
    void navigate_to(const std::string& page_name);
    void on_return_clicked();
    void update_return_button_visibility();

    void on_login_clicked();
    void show_admin_users();
    void on_add_user_clicked();
    void on_edit_user(int user_id);
    void on_delete_user(int user_id);

    void show_admin_services();
    void on_add_service_clicked();
    void on_edit_service(int service_id);
    void on_delete_service(int service_id);

    void show_history_services();
    void on_history_service_clicked(int service_id);

    Gtk::Stack stack;

    Gtk::Box login_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Label login_title{"Login"};
    Gtk::Entry login_user;
    Gtk::Entry login_pass;
    Gtk::Button login_btn{"Login"};
    Gtk::Label login_msg{""};

    Gtk::Box admin_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Label admin_title{"Admin Dashboard"};
    Gtk::Button admin_users_btn{"Users"};
    Gtk::Button admin_services_btn{"Services"};
    Gtk::Button admin_history_btn{"History"};

    
    Gtk::Box admin_users_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Label admin_users_title{"Users"};
    Gtk::ScrolledWindow admin_users_scrolled;

    Gtk::Box admin_services_box{Gtk::Orientation::VERTICAL, 20};
    Gtk::Box admin_services_list_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box admin_services_box_title{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Box admin_services_box_subtitle{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Label admin_services_title{"Services"};
    Gtk::ScrolledWindow admin_services_scrolled; 

    Gtk::Box admin_history_box{Gtk::Orientation::VERTICAL, 20};
    Gtk::Box admin_history_list_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box admin_history_box_title{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Box admin_history_box_subtitle{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Label admin_history_title{"History"};
    Gtk::ScrolledWindow admin_history_scrolled; 

    Gtk::Box technician_services_box{Gtk::Orientation::VERTICAL, 20};
    Gtk::Box technician_services_list_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box technician_services_box_title{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Box technician_services_box_subtitle{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Label technician_services_title{"services"};
    Gtk::ScrolledWindow technician_services_scrolled; 

    Gtk::Button return_button{"Return"};

    sqlite3 *db;
    int logged_in_user_id = 0;
    
    std::stack<std::string> navigation_stack;
    std::string current_page;
};

MyWindow::MyWindow(sqlite3 *db_) : db(db_) {
    set_default_size(700, 480);
    set_title("Service Desk");
    maximize();
    set_decorated(true);

    admin_users_scrolled.set_child(admin_users_box);
    admin_users_scrolled.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    admin_users_scrolled.set_propagate_natural_height(true);

    admin_services_scrolled.set_child(admin_services_box);
    admin_services_scrolled.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    admin_services_scrolled.set_propagate_natural_height(true);

    admin_history_scrolled.set_child(admin_history_box);
    admin_history_scrolled.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    admin_history_scrolled.set_propagate_natural_height(true);

    technician_services_scrolled.set_child(technician_services_box);
    technician_services_scrolled.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    technician_services_scrolled.set_propagate_natural_height(true);

    stack.add(login_box, "login");
    stack.add(admin_box, "admin_users");
    stack.add(admin_users_scrolled, "admin_users_list");      
    stack.add(admin_services_scrolled, "admin_services_list");
    stack.add(admin_history_scrolled, "admin_history_list");
    stack.add(technician_services_scrolled, "technician_services_list");

    set_child(stack);


    login_box.set_margin(20);
    login_box.get_style_context()->add_class("card");
    
    login_title.get_style_context()->add_class("title");
    login_user.set_placeholder_text("username");
    login_pass.set_placeholder_text("access code");
    login_pass.set_visibility(false);
    
    login_btn.get_style_context()->add_class("primary");
    login_msg.get_style_context()->add_class("dialog-error");
    
    login_box.append(login_title);
    login_box.append(login_user);
    login_box.append(login_pass);
    login_box.append(login_btn);
    login_box.append(login_msg);
    login_btn.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_login_clicked));

    admin_box.set_margin(20);
    admin_box.get_style_context()->add_class("card");
    
    admin_title.get_style_context()->add_class("title");
    admin_users_btn.get_style_context()->add_class("primary");
    admin_services_btn.get_style_context()->add_class("primary");
    admin_history_btn.get_style_context()->add_class("primary");
    
    admin_box.append(admin_title);
    admin_box.append(admin_users_btn);
    admin_box.append(admin_services_btn);
    admin_box.append(admin_history_btn);
    admin_users_btn.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::show_admin_users));
    admin_services_btn.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::show_admin_services));
    admin_history_btn.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::show_history_services));

    admin_users_box.set_margin(12);
    admin_users_title.get_style_context()->add_class("section-header");
    admin_users_box.append(admin_users_title);
    
    auto add_user_btn = Gtk::make_managed<Gtk::Button>("Add user");
    add_user_btn->get_style_context()->add_class("success");
    admin_users_box.append(*add_user_btn);
    add_user_btn->signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_add_user_clicked));
    
    return_button.get_style_context()->add_class("flat");
    admin_users_box.append(return_button);
    return_button.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_return_clicked));

    admin_services_box.set_margin(12);
    admin_services_title.get_style_context()->add_class("section-header");
    admin_services_box_title.get_style_context()->add_class("admin-service-box-title");
    admin_services_box_subtitle.get_style_context()->add_class("admin-service-box-subtitle");

      // admin_services_list_box.get_style_context()->add_class("admin-service-box-subtitle");
    admin_services_title.set_halign(Gtk::Align::START);
    admin_services_title.set_hexpand(true);

    admin_services_box_title.append(admin_services_title);
    auto add_service_btn = Gtk::make_managed<Gtk::Button>("Add Service >");
    add_service_btn->set_halign(Gtk::Align::END);

    admin_services_box_title.append(*add_service_btn);

    admin_services_box.append(admin_services_box_title);
    admin_services_box.append(admin_services_box_subtitle);
    

    add_service_btn->get_style_context()->add_class("success");
    add_service_btn->signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_add_service_clicked));


    admin_history_box.set_margin(12);
    admin_history_title.get_style_context()->add_class("section-header");
    admin_history_box_title.get_style_context()->add_class("admin-service-box-title");
    admin_history_box_subtitle.get_style_context()->add_class("admin-service-box-subtitle");
    admin_history_title.set_halign(Gtk::Align::START);
    admin_history_title.set_hexpand(true);

    technician_services_box.set_margin(12);
    technician_services_title.get_style_context()->add_class("section-header");
    technician_services_box_title.get_style_context()->add_class("admin-service-box-title");
    technician_services_box_subtitle.get_style_context()->add_class("admin-service-box-subtitle");
    technician_services_title.set_halign(Gtk::Align::START);
    technician_services_title.set_hexpand(true);


    stack.set_visible_child("login");
    current_page = "login";
    update_return_button_visibility();
}

void MyWindow::navigate_to(const std::string& page_name) {
    if (current_page != "login") {
        navigation_stack.push(current_page);
    }
    current_page = page_name;
    stack.set_visible_child(page_name);
    update_return_button_visibility();
}

void MyWindow::on_return_clicked() {
    if (!navigation_stack.empty()) {
        std::string previous_page = navigation_stack.top();
        navigation_stack.pop();
        current_page = previous_page;
        stack.set_visible_child(previous_page);
        update_return_button_visibility();
    }
}

void MyWindow::update_return_button_visibility() {
    bool should_show = (current_page != "login" && current_page != "admin_users");
    
    if (return_button.get_parent()) {
        auto old_parent = return_button.get_parent();
        if (old_parent) {
            dynamic_cast<Gtk::Box*>(old_parent)->remove(return_button);
        }
    }

    if (should_show && !return_button.get_parent()) {
        auto us = get_user_by_id(logged_in_user_id,  db);
        if (us->role_id != 1) return;
        if (current_page == "admin_users_list") {
            admin_users_box.append(return_button);
        } else if (current_page == "admin_services_list") {
            admin_services_box.append(return_button);
        } else if (current_page == "admin_history_list") {
            admin_history_box.append(return_button);
        }
    }
    return_button.set_visible(should_show);
}

void MyWindow::on_login_clicked() {
    auto user = login_user.get_text();
    auto pass = login_pass.get_text();
    login_msg.set_text("");
    auto uid_opt = try_login(user, pass);
    if (!uid_opt) {
        login_msg.set_text("Invalid credentials");
        return;
    }
    logged_in_user_id = uid_opt->user_id;

    while (!navigation_stack.empty()) {
        navigation_stack.pop();
    }

    if (uid_opt->role_id == 1) {
        current_page = "admin_users";
        stack.set_visible_child("admin_users");
        update_return_button_visibility();
    } else if (uid_opt->role_id == 2) {
        show_admin_services();
    } else {
        std::vector<ServiceRow> sv = get_services(db, logged_in_user_id);
        clear_container(technician_services_box);
        
        technician_services_box.append(technician_services_box_title);
        
        for (auto &s: sv) {
            auto w = Gtk::make_managed<ServiceRowWidget>(s,
                [this](int id){ on_edit_service(id); },
                [this](int id){ on_delete_service(id); });
            technician_services_box.append(*w);
        }
        navigate_to("technician_services_list");
    }
}

void MyWindow::show_admin_users() {
    clear_container(admin_users_box);
    admin_users_box.append(admin_users_title);
    
    auto add_user_btn = Gtk::make_managed<Gtk::Button>("Add user");
    add_user_btn->get_style_context()->add_class("success");
    admin_users_box.append(*add_user_btn);
    add_user_btn->signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_add_user_clicked));

    auto users = get_users(db);
    for (auto &u: users) {
        auto row = Gtk::make_managed<AdminUserRow>(u,
            [this](int id){ on_edit_user(id); },
            [this](int id){ on_delete_user(id); });
        admin_users_box.append(*row);
    }
    
    navigate_to("admin_users_list");
}

void MyWindow::on_add_user_clicked() {
    auto win = Gtk::make_managed<Gtk::Window>();
    win->set_title("Add user");
    win->set_modal(true);
    win->set_transient_for(*this);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    box->set_margin(20);
    box->get_style_context()->add_class("card");
    win->set_child(*box);

    auto e_full = Gtk::make_managed<Gtk::Entry>();
    auto e_email = Gtk::make_managed<Gtk::Entry>();
    auto e_user = Gtk::make_managed<Gtk::Entry>();
    auto e_pass = Gtk::make_managed<Gtk::Entry>();
    auto role_combo = Gtk::make_managed<Gtk::ComboBoxText>();
    
    role_combo->append("admin"); 
    role_combo->append("commercial"); 
    role_combo->append("technician");
    role_combo->set_active(2);

    e_user->set_placeholder_text("Username");
    e_full->set_placeholder_text("Full name");
    e_email->set_placeholder_text("Email");
    e_pass->set_placeholder_text("Access code");

    box->append(*e_full);
    box->append(*e_email);
    box->append(*e_user);
    box->append(*e_pass);
    box->append(*role_combo);

    auto btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    btn_box->set_halign(Gtk::Align::END);
    btn_box->set_margin_top(10);
    
    auto add_btn = Gtk::make_managed<Gtk::Button>("Add");
    add_btn->get_style_context()->add_class("success");
    auto cancel_btn = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel_btn->get_style_context()->add_class("flat");
    
    btn_box->append(*cancel_btn);
    btn_box->append(*add_btn);
    box->append(*btn_box);

    add_btn->signal_clicked().connect([this, e_full, e_email, e_user, e_pass, role_combo, win]() {
        std::string full = e_full->get_text();
        std::string email = e_email->get_text();
        std::string username = e_user->get_text();
        std::string pass = e_pass->get_text();
        int role = role_combo->get_active_row_number() + 1;
        std::cout << "adding user\n";
        if (add_user(full, email, username, pass, role, db)) {
            std::cout << "added user\n";
            show_admin_users();
            win->hide();
        } else {
            std::cout << "failed user\n";
            Gtk::MessageDialog err(*this, "Failed to add user", false, Gtk::MessageType::ERROR);
            err.set_modal(true);
            err.show();
        }
    });
    
    cancel_btn->signal_clicked().connect([win]() { win->hide(); });

    win->show();
}

void MyWindow::on_edit_user(int user_id) {
    auto uopt = get_user_by_id(user_id, db);
    if (!uopt) return;
    
    auto win = Gtk::make_managed<Gtk::Window>();
    win->set_title("Edit user");
    win->set_modal(true);
    win->set_transient_for(*this);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    box->set_margin(20);
    box->get_style_context()->add_class("card");
    win->set_child(*box);

    auto e_full = Gtk::make_managed<Gtk::Entry>();
    auto e_email = Gtk::make_managed<Gtk::Entry>();
    auto e_user = Gtk::make_managed<Gtk::Entry>();
    auto role_combo = Gtk::make_managed<Gtk::ComboBoxText>();
    
    role_combo->append("admin"); 
    role_combo->append("commercial"); 
    role_combo->append("technician");
    role_combo->set_active(uopt->role_id - 1);

    e_user->set_text(uopt->username);
    e_full->set_text(uopt->full_name);
    e_email->set_text(uopt->email);

    box->append(*e_full);
    box->append(*e_email);
    box->append(*e_user);
    box->append(*role_combo);

    auto btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    btn_box->set_halign(Gtk::Align::END);
    btn_box->set_margin_top(10);
    
    auto save_btn = Gtk::make_managed<Gtk::Button>("Save");
    save_btn->get_style_context()->add_class("primary");
    auto cancel_btn = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel_btn->get_style_context()->add_class("flat");
    
    btn_box->append(*cancel_btn);
    btn_box->append(*save_btn);
    box->append(*btn_box);
    int id = uopt->user_id;

    save_btn->signal_clicked().connect([this, id, e_full, e_email, e_user, role_combo, win]() {
        std::string full = e_full->get_text();
        std::string email = e_email->get_text();
        std::string username = e_user->get_text();
        int role = role_combo->get_active_row_number() + 1;
        if (edit_user(id, full, email, username, role, db)) {
            std::cout << "edited user\n";
            show_admin_users();
            win->hide();
        } else {
            std::cout << "failed to edit user\n";
            Gtk::MessageDialog err(*this, "Failed to edit user", false, Gtk::MessageType::ERROR);
            err.set_modal(true);
            err.show();
        }
    });
    
    cancel_btn->signal_clicked().connect([win]() { win->hide(); });

    win->show();
}

void MyWindow::on_delete_user(int user_id) {
    auto confirm_dialog = Gtk::AlertDialog::create("Delete user? This cannot be undone");
    confirm_dialog->set_buttons({ "Cancel", "Delete" });

    confirm_dialog->choose(*this, [this, user_id, confirm_dialog](Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            int response = confirm_dialog->choose_finish(result);
            if (response == 1) {
                if (!delete_user(user_id, db)) {
                    auto error_dialog = Gtk::AlertDialog::create("Failed to delete user");
                    error_dialog->set_buttons({ "OK" });
                    error_dialog->show(*this);
                } else {
                    show_admin_users();
                }
            }
        } catch (const Glib::Error& e) {
        }
    });
}

void MyWindow::show_admin_services() {
    clear_container(admin_services_box);
    clear_container(admin_services_box_title);
    clear_container(admin_services_box_subtitle);
    clear_container(admin_services_list_box);

    auto filter_status = Gtk::make_managed<Gtk::ComboBoxText>();

    auto services = std::make_shared<std::vector<ServiceRow>>();
    
    
    filter_status->append("all"); 
    filter_status->append("open"); 
    filter_status->append("closed"); 
    filter_status->append("waiting technician");
    filter_status->set_active(0);
    auto filter_label = Gtk::make_managed<Gtk::Label>("Filter by: ");
    auto filter_entry = Gtk::make_managed<Gtk::Entry>();
    

    admin_services_box_subtitle.append(*filter_label);
    admin_services_box_subtitle.append(*filter_entry);
    admin_services_box_subtitle.append(*filter_status);

    admin_services_title.set_halign(Gtk::Align::START);
    admin_services_title.set_hexpand(true);
    admin_services_box_title.append(admin_services_title);

    auto add_service_btn = Gtk::make_managed<Gtk::Button>("Add Service >");
    add_service_btn->set_halign(Gtk::Align::END);

    admin_services_box_title.append(*add_service_btn);

    admin_services_box.append(admin_services_box_title);
    admin_services_box.append(admin_services_box_subtitle);
    
    add_service_btn->get_style_context()->add_class("success");
    add_service_btn->signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_add_service_clicked));

    *services = get_services(db, 0);
    for (auto &s : *services) {
        if (s.status == filter_status->get_active_text() || filter_status->get_active_text() == "all")
        {
            auto w = Gtk::make_managed<ServiceRowWidget>(s,
            [this](int id){ on_edit_service(id); },
            [this](int id){ on_delete_service(id); });
            admin_services_list_box.append(*w);
        } 
    }
    
    admin_services_box.append(admin_services_list_box);
    
    admin_services_box.append(return_button);
    
    filter_entry->signal_changed().connect([this, filter_entry, services, filter_status]() {
        clear_container(admin_services_list_box);
                
        for (auto &s : *services) {
            std::regex pattern("(" + std::string(filter_entry->get_text()) + ")(.*)");
            if (std::regex_match(s.client_name, pattern) && (filter_status->get_active_text() == s.status || filter_status->get_active_text() == "all")) {
                auto w = Gtk::make_managed<ServiceRowWidget>(s,
                [this](int id){ on_edit_service(id); },
                [this](int id){ on_delete_service(id); });
                admin_services_list_box.append(*w);

                
            }
        }
    });
    filter_status->signal_changed().connect([this, filter_entry, services, filter_status]() {
        clear_container(admin_services_list_box);
                
        for (auto &s : *services) {
            std::regex pattern("(" + std::string(filter_entry->get_text()) + ")(.*)");
            if (std::regex_match(s.client_name, pattern) && (filter_status->get_active_text() == s.status || filter_status->get_active_text() == "all")) {
                auto w = Gtk::make_managed<ServiceRowWidget>(s,
                [this](int id){ on_edit_service(id); },
                [this](int id){ on_delete_service(id); });
                admin_services_list_box.append(*w);

                
            }
        }
    });

    navigate_to("admin_services_list");
}

void MyWindow::show_history_services() {
    clear_container(admin_history_box);
    clear_container(admin_history_box_title);
    clear_container(admin_history_box_subtitle);
    clear_container(admin_history_list_box);


    auto services = std::make_shared<std::vector<ServiceRow>>();
    
    auto filter_label = Gtk::make_managed<Gtk::Label>("Filter by: ");
    auto filter_entry = Gtk::make_managed<Gtk::Entry>();

    admin_history_box_subtitle.append(*filter_label);
    admin_history_box_subtitle.append(*filter_entry);

    admin_history_title.set_halign(Gtk::Align::START);
    admin_history_title.set_hexpand(true);
    admin_history_box_title.append(admin_history_title);

    admin_history_box.append(admin_history_box_title);
    admin_history_box.append(admin_history_box_subtitle);
    

    *services = get_services(db, 0);
    for (auto &s : *services) {
        auto w = Gtk::make_managed<ServiceHistoryRowWidget>(s,
        [this](int id){ on_edit_service(id); });
        admin_history_list_box.append(*w);
         
    }
    
    admin_history_box.append(admin_history_list_box);
    
    admin_history_box.append(return_button);
    
    filter_entry->signal_changed().connect([this, filter_entry, services]() {
        clear_container(admin_history_list_box);
                
        for (auto &s : *services) {
            std::regex pattern("(" + std::string(filter_entry->get_text()) + ")(.*)");
            if (std::regex_match(s.client_name, pattern)) {
                auto w = Gtk::make_managed<ServiceHistoryRowWidget>(s,
                [this](int id){ on_edit_service(id); });
                admin_history_list_box.append(*w);

                
            }
        }
    });
    update_return_button_visibility();
    navigate_to("admin_history_list");
  
}

void MyWindow::on_add_service_clicked() {
    auto win = Gtk::make_managed<Gtk::Window>();
    win->set_title("Add Service");
    win->set_modal(true);
    win->set_transient_for(*this);

    win->set_default_size(500, 400); 

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    box->set_margin(20);
    box->get_style_context()->add_class("card");
    win->set_child(*box);

    auto e_client = Gtk::make_managed<Gtk::Entry>();
    auto e_phone = Gtk::make_managed<Gtk::Entry>();
    auto e_email = Gtk::make_managed<Gtk::Entry>();
    auto e_equip = Gtk::make_managed<Gtk::Entry>();
    auto e_problem = Gtk::make_managed<Gtk::Entry>();
    
    e_client->set_placeholder_text("Client name");
    e_phone->set_placeholder_text("Phone number");
    e_email->set_placeholder_text("Email");
    e_equip->set_placeholder_text("Equipment description");
    e_problem->set_placeholder_text("Problem description");
    
    box->append(*e_client);
    box->append(*e_phone);
    box->append(*e_email);
    box->append(*e_equip);
    box->append(*e_problem);

    auto btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    btn_box->set_halign(Gtk::Align::END);
    btn_box->set_margin_top(10);
    
    auto add_btn = Gtk::make_managed<Gtk::Button>("Add");
    add_btn->get_style_context()->add_class("success");
    auto cancel_btn = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel_btn->get_style_context()->add_class("flat");
    
    btn_box->append(*cancel_btn);
    btn_box->append(*add_btn);
    box->append(*btn_box);

    add_btn->signal_clicked().connect([this, e_client, e_phone, e_email, e_equip, e_problem, win]() {
        if (add_service(e_client->get_text(), e_phone->get_text(), e_email->get_text(), e_equip->get_text(), e_problem->get_text(), logged_in_user_id, db)) {
            show_admin_services();
            win->hide();
        } else {
            Gtk::MessageDialog err(*this, "Failed to add service", false, Gtk::MessageType::ERROR);
            err.set_modal(true);
            err.show();
        }
    });

    cancel_btn->signal_clicked().connect([win]() { win->hide(); });

    win->show();
}

void MyWindow::on_edit_service(int service_id) {
    auto services = get_services(db, 0);
    ServiceRow srow;
    bool found = false;
    for (auto &s : services) {
        if (s.service_id == service_id) { srow = s; found = true; break; }
    }
    if (!found) return;

    auto win = Gtk::make_managed<Gtk::Window>();
    win->set_title("Edit Service");
    win->set_modal(true);
    win->set_transient_for(*this);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    box->set_margin(20);
    box->get_style_context()->add_class("card");
    win->set_child(*box);

    auto e_client = Gtk::make_managed<Gtk::Entry>();
    auto e_phone = Gtk::make_managed<Gtk::Entry>();
    auto e_email = Gtk::make_managed<Gtk::Entry>();
    auto e_equipment = Gtk::make_managed<Gtk::Entry>();
    auto e_problem = Gtk::make_managed<Gtk::Entry>();
    auto e_status = Gtk::make_managed<Gtk::ComboBoxText>();

    e_client->set_text(srow.client_name);
    e_phone->set_text(srow.phone_number);
    e_email->set_text(srow.email);
    e_equipment->set_text(srow.equipment);
    e_problem->set_text(srow.problem_report);
    e_status->append("open");
    e_status->append("diagnosing");
    e_status->append("repair");
    e_status->append("done");
    e_status->append("delivered");
    e_status->append("canceled");
    e_status->set_active_text(srow.status);
    box->append(*e_client);
    box->append(*e_phone);
    box->append(*e_email);
    box->append(*e_equipment);
    box->append(*e_problem);
    box->append(*e_status);

    auto btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    btn_box->set_halign(Gtk::Align::END);
    btn_box->set_margin_top(10);
    
    auto save_btn = Gtk::make_managed<Gtk::Button>("Save");
    save_btn->get_style_context()->add_class("primary");
    auto cancel_btn = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel_btn->get_style_context()->add_class("flat");
    
    btn_box->append(*cancel_btn);
    btn_box->append(*save_btn);
    box->append(*btn_box);
    int id = srow.service_id;
    
    save_btn->signal_clicked().connect([this, id, srow, e_client, e_phone, e_email, e_equipment, e_problem, e_status, win]() {
        std::string client = e_client->get_text();
        std::string phone = e_phone->get_text();
        std::string email = e_email->get_text();
        std::string equipment = e_equipment->get_text();
        std::string problem = e_problem->get_text();
        std::string status = e_status->get_active_text();
        
        if (edit_service(id, client, phone, email, equipment, problem, srow.created_by_id, status, db)) {
            std::cout << "edited service\n";
            show_admin_services();
            win->hide();
        } else {
            std::cout << "failed to edit service\n";
            Gtk::MessageDialog err(*this, "Failed to edit service", false, Gtk::MessageType::ERROR);
            err.set_modal(true);
            err.show();
        }
    });
    
    cancel_btn->signal_clicked().connect([win]() { win->hide(); });

    win->show();
}

void MyWindow::on_delete_service(int service_id) {
    auto dialog = std::make_shared<Gtk::MessageDialog>(
        *this,
        "Delete service? This cannot be undone",
        false,
        Gtk::MessageType::QUESTION,
        Gtk::ButtonsType::OK_CANCEL
    );

    dialog->set_modal(true);

    dialog->signal_response().connect(
        [this, service_id, dialog](int response_id) {
            if (response_id == Gtk::ResponseType::OK) {
                if (!delete_service(service_id, db)) {
                    auto err = std::make_shared<Gtk::MessageDialog>(
                        *this,
                        "Failed to delete service",
                        false,
                        Gtk::MessageType::ERROR,
                        Gtk::ButtonsType::OK
                    );
                    err->set_modal(true);
                    err->signal_response().connect([err](int) {});
                    err->show();
                } else {
                    show_admin_services();
                }
            }
        }
    );

    dialog->show();
}

int main(int argc, char* argv[])
{
    g_setenv("GTK_CSD", "0", TRUE);
    auto app = Gtk::Application::create("org.gtkmm.login");
    
    auto provider = Gtk::CssProvider::create();
    provider->load_from_path("../style/main.css");
    Gtk::StyleContext::add_provider_for_display(
        Gdk::Display::get_default(),
        provider,
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    int err = connect("test", db);

    initDatabase(db);
    add_user("admin", "admin", "admin", "1111", 1, db);
    return app->make_window_and_run<MyWindow>(argc, argv, db);
}