#include "query5.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <algorithm>

std::mutex result_mutex;
// Function to parse command line arguments
bool parseArgs(int argc, char* argv[], std::string& r_name, std::string& start_date, std::string& end_date, int& num_threads, std::string& table_path, std::string& result_path) {
    if (argc != 13)
        return false;

    for (int i = 1; i < argc; i += 2) {
        std::string key = argv[i];
        std::string val = argv[i + 1];
        if (key == "--r_name") 
            r_name = val;
        else if (key == "--start_date") 
            start_date = val;
        else if (key == "--end_date") 
            end_date = val;
        else if (key == "--threads") 
            num_threads = std::stoi(val);
        else if (key == "--table_path") 
            table_path = val;
        else if (key == "--result_path") 
            result_path = val;
        else 
            return false;
    }

    return true;
    // Example: --r_name ASIA --start_date 1994-01-01 --end_date 1995-01-01 --threads 4 --table_path /path/to/tables --result_path /path/to/results
}

// Function to read TPCH data from the specified paths
bool readTPCHData(const std::string& table_path, std::vector<std::map<std::string, std::string>>& customer_data, std::vector<std::map<std::string, std::string>>& orders_data, std::vector<std::map<std::string, std::string>>& lineitem_data, std::vector<std::map<std::string, std::string>>& supplier_data, std::vector<std::map<std::string, std::string>>& nation_data, std::vector<std::map<std::string, std::string>>& region_data) {
    return readCSV(path + "/customer.tbl", customer_data) &&
           readCSV(path + "/orders.tbl", orders_data) &&
           readCSV(path + "/lineitem.tbl", lineitem_data) &&
           readCSV(path + "/supplier.tbl", supplier_data) &&
           readCSV(path + "/nation.tbl", nation_data) &&
           readCSV(path + "/region.tbl", region_data);
}
bool readCSV(const std::string& filename, std::vector<std::map<std::string, std::string>>& table) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    std::vector<std::string> headers;
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, '|')) 
                headers.push_back(cell);
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::map<std::string, std::string> row;
        for (const auto& header : headers) {
            if (!std::getline(ss, cell, '|')) break;
            row[header] = cell;
        }
        table.push_back(row);
    }

    return true;
}

// Function to execute TPCH Query 5 using multithreading
bool executeQuery5(const std::string& r_name, const std::string& start_date, const std::string& end_date, int num_threads, const std::vector<std::map<std::string, std::string>>& customer_data, const std::vector<std::map<std::string, std::string>>& orders_data, const std::vector<std::map<std::string, std::string>>& lineitem_data, const std::vector<std::map<std::string, std::string>>& supplier_data, const std::vector<std::map<std::string, std::string>>& nation_data, const std::vector<std::map<std::string, std::string>>& region_data, std::map<std::string, double>& results) {
    auto worker = [&](int tid, int total_threads) {
        std::map<std::string, double> local_results;
        for (size_t i = tid; i < lineitem_data.size(); i += total_threads) {
            const auto& row = lineitem_data[i];
            std::string nation = "NATION_" + std::to_string(i % 5); // mock
            double revenue = std::stod(row.at("l_extendedprice")) * (1 - std::stod(row.at("l_discount")));
            local_results[nation] += revenue;
        }

        std::lock_guard<std::mutex> lock(result_mutex);
        for (const auto& [nation, rev] : local_results) {
            results[nation] += rev;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker, i, num_threads);
    for (auto& t : threads)
        t.join();

    return true;
}

// Function to output results to the specified path
bool outputResults(const std::string& result_path, const std::map<std::string, double>& results) {
   std::ofstream out(result_path + "/query5_result.txt");
    if (!out.is_open()) 
    return false;

    for (const auto& [nation, revenue] : results) {
        out << nation << "|" << revenue << "\n";
    }

    return true;
} 
