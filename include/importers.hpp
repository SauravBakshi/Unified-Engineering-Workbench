#pragma once
#include <cstddef>
#include <string>
struct StlStats { std::size_t triangles=0; double min[3]{}, max[3]{}; };
struct CsvStats { std::size_t nodes=0; double min_temp=0, max_temp=0; };
StlStats validate_ascii_stl(const std::string& path, std::size_t max_bytes=20*1024*1024);
CsvStats validate_temperature_csv(const std::string& path, std::size_t max_bytes=20*1024*1024);
