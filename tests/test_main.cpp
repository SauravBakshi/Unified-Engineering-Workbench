#include "thermal.hpp"
#include "importers.hpp"
#include "backprop.hpp"
#include "benchmark.hpp"
#include "embedded.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <limits>

static void require(bool ok,const std::string& msg){if(!ok)throw std::runtime_error(msg);}
template<class F> static void must_throw(F fn,const std::string& msg){bool threw=false;try{fn();}catch(const std::exception&){threw=true;}require(threw,msg);}
int main(){
 try {
   const auto temp=std::filesystem::temp_directory_path();
   ThermalConfig c;
   auto r1=solve_thermal(c,1),r2=solve_thermal(c,2);
   auto r10=solve_thermal(c,10),r20=solve_thermal(c,20),r40=solve_thermal(c,40),r80=solve_thermal(c,80);
   require(r1.temperature.front()==100 && r1.temperature.back()==20,"single-element boundary handling");
   require(std::abs(r2.temperature[1]-exact_temperature(c,.5))<1e-10,"two-element interior solution");
   require(r10.temperature.front()==100 && r10.temperature.back()==20,"Dirichlet boundary conditions");
   require(std::abs(r10.max_midpoint_error-1.25)<1e-10,"N=10 analytical midpoint error");
   require(std::abs(r20.max_midpoint_error-.3125)<1e-10,"N=20 analytical midpoint error");
   require(std::abs(r40.max_midpoint_error-.078125)<1e-10,"N=40 analytical midpoint error");
   require(std::abs(r80.max_midpoint_error-.01953125)<1e-10,"N=80 analytical midpoint error");
   require(std::abs(r10.max_midpoint_error/r20.max_midpoint_error-4)<1e-10,"second-order convergence ratio");
   ThermalConfig bad_geometry=c;bad_geometry.length=0;must_throw([&]{solve_thermal(bad_geometry,10);},"reject zero rod length");
   require(std::abs(exact_temperature(c,0)-100)<1e-12 && std::abs(exact_temperature(c,1)-20)<1e-12,"analytical boundary values");
   must_throw([&]{solve_thermal(c,0);},"reject zero elements");
   ThermalConfig bad=c;bad.conductivity=std::numeric_limits<double>::quiet_NaN();must_throw([&]{solve_thermal(bad,10);},"reject non-finite thermal config");

   const auto stl=temp/"uew_test.stl";{std::ofstream f(stl);f<<"solid t\n facet normal 0 0 1\n outer loop\n vertex 0 0 0\n vertex 1 0 0\n vertex 0 1 0\n endloop\n endfacet\nendsolid\n";}
   auto ss=validate_ascii_stl(stl.string());require(ss.triangles==1,"valid ASCII STL");
   {std::ofstream f(stl);f<<"solid bad\n facet normal nan 0 1\n outer loop\n vertex 0 0 0\n vertex 1 0 0\n vertex 0 1 0\n endloop\n endfacet\nendsolid\n";}
   must_throw([&]{validate_ascii_stl(stl.string());},"reject malformed/non-finite STL normal");
   {std::ofstream f(stl);}
   must_throw([&]{validate_ascii_stl(stl.string());},"reject empty STL");
   const auto csv=temp/"uew_test.csv";{std::ofstream f(csv);f<<"node_id,x_m,y_m,z_m,temperature_C\r\n1,0,0,0,20\r\n2,1,0,0,30\r\n";}
   auto cs=validate_temperature_csv(csv.string());require(cs.nodes==2 && cs.min_temp==20 && cs.max_temp==30,"valid CRLF nodal CSV");
   {std::ofstream f(csv);f<<"node_id,x_m,y_m,z_m,temperature_C\n1,0,nan,0,20\n";}
   must_throw([&]{validate_temperature_csv(csv.string());},"reject non-finite CSV values");
   {std::ofstream f(csv);f<<"node_id,x_m,y_m,z_m,temperature_C\n1,0,0,0,20\n1,1,0,0,30\n";}
   must_throw([&]{validate_temperature_csv(csv.string());},"reject duplicate node ids");
   std::filesystem::remove(stl);std::filesystem::remove(csv);

   auto bp=run_backprop();require(bp.max_finite_difference_error<1e-5,"finite difference gradient check");
   must_throw([&]{run_backprop(1,2,.5,0,1,0,.1,0);},"reject zero finite-difference epsilon");
   require(std::abs(bp.gradient[0]-bp.gradient[1])<1e-12,"w and b gradients for x=1");
   const auto benchfile=temp/"uew_benchmark_test.csv";auto rows=run_benchmark(benchfile.string(),12,12,2026);
   require(!rows.empty() && rows.front().workers==1,"serial benchmark row");
   for(const auto& row:rows)require(row.max_abs_difference_C<=1e-8,"serial/parallel numerical agreement");
   std::filesystem::remove(benchfile);
   auto dev=demo_embedded_devices();require(dev.size()==3,"three mock devices");
   must_throw([&]{update_embedded_device("telemetry",1);},"reject telemetry before connection");
   auto connected=update_embedded_device("connect",0);require(connected[0].state.find("Connected")==0,"MCU mock connect");
   auto telemetry=update_embedded_device("telemetry",0);require(telemetry[0].message.find("Telemetry")!=std::string::npos,"MCU mock telemetry");
   auto recovered=update_embedded_device("retry",2);require(recovered[2].state.find("Connected after retry")==0,"JTAG mock timeout recovery");
   auto disconnected=update_embedded_device("disconnect",0);require(disconnected[0].state.find("Disconnected")==0,"MCU mock disconnect");
   const auto lossfile=temp/"uew_loss_test.csv";write_loss_csv(lossfile.string(),100,.1);
   std::string header;double step0=0,last=0;
   {
     std::ifstream lossin(lossfile);
     require(static_cast<bool>(lossin),"open loss CSV for validation");
     std::getline(lossin,header);
     int step=0;
     while(lossin>>step){char comma;double value;lossin>>comma>>value;if(step==0)step0=value;last=value;}
   } // Close the file before deleting it (required on Windows).
   require(header=="step,loss" && last<step0,"100-step training loss decreases");
   std::filesystem::remove(lossfile);
   std::cout<<"PASS: thermal BCs, analytical errors, convergence, invalid configs, STL/CSV validation, CRLF, negative parser cases, finite-difference gradients, serial/parallel agreement, embedded mocks\n";
   return 0;
 } catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
