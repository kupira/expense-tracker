#include <iostream>
#include <rapidjson/rapidjson.h>
#include <string>
#include <map>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace rjson = rapidjson;

std::string dateToday() {
    time_t t = time(nullptr);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return buf;
}

rjson::Document loadExpenses(const std::string& filename) {
    std::ifstream in(filename);
    rjson::Document doc;

    if (in.is_open()) {
        std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!str.empty()) {
            doc.Parse(str.c_str());
            if (doc.HasParseError() || !doc.IsArray()) {
                std::cerr << "Warning: corrupt or invalid JSON, starting new array\n";
                doc.SetArray();
            }
        } else {
            doc.SetArray(); // file is empty
        }
    } else {
        doc.SetArray(); // file does not exist 
    }

    return doc;
}

void saveExpenses(const std::string& filename, rjson::Document& doc) {
    rjson::StringBuffer buffer;
    rjson::PrettyWriter<rjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream out(filename);
    out << buffer.GetString();
}

std::map<std::string, std::string> parseArguments(int argc, char* argv[]) {
    std::map<std::string, std::string> opts;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string key = arg.substr(2);
            // if next token exists and does not begin from '-'
            if (i + 1 < argc && argv[i+1][0] != '-')
                opts[key] = argv[++i];
            else
                opts[key] = "none"; 
        }
    }
    return opts;
}

void cmdAdd(rjson::Document& doc, std::map<std::string, std::string>& opts) {
    if (!opts.count("description") || !opts.count("amount")) {
        std::cerr << "Need --description and --amount\n";
        return;
    }

    rjson::Document::AllocatorType& allocator = doc.GetAllocator();

    std::string desc = opts.at("description");
    double amount = std::stod(opts.at("amount"));
    int id = doc.Size() ? doc[doc.Size() - 1]["id"].GetInt() + 1 : 1;

    rjson::Value expense(rapidjson::kObjectType);
    expense.AddMember("id", id, allocator);
    expense.AddMember("date", rjson::Value(dateToday().c_str(), allocator), allocator);
    expense.AddMember("description", rjson::Value(desc.c_str(), allocator), allocator);
    expense.AddMember("amount", amount, allocator);

    doc.PushBack(expense, allocator);

    std::cout << "Expense added succesfully (ID: " << id << ")\n";
}

void cmdList(rjson::Document& doc) {
    std::cout << "ID  Date        Description  Amount\n"
                 "-----------------------------------\n";

    for (rjson::SizeType i = 0; i < doc.Size(); i++) {
        auto& e = doc[i]; // e for expense
        std::cout 
            << e["id"].GetInt() << "   "
            << e["date"].GetString() << "  "
            << e["description"].GetString() << "     "
            << e["amount"].GetDouble() << "kr\n";
    }
}

void cmdSummary(rjson::Document& doc, std::map<std::string, std::string>& opts) {
    double sum = 0;
    
    if (!opts.count("month")) {
        for (rjson::SizeType i = 0; i < doc.Size(); i++) {
            sum += doc[i]["amount"].GetDouble();
        }
    }
    std::cout << "Total expenses: " << sum << "kr\n";
}

void cmdDelete(rjson::Document& doc, std::map<std::string, std::string>& opts) {
    if (!opts.count("id")) {
        std::cerr << "Need --id\n";
        return;
    }
    if (opts.at("id") == "none") {
        std::cerr << "Provide correct ID\n";
        return;
    }

    int targetId = std::stoi(opts.at("id"));
    bool found = false;

    for (rjson::SizeType i = 0; i < doc.Size(); i++) {
        if (doc[i]["id"].GetInt() == targetId) {
            doc.Erase(doc.Begin() + i);
            found = true;
            break;
        }
    }
    
    if (found)
        std::cout << "Expense deleted succesfully (ID: " << targetId << ")\n";
    else 
        std::cerr << "No expense found with ID " << targetId << '\n';
}

bool handleCommands(rjson::Document& doc, std::string cmd, std::map<std::string, std::string>& opts) {
    if (cmd == "add") cmdAdd(doc, opts);
    else if (cmd == "list") cmdList(doc);
    else if (cmd == "summary") cmdSummary(doc, opts);
    else if (cmd == "delete") cmdDelete(doc, opts);
    else {
        std::cerr << "Provide correct command: [add|list|summary|delete]\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr<< "usage: " << argv[0] << " [add|list|summary|delete] [options]\n";
        return 0;
    }

    std::string filename = "expenses.json";
    rjson::Document data = loadExpenses(filename);
    auto options = parseArguments(argc, argv);
    std::string command = argv[1];
    
    if (!handleCommands(data, command, options))
        return 0;
    saveExpenses(filename, data);
    
    return 0;
}
