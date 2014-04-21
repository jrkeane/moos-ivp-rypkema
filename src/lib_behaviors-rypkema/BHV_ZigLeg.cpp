/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ZigLeg.cpp                                    */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "BHV_ZigLeg.h"
#include "XYRangePulse.h"
#include "ZAIC_PEAK.h"
#include <math.h>

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_ZigLeg::BHV_ZigLeg(IvPDomain domain) :
  IvPBehavior(domain), m_wpt_idx(-1), m_wpt_time(0.0), m_xpos(0.0), m_ypos(0.0), m_pulse(false), m_range(20.0), m_duration(10.0), m_angle(45.0), m_zig(false)
{
  // Provide a default behavior name
  IvPBehavior::setParam("name", "defaultname");

  // Declare the behavior decision space
  m_domain = subDomain(m_domain, "course,speed");

  // Add any variables this behavior needs to subscribe for
  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("WPT_INDEX");
  addInfoVars("NAV_HEADING");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_ZigLeg::setParam(string param, string val)
{
  // Convert the parameter to lower case for more general matching
  param = tolower(param);

  // Get the numerical value of the param argument for convenience once
  double double_val = atof(val.c_str());

  if((param == "zig_angle") && isNumber(val)) {
    m_angle = double_val;
    return(true);
  }
  else if ((param == "zig_duration") && isNumber(val)) {
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

void BHV_ZigLeg::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()
//   Purpose: Invoked once upon helm start, even if this behavior
//            is a template and not spawned at startup

void BHV_ZigLeg::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()
//   Purpose: Invoked on each helm iteration if conditions not met.

void BHV_ZigLeg::onIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_ZigLeg::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()
//   Purpose: Invoked each time a param is dynamically changed

void BHV_ZigLeg::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()
//   Purpose: Invoked once upon each transition from idle to run state

void BHV_ZigLeg::onIdleToRunState()
{
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()
//   Purpose: Invoked once upon each transition from run to idle state

void BHV_ZigLeg::onRunToIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onRunState()
//   Purpose: Invoked each iteration when run conditions have been met.

IvPFunction* BHV_ZigLeg::onRunState()
{
  // Part 1: Build the IvP function
  IvPFunction *ipf = 0;

  updateStateVars();

  double five_secs = getBufferCurrTime();
  if (m_pulse) {
    if ((five_secs - m_wpt_time) >= 7.0) {
      m_pulse = false;
      m_zig = true;
    }
  }
  if (m_zig) {
    ZAIC_PEAK crs_zaic(m_domain, "course");
    double ang = fmod((m_heading+m_angle), 360.0);
    crs_zaic.setSummit(ang);
    crs_zaic.setPeakWidth(0);
    crs_zaic.setBaseWidth(180.0);
    crs_zaic.setSummitDelta(0);
    crs_zaic.setValueWrap(true);
    if(crs_zaic.stateOK() == false) {
      string warnings = "Course ZAIC problems " + crs_zaic.getWarnings();
      postWMessage(warnings);
      return(0);
    }

    ipf = crs_zaic.extractIvPFunction();

    if ((five_secs - m_wpt_time) >= (7.0 + m_duration)) {
      m_zig = false;
    }
  }

  // Part N: Prior to returning the IvP function, apply the priority wt
  // Actual weight applied may be some value different than the configured
  // m_priority_wt, depending on the behavior author's insite.
  if(ipf)
    ipf->setPWT(m_priority_wt);

  return(ipf);
}

void BHV_ZigLeg::updateStateVars()
{
  bool ok1, ok2, ok3, ok4;
  int temp_idx;
  m_xpos = getBufferDoubleVal("NAV_X", ok1);
  m_ypos = getBufferDoubleVal("NAV_Y", ok2);
  temp_idx = (int)getBufferDoubleVal("WPT_INDEX", ok3);
  if ((m_wpt_idx != temp_idx) && (m_wpt_idx != -1)) {
    m_pulse = true;
    m_wpt_time = getBufferCurrTime();
  }
  m_wpt_idx = temp_idx;
  if (!m_zig) {
    m_heading = getBufferDoubleVal("NAV_HEADING", ok4);
  } else {
    ok4 = true;
  }
  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in buffer.");
  }
  if (!ok3) {
    postWMessage("No waypoint index info in buffer.");
  }
  if (!ok4) {
    postWMessage("No ownship heading info in buffer.");
  }
}
