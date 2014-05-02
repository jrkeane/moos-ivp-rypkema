/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_MySpiral.cpp                                    */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "BHV_MySpiral.h"
#include "ZAIC_PEAK.h"
#include <cmath>
#include "OF_Coupler.h"

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_MySpiral::BHV_MySpiral(IvPDomain domain) :
  IvPBehavior(domain), m_xpos(0.0), m_ypos(0.0), m_heading(0.0), m_init_heading(0.0), m_num_rot(0), m_rudder(140.0), m_desired_rudder(140.0), m_ccw(false), m_increment(1.0), m_increment_factor(0.5), m_desired_speed(2.0), m_time_length(100.0), m_enter_spiral(false), m_min_rudder(50), m_min_dist_bet_pts(10)
{
  // Provide a default behavior name
  IvPBehavior::setParam("name", "defaultname");

  // Declare the behavior decision space
  m_domain = subDomain(m_domain, "course,speed");

  // Add any variables this behavior needs to subscribe for
  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("NAV_HEADING");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_MySpiral::setParam(string param, string val)
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
  else if((param == "speed") && (double_val > 0) && (isNumber(val))) {
    m_desired_speed = double_val;
    return(true);
  }
  else if((param == "rudder_start") && (double_val > 0)  && (isNumber(val))) {
    m_desired_rudder = double_val;
    return(true);
  }
  else if((param == "rudder_increment") && (double_val > 0) && (isNumber(val))) {
    m_increment = double_val;
    return(true);
  }
  else if((param == "rudder_increment_factor") && (double_val > 0) && (isNumber(val))) {
    m_increment_factor = double_val;
    return(true);
  }
  else if((param == "spiral_time_length") && (double_val > 0) && (isNumber(val))) {
    m_time_length = double_val;
    return(true);
  }
  else if((param == "rudder_min") && (double_val > 0) && (isNumber(val))) {
    m_min_rudder = double_val;
    return(true);
  }
  else if((param == "points_distance") && (double_val > 0) && (isNumber(val))) {
    m_min_dist_bet_pts = double_val;
    return(true);
  }
  else if((param == "ccw") && (isNumber(val))) {
    if (double_val > 0) {
      m_ccw = true;
    } else {
      m_ccw = false;
    }
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

void BHV_MySpiral::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()
//   Purpose: Invoked once upon helm start, even if this behavior
//            is a template and not spawned at startup

void BHV_MySpiral::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()
//   Purpose: Invoked on each helm iteration if conditions not met.

void BHV_MySpiral::onIdleState()
{
  m_enter_spiral = false;
  //postWMessage("IDLE!");
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_MySpiral::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()
//   Purpose: Invoked each time a param is dynamically changed

void BHV_MySpiral::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()
//   Purpose: Invoked once upon each transition from idle to run state

void BHV_MySpiral::onIdleToRunState()
{
  //postWMessage("ENTERING SPIRAL!");
  bool ok1;
  m_init_heading = getBufferDoubleVal("NAV_HEADING", ok1);
  if (!ok1) {
    postWMessage("No ownship heading info in buffer.");
  }
  m_start_time = getBufferCurrTime();
  //postMessage("START TIME ------------------", m_start_time);
  m_rudder = m_desired_rudder;
  m_reduction = m_increment;
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()
//   Purpose: Invoked once upon each transition from run to idle state

void BHV_MySpiral::onRunToIdleState()
{
  //postWMessage("RUN TO IDLE");
}

//---------------------------------------------------------------
// Procedure: onRunState()
//   Purpose: Invoked each iteration when run conditions have been met.

IvPFunction* BHV_MySpiral::onRunState()
{
  if (!m_enter_spiral) {
    m_enter_spiral = true;
  }

  // Part 1: Build the IvP function
  IvPFunction *ipf = 0;

  // update state variables
  updateStateVars();

  m_elapsed_time = getBufferCurrTime() - m_start_time;
  //postMessage("ELAPSED TIME ------------------", m_elapsed_time);
  if (m_elapsed_time > m_time_length) {
    setComplete();
    m_enter_spiral = false;
    return(0);
  }

  m_reduction = m_reduction*m_increment_factor;
  m_rudder = m_rudder - m_reduction;
  if (m_rudder < m_min_rudder) {
    m_rudder = m_min_rudder;
  }

  ZAIC_PEAK rddr_zaic(m_domain, "course");
  double rudder;
  if (m_ccw) {
    rudder = -m_rudder;
  } else {
    rudder = m_rudder;
  }
  //postMessage("RUDDER ------------------", rudder);
  //postMessage("HEADING ------------------", m_heading);
  double des_heading = fmod((m_heading+rudder), 360.0);
  if (des_heading < 0) {
    des_heading += 360;
  }
  rddr_zaic.setSummit(des_heading);
  rddr_zaic.setPeakWidth(0);
  rddr_zaic.setBaseWidth(10.0);
  rddr_zaic.setSummitDelta(0);
  rddr_zaic.setValueWrap(true);
  if(rddr_zaic.stateOK() == false) {
    string warnings = "Rudder ZAIC problems " + rddr_zaic.getWarnings();
    postWMessage(warnings);
    return(0);
  }

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

  IvPFunction *spd_ipf = spd_zaic.extractIvPFunction();
  IvPFunction *rddr_ipf = rddr_zaic.extractIvPFunction();

  OF_Coupler coupler;
  ipf = coupler.couple(rddr_ipf, spd_ipf, 50, 50);

  // Part N: Prior to returning the IvP function, apply the priority wt
  // Actual weight applied may be some value different than the configured
  // m_priority_wt, depending on the behavior author's insite.
  if(ipf)
    ipf->setPWT(m_priority_wt);

  return(ipf);
}

void BHV_MySpiral::updateStateVars()
{
  bool ok1, ok2, ok3, ok4, ok5;
  string cw, ccw;
  int temp_idx;
  m_xpos = getBufferDoubleVal("NAV_X", ok1);
  m_ypos = getBufferDoubleVal("NAV_Y", ok2);
  m_heading = getBufferDoubleVal("NAV_HEADING", ok3);
  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in buffer.");
  }
  if (!ok3) {
    postWMessage("No ownship heading info in buffer.");
  }
}
