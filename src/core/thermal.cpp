#include "thermal.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

double exact_temperature(const ThermalConfig& c, double x) {
    return c.left_temperature + (c.right_temperature-c.left_temperature)*x/c.length
        + (c.generation/(2.0*c.conductivity))*x*(c.length-x);
}
ThermalResult solve_thermal(const ThermalConfig& c, int n) {
    if(n<1 || n>1000000 || !std::isfinite(c.length) || !std::isfinite(c.area) ||
       !std::isfinite(c.conductivity) || !std::isfinite(c.generation) ||
       !std::isfinite(c.left_temperature) || !std::isfinite(c.right_temperature) ||
       c.length<=0 || c.area<=0 || c.conductivity<=0)
        throw std::invalid_argument("Elements must be 1..1,000,000; geometry and conductivity must be positive finite values; all thermal parameters must be finite.");
    const double h=c.length/n, ke=c.conductivity*c.area/h;
    std::vector<double> K(n+1,0.0), off(n,0.0), F(n+1,0.0);
    // 2-node linear element: ke * [[1,-1],[-1,1]], fe=q*A*h/2*[1,1]
    for(int e=0;e<n;++e) {
        K[e]+=ke; K[e+1]+=ke; off[e]=-ke;
        F[e]+=c.generation*c.area*h/2.0;
        F[e+1]+=c.generation*c.area*h/2.0;
    }
    // Eliminate the prescribed boundary degrees of freedom from the interior RHS.
    // The remaining tridiagonal system is solved with the Thomas algorithm.
    if(n>1) {
        const int m=n-1; // number of unconstrained interior nodes
        std::vector<double> lower(m,0.0), diagonal(m,0.0), upper(m,0.0), rhs(m,0.0);
        for(int i=1;i<n;++i) {
            const int row=i-1;
            lower[row]=off[i-1];
            diagonal[row]=K[i];
            upper[row]=(i<n-1)?off[i]:0.0;
            rhs[row]=F[i];
            if(i==1) rhs[row]-=off[0]*c.left_temperature;
            if(i==n-1) rhs[row]-=off[n-1]*c.right_temperature;
        }
        for(int row=1;row<m;++row) {
            const double factor=lower[row]/diagonal[row-1];
            diagonal[row]-=factor*upper[row-1];
            rhs[row]-=factor*rhs[row-1];
        }
        std::vector<double> t(n+1,0.0);
        t[0]=c.left_temperature; t[n]=c.right_temperature;
        t[n-1]=rhs[m-1]/diagonal[m-1];
        for(int row=m-2;row>=0;--row)
            t[row+1]=(rhs[row]-upper[row]*t[row+2])/diagonal[row];
        ThermalResult r; r.x.resize(n+1); r.temperature=t;
        for(int i=0;i<=n;++i) r.x[i]=i*h;
        for(int e=0;e<n;++e) {
            double xm=(e+.5)*h;
            double fem=.5*(t[e]+t[e+1]);
            r.max_midpoint_error=std::max(r.max_midpoint_error,std::abs(fem-exact_temperature(c,xm)));
        }
        return r;
    }
    ThermalResult r; r.x={0,c.length}; r.temperature={c.left_temperature,c.right_temperature};
    double xm=c.length/2, fem=(c.left_temperature+c.right_temperature)/2;
    r.max_midpoint_error=std::abs(fem-exact_temperature(c,xm));
    return r;
}
void write_thermal_csv(const ThermalResult& r,const std::string& path) {
    std::ofstream f(path); if(!f) throw std::runtime_error("Cannot write "+path);
    f<<"x_m,temperature_C\n"<<std::setprecision(12);
    for(std::size_t i=0;i<r.x.size();++i) f<<r.x[i]<<","<<r.temperature[i]<<"\n";
}
