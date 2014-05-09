/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ArcSearch.cpp                                    */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "BHV_ArcSearch.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"
#include "AngleUtils.h"
#include <math.h>
#include <sstream>

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_ArcSearch::BHV_ArcSearch(IvPDomain domain) :
  IvPBehavior(domain), m_startup(true), m_wpt_idx(0), m_wpt_final_idx(0)
{
  // Provide a default behavior name
  IvPBehavior::setParam("name", "defaultname");

  // Declare the behavior decision space
  m_domain = subDomain(m_domain, "course,speed");

  m_desired_speed  = 0;
  m_arrival_radius = 10;
  m_osx  = 0;
  m_osy  = 0;

  // Add any variables this behavior needs to subscribe for
  addInfoVars("NAV_X, NAV_Y");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_ArcSearch::setParam(string param, string val)
{
  // Convert the parameter to lower case for more general matching
  param = tolower(param);

  // Get the numerical value of the param argument for convenience once
  double double_val = atof(val.c_str());

  if((param == "foo") && isNumber(val)) {
    // Set local member variables here
    return(true);
  }
  else if (param == "bar") {
    // return(setBooleanOnString(m_my_bool, val));
  }
  else if((param == "center_x")  && (isNumber(val))) {
    m_center_x = double_val;
    return(true);
  }
  else if((param == "center_y")  && (isNumber(val))) {
    m_center_y = double_val;
    return(true);
  }
  else if((param == "angle") && (isNumber(val))) {
    m_angle = double_val;
    return(true);
  }
  else if((param == "speed") && (double_val > 0) && (isNumber(val))) {
    m_desired_speed = double_val;
    return(true);
  }
  else if((param == "arrival_radius") && (double_val > 0) && (isNumber(val))) {
    m_arrival_radius = double_val;
    return(true);
  }
  else if((param == "swath") && (isNumber(val))) {
    m_swath = double_val;
    return(true);
  }
  else if((param == "num_points") && (double_val > 0) && (isNumber(val))) {
    m_num_points = double_val;
    return(true);
  }
  else if((param == "radius") && (isNumber(val))) {
    m_radius = double_val;
    return(true);
  }

  // If not handled above, then just return false;
  return(false);
}

//---------------------------------------------------------------
// Procedure: onSetParamComplete()
//   Purpose: Invoked once after all parameters have been handled.
//            Good place to ensure all required params have are set.
//            Or any inter-param relationships like a<b.

void BHV_ArcSearch::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()
//   Purpose: Invoked once upon helm start, even if this behavior
//            is a template and not spawned at startup

void BHV_ArcSearch::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()
//   Purpose: Invoked on each helm iteration if conditions not met.

void BHV_ArcSearch::onIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_ArcSearch::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()
//   Purpose: Invoked each time a param is dynamically changed

void BHV_ArcSearch::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()
//   Purpose: Invoked once upon each transition from idle to run state

void BHV_ArcSearch::onIdleToRunState()
{
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()
//   Purpose: Invoked once upon each transition from run to idle state

void BHV_ArcSearch::onRunToIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onRunState()
//   Purpose: Invoked each iteration when run conditions have been met.

IvPFunction* BHV_ArcSearch::onRunState()
{
  // Part 1: Build the IvP function
  IvPFunction *ipf = 0;

  bool ok1, ok2;
  m_osx = getBufferDoubleVal("NAV_X", ok1);
  m_osy = getBufferDoubleVal("NAV_Y", ok2);
  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in info_buffer.");
    return(0);
  }

  //calculate arc points
  if (m_startup) {
    double start_angle = m_angle-(m_swath/2);
    double pie_angles = m_swath/(m_num_points-1);
    stringstream viewseglist;
    viewseglist << "pts={";
    for(unsigned int i = 0; i < m_num_points; i++) {
      double angle = start_angle + pie_angles*i;
      double x = m_center_x + m_radius*cos(angle*M_PI/180);
      double y = m_center_y + m_radius*sin(angle*M_PI/180);
      viewseglist << x << "," << y << ":";
      m_x_points.push_back(x);
      m_y_points.push_back(y);
    }
    viewseglist << "},label=arcsearch,edge_size=4";
    postMessage("VIEW_SEGLIST", viewseglist.str());
    m_wpt_final_idx = m_num_points-1;
    m_wpt_idx = 0;
    m_nextpt_x = m_x_points[0];
    m_nextpt_y = m_y_points[0];
    m_startup = false;
  }

  if(m_wpt_idx > m_wpt_final_idx) {
    setComplete();
    return(0);
  } else {
    ZAIC_PEAK spd_zaic(m_domain, "speed");
    spd_zaic.setSummit(m_desired_speed);
    spd_zaic.setPeakWidth(0.5);
    spd_zaic.setBaseWidth(1.0);
    spd_zaic.setSummitDelta(0.8);
    if(spd_zaic.stateOK() == false) {
      string warnings = "Speed ZAIC problems " + spd_zaic.getWarnings();
      postWMessage(warnings);
      return(0);
    }

    double dist = hypot((m_nextpt_x-m_osx), (m_nextpt_y-m_osy));
    if(dist <= m_arrival_radius) {
      m_wpt_idx++;
    }
    m_nextpt_x = m_x_points[m_wpt_idx];
    m_nextpt_y = m_y_points[m_wpt_idx];
    double rel_ang_to_wpt = relAng(m_osx, m_osy, m_nextpt_x, m_nextpt_y);
    ZAIC_PEAK crs_zaic(m_domain, "course");
    crs_zaic.setSummit(rel_ang_to_wpt);
    crs_zaic.setPeakWidth(0);
    crs_zaic.setBaseWidth(180.0);
    crs_zaic.setSummitDelta(0);
    crs_zaic.setValueWrap(true);
    if(crs_zaic.stateOK() == false) {
      string warnings = "Course ZAIC problems " + crs_zaic.getWarnings();
      postWMessage(warnings);
      return(0);
    }

    IvPFunction *spd_ipf = spd_zaic.extractIvPFunction();
    IvPFunction *crs_ipf = crs_zaic.extractIvPFunction();

    OF_Coupler coupler;
    ipf = coupler.couple(crs_ipf, spd_ipf, 50, 50);
  }

  // Part N: Prior to returning the IvP function, apply the priority wt
  // Actual weight applied may be some value different than the configured
  // m_priority_wt, depending on the behavior author's insite.
  if(ipf)
    ipf->setPWT(m_priority_wt);

  return(ipf);
}

