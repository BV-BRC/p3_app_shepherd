#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>

using namespace std::chrono;

namespace pt = boost::property_tree;

int main()
{
    std::cout << std::is_same<high_resolution_clock, system_clock>{} << std::endl;
    pt::ptree root;
    pt::ptree pnode;
    pt::ptree procs;
    procs.put("", 100);
    pnode.push_back(std::make_pair("", procs));
    procs.put("", 101);
    pnode.push_back(std::make_pair("", procs));
    root.add_child("procs", pnode);
    pt::write_json(std::cout, root);
    return 0;
}
