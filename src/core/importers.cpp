#include "importers.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <algorithm>
#include <cctype>
#include <set>

static std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    const auto last=value.find_last_not_of(" \t\r\n");
    return value.substr(first,last-first+1);
}
static std::string read_file(const std::string& p,std::size_t limit) {
    std::ifstream f(p,std::ios::binary);
    if(!f) throw std::runtime_error("Cannot open file: "+p);
    f.seekg(0,std::ios::end); auto sz=f.tellg();
    if(sz<=0) throw std::runtime_error("File is empty.");
    if(static_cast<std::size_t>(sz)>limit) throw std::runtime_error("File exceeds documented 20 MiB limit.");
    f.seekg(0); std::ostringstream s; s<<f.rdbuf(); return s.str();
}
StlStats validate_ascii_stl(const std::string& path,std::size_t max_bytes) {
    auto s=read_file(path,max_bytes);
    if(s.find("solid")==std::string::npos || s.find("facet normal")==std::string::npos)
        throw std::runtime_error("Not a supported ASCII STL: expected solid/facet normal records.");
    std::regex vr(R"(vertex\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+))");
    std::regex nr(R"(facet\s+normal\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+))");
    StlStats out; for(int i=0;i<3;++i){out.min[i]=std::numeric_limits<double>::infinity();out.max[i]=-out.min[i];}
    auto begin=std::sregex_iterator(s.begin(),s.end(),vr), end=std::sregex_iterator();
    std::size_t vertices=0;
    for(auto it=begin;it!=end;++it) {
        double v[3]; for(int j=0;j<3;++j) { v[j]=std::stod((*it)[j+1]); if(!std::isfinite(v[j])) throw std::runtime_error("STL contains non-finite vertex."); }
        for(int j=0;j<3;++j){out.min[j]=std::min(out.min[j],v[j]);out.max[j]=std::max(out.max[j],v[j]);}
        ++vertices;
    }
    if(vertices==0 || vertices%3!=0) throw std::runtime_error("STL has incomplete triangle vertex data.");
    std::size_t normals=0;
    for(auto it=std::sregex_iterator(s.begin(),s.end(),nr);it!=end;++it) {
        for(int j=1;j<=3;++j) if(!std::isfinite(std::stod((*it)[j])))
            throw std::runtime_error("STL contains a non-finite facet normal.");
        ++normals;
    }
    const std::regex facet_token(R"(facet\s+normal)");
    std::size_t facet_tokens=0;
    for(auto it=std::sregex_iterator(s.begin(),s.end(),facet_token);it!=end;++it) ++facet_tokens;
    if(normals!=vertices/3 || facet_tokens!=normals) throw std::runtime_error("STL facet normals and triangle vertices are malformed or inconsistent.");
    out.triangles=vertices/3; return out;
}
CsvStats validate_temperature_csv(const std::string& path,std::size_t max_bytes) {
    auto s=read_file(path,max_bytes); std::istringstream in(s); std::string line;
    if(!std::getline(in,line) || trim(line)!="node_id,x_m,y_m,z_m,temperature_C")
        throw std::runtime_error("CSV header must be node_id,x_m,y_m,z_m,temperature_C");
    CsvStats out; std::set<long long> ids; out.min_temp=std::numeric_limits<double>::infinity(); out.max_temp=-out.min_temp;
    while(std::getline(in,line)) {
        line=trim(line); if(line.empty()) continue;
        std::stringstream ss(line); std::string cell; double vals[5]; int i=0;
        while(std::getline(ss,cell,',')) {
            if(i>=5) throw std::runtime_error("CSV row has too many columns.");
            cell=trim(cell); std::size_t used=0; vals[i]=std::stod(cell,&used);
            if(used!=cell.size() || !std::isfinite(vals[i])) throw std::runtime_error("CSV contains malformed or non-finite data.");
            ++i;
        }
        if(i!=5) throw std::runtime_error("CSV row must contain exactly five columns.");
        if(vals[0]<1 || vals[0]>static_cast<double>(std::numeric_limits<int>::max()) || std::floor(vals[0])!=vals[0])
            throw std::runtime_error("node_id must be a positive integer.");
        if(!ids.insert(static_cast<long long>(vals[0])).second) throw std::runtime_error("CSV contains duplicate node_id values.");
        out.min_temp=std::min(out.min_temp,vals[4]); out.max_temp=std::max(out.max_temp,vals[4]); ++out.nodes;
    }
    if(out.nodes==0) throw std::runtime_error("CSV contains a header but no nodes.");
    return out;
}
