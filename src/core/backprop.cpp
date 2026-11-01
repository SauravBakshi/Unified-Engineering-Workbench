#include "backprop.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
static double loss(double x,double y,double w,double b,double v,double c) {
    double h=std::tanh(w*x+b), yh=v*h+c, e=yh-y; return .5*e*e;
}
BackpropResult run_backprop(double x,double y,double w,double b,double v,double c,double lr,double eps) {
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(b)||
       !std::isfinite(v)||!std::isfinite(c)||!std::isfinite(lr)||!std::isfinite(eps)||lr<=0||eps<=0)
        throw std::invalid_argument("Backprop inputs and learning rate/epsilon must be finite; learning rate and epsilon must be positive.");
    double h=std::tanh(w*x+b), yh=v*h+c, e=yh-y, dJdy=e;
    double dJdc=dJdy, dJdv=dJdy*h, dJdh=dJdy*v, dz=dJdh*(1-h*h);
    std::array<double,4> g={dz*x,dz,dJdv,dJdc};
    double p[4]={w,b,v,c}; double maxerr=0;
    for(int i=0;i<4;++i) {
        double plus[4]={w,b,v,c}, minus[4]={w,b,v,c};
        plus[i]+=eps; minus[i]-=eps;
        double fd=(loss(x,y,plus[0],plus[1],plus[2],plus[3])-loss(x,y,minus[0],minus[1],minus[2],minus[3]))/(2*eps);
        maxerr=std::max(maxerr,std::abs(fd-g[i]));
    }
    BackpropResult r; r.h=h; r.prediction=yh; r.loss=.5*e*e; r.gradient=g; r.max_finite_difference_error=maxerr;
    for(int i=0;i<4;++i) r.updated[i]=p[i]-lr*g[i];
    return r;
}
void write_loss_csv(const std::string& path,int steps,double lr) {
    if(steps<1 || !std::isfinite(lr) || lr<=0) throw std::invalid_argument("Training steps and learning rate must be positive and finite.");
    double x=1,y=2,w=.5,b=0,v=1,c=0;
    std::ofstream f(path); if(!f) throw std::runtime_error("Cannot write loss CSV");
    f<<"step,loss\n"<<std::setprecision(12);
    for(int s=0;s<=steps;++s) {
        f<<s<<","<<loss(x,y,w,b,v,c)<<"\n";
        auto r=run_backprop(x,y,w,b,v,c,lr);
        w=r.updated[0];b=r.updated[1];v=r.updated[2];c=r.updated[3];
    }
}
