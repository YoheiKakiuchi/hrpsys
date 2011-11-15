#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
using namespace std;
#include "interpolator.h"

interpolator::interpolator(int dim_, double dt_, interpolation_mode imode_)
{
  imode = imode_;
  dim = dim_;
  dt = dt_;
  length = 0;
  g = new double[dim];
  x = new double[dim];
  v = new double[dim];
  a = new double[dim];
  for (int i=0; i<dim; i++){
    g[i] = x[i] = v[i] = a[i] = 0.0;
  }
  delay = 0;
}

interpolator::~interpolator()
{
  clear();
  delete [] g;
  delete [] x;
  delete [] v;
  delete [] a;
}

void interpolator::clear()
{
  while (!isEmpty()){
    pop();
  }
}

// 1dof interpolator
void interpolator::hoffarbib(double &remain_t, double goal,
			     double &xx, double &vv, double &aa)
{
  double da;
  double rm_t = remain_t + delay;
#define EPS 1e-6
  if (rm_t > dt+EPS){
    da = -9*aa/rm_t - 36*vv/(rm_t*rm_t)
      + 60*(goal-xx)/(rm_t*rm_t*rm_t);
    aa += da*dt;
    vv += aa*dt;
    xx += vv*dt;
    remain_t -= dt;
  }else{
    aa = vv = 0;
    xx = goal;
    remain_t = 0;
  }
}

void interpolator::linear_interpolation(double &remain_t, double goal,
					double &xx, double &vv, double &aa)
{
  if (remain_t > dt+EPS){
    aa = 0;
    vv = (goal-xx)/remain_t;
    xx += vv*dt;
    remain_t -= dt;
  }else{
    aa = vv = 0;
    xx = goal;
    remain_t = 0;
  }
}

void interpolator::sync()
{
  //cout << "sync:" << length << "," << q.size() << endl;
  length = q.size();
}

double interpolator::calc_interpolation_time(const double *newg,
					     double avg_vel)
{
  double remain_t;
  double max_diff = 0, diff;
  for (int i=0; i<dim; i++){
    diff = fabs(newg[i]-g[i]);
    if (diff > max_diff) max_diff = diff;
  }
  remain_t = max_diff/avg_vel;
#define MIN_INTERPOLATION_TIME	(1.0)
  if (remain_t < MIN_INTERPOLATION_TIME) remain_t = MIN_INTERPOLATION_TIME;
  return remain_t;
}

bool interpolator::setInterpolationMode (interpolation_mode i_mode_)
{
    if (i_mode_ != LINEAR && i_mode_ != HOFFARBIB) return false;
    imode = i_mode_;
    return true;
};

void interpolator::go(const double *newg, double time, bool immediate)
{
  int i;
  double remain_t = time, rm_t=-1;
  if (remain_t == 0) remain_t = calc_interpolation_time(newg);
  while(remain_t>0){
    for (i=0; i<dim; i++){
      rm_t = remain_t;
      switch(imode){
      case LINEAR:
	linear_interpolation(rm_t, newg[i], x[i], v[i], a[i]);
	break;
      case HOFFARBIB:
	hoffarbib(rm_t, newg[i], x[i], v[i], a[i]);
	break;
      }
    }
    push(x, false);
    remain_t = rm_t;
  }
  if (immediate) sync();
}

void interpolator::load(const char *fname, double time_to_start, double scale,
			bool immediate)
{
  ifstream strm(fname);
  if (!strm.is_open()) {
    cerr << "file not found(" << fname << ")" << endl;
    return;
  }
  double *vs, ptime=-1,time;
  vs = new double[dim];
  strm >> time;
  while(strm.eof()==0){
    for (int i=0; i<dim; i++){
      strm >> vs[i];
    }
    if (ptime <0){
      go(vs, time_to_start, false);
    }else{
      go(vs, scale*(time-ptime), false);
    }
    ptime = time;
    strm >> time;
  }
  strm.close();
  delete [] vs;
  if (immediate) sync();
}

void interpolator::load(string fname, double time_to_start, double scale,
			bool immediate)
{
  load(fname.c_str(), time_to_start, scale, immediate);
}

void interpolator::push(const double *a, bool immediate)
{
  double *p = new double[dim];
  memcpy(p, a, sizeof(double)*dim);
  q.push_back(p);
  if (immediate) sync();
}

void interpolator::pop()
{
  if (length > 0){
    length--;
    double *&vs = q.front();
    delete [] vs;
    q.pop_front();
  }
}

void interpolator::pop_back()
{
  if (length > 0){
    length--;
    double *&vs = q.back();
    delete [] vs;
    q.pop_back();
    if (length > 0){
      memcpy(x, q.back(), sizeof(double)*dim);
    }else{
      memcpy(x, g, sizeof(double)*dim);
    }
  }
}

void interpolator::set(const double *angle)
{
  for (int i=0; i<dim; i++){
    g[i] = x[i] = angle[i];
    v[i] = a[i] = 0;
  }
}

double *interpolator::front()
{
  if (length!=0){
    return q.front();
  }else{
    return g;
  }
}

void interpolator::get(double *a, bool popp)
{
  if (length!=0){
    double *&vs = q.front();
    if (vs == NULL) {
      cerr << "interpolator::get vs = NULL, q.size() = " << q.size() 
	   << ", length = " << length << endl;
    }
    memcpy(g, vs, sizeof(double)*dim);
    if (popp) pop();
  }
  memcpy(a, g, sizeof(double)*dim);
}

bool interpolator::isEmpty()
{
  if (length==0){
    return true;
  }else{
    return false;
  }
}

double interpolator::remain_time()
{
  return dt*length;
}