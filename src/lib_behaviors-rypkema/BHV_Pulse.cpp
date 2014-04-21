/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_Pulse.cpp                                    */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "BHV_Pulse.h"
#include "XYRangePulse.h"

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_Pulse::BHV_Pulse(IvPDomain domain) :
  IvPBehavior(domain), m_wpt_idx(-1), m_wpt_time(0.0), m_xpos(0.0), m_ypos(0.0), m_pulse(false), m_range(20.0), m_duration(2)
{
  // Provide a default behavior name
  IvPBehavior::setParam("name", "defaultname");

  // Declare the behavior decision space
  m_domain = subDomain(m_domain, "course,speed");

  // Add any variables this behavior needs to subscribe for
  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("WPT_INDEX");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_Pulse::setParam(string param, string val)
{
  // Convert the parameter to lower case for more general matching
  param = tolower(param);

  // Get the numerical value of the param argument for convenience once
  double double_val = atof(val.c_str());

  if((param == "pulse_range") && isNumber(val)) {
    m_range = double_val;
    return(true);
  }
  else if ((param == "pulse_duration") && isNumber(val)) {
    m_duration = double_val;
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

void BHV_Pulse::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()
//   Purpose: Invoked once upon helm start, even if this behavior
//            is a template and not spawned at startup

void BHV_Pulse::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()
//   Purpose: Invoked on each helm iteration if conditions not met.

void BHV_Pulse::onIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_Pulse::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()
//   Purpose: Invoked each time a param is dynamically changed

void BHV_Pulse::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()
//   Purpose: Invoked once upon each transition from idle to run state

void BHV_Pulse::onIdleToRunState()
{
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()
//   Purpose: Invoked once upon each transition from run to idle state

void BHV_Pulse::onRunToIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onRunState()
//   Purpose: Invoked each iteration when run conditions have been met.

IvPFunction* BHV_Pulse::onRunState()
{
  // Part 1: Build the IvP function
  IvPFunction *ipf = 0;

  updateStateVars();

  double five_secs = getBufferCurrTime();
  if (m_pulse) {
    if ((five_secs - m_wpt_time) >= 5.0) {
      m_pulse = false;
      XYRangePulse pulse;
      pulse.set_x(m_xpos);
      pulse.set_y(m_ypos);
      pulse.set_label("bhv_pulse");
      pulse.set_rad(m_range);
      pulse.set_duration(m_duration);
      pulse.set_time(five_secs);
      pulse.set_color("edge", "yellow");
      pulse.set_color("fill", "yellow");
      string spec = pulse.get_spec();
      postMessage("VIEW_RANGE_PULSE", spec);
    }
  }

  // Part N: Prior to returning the IvP function, apply the priority wt
  // Actual weight applied may be some value different than the configured
  // m_priority_wt, depending on the behavior author's insite.
  if(ipf)
    ipf->setPWT(m_priority_wt);

  return(ipf);
}

void BHV_Pulse::updateStateVars()
{
  bool ok1, ok2, ok3;
  int temp_idx;
  m_xpos = getBufferDoubleVal("NAV_X", ok1);
  m_ypos = getBufferDoubleVal("NAV_Y", ok2);
  temp_idx = (int)getBufferDoubleVal("WPT_INDEX", ok3);
  if ((m_wpt_idx != temp_idx) && (m_wpt_idx != -1)) {
    m_pulse = true;
    m_wpt_time = getBufferCurrTime();
  }
  m_wpt_idx = temp_idx;
  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in buffer.");
  }
  if (!ok3) {
    postWMessage("No waypoint index info in buffer.");
  }
}
