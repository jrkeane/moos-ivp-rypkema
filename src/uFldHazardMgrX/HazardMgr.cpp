/*****************************************************************/
/*    NAME: Michael Benjamin, Henrik Schmidt, and John Leonard   */
/*    ORGN: Dept of Mechanical Eng / CSAIL, MIT Cambridge MA     */
/*    FILE: HazardMgr.cpp                                        */
/*    DATE: Oct 26th 2012                                        */
/*                                                               */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License   */
/* as published by the Free Software Foundation; either version  */
/* 2 of the License, or (at your option) any later version.      */
/*                                                               */
/* This program is distributed in the hope that it will be       */
/* useful, but WITHOUT ANY WARRANTY; without even the implied    */
/* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/* PURPOSE. See the GNU General Public License for more details. */
/*                                                               */
/* You should have received a copy of the GNU General Public     */
/* License along with this program; if not, write to the Free    */
/* Software Foundation, Inc., 59 Temple Place - Suite 330,       */
/* Boston, MA 02111-1307, USA.                                   */
/*****************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "HazardMgr.h"
#include "XYFormatUtilsHazard.h"
#include "XYFormatUtilsPoly.h"
#include "ACTable.h"
#include "NodeMessage.h"

using namespace std;

//---------------------------------------------------------
// Constructor

HazardMgr::HazardMgr() : m_repeats(0), m_max_time(9999.0), m_options_set(false), m_hazard_reports(0), m_deploy_time(0), m_deployed(false), m_traverse_time(0), m_search_time(0), m_total_time(0), m_report_count(0), m_pen_false_alarm(0), m_pen_missed_hazard(0), m_vote_multiplier(1), m_many_hazard_mode(false), m_many_hazard_count(0), m_many_hazard_reset_time(true)
{
  // Config variables
  m_swath_width_desired = 25;
  m_pd_desired          = 0.9;

  // State Variables
  m_sensor_config_requested = false;
  m_sensor_config_set   = false;
  m_swath_width_granted = 0;
  m_pd_granted          = 0;

  m_sensor_config_reqs = 0;
  m_sensor_config_acks = 0;
  m_sensor_report_reqs = 0;
  m_detection_reports  = 0;

  m_summary_reports = 0;
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool HazardMgr::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key   = msg.GetKey();
    string sval  = msg.GetString();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    if(key == "UHZ_CONFIG_ACK")
      handleMailSensorConfigAck(sval);

    else if(key == "UHZ_OPTIONS_SUMMARY")
      handleMailSensorOptionsSummary(sval);

    else if(key == "UHZ_DETECTION_REPORT")
      handleMailDetectionReport(sval);

    else if(key == "HAZARDSET_REQUEST")
      handleMailReportRequest();

    else if(key == "UHZ_HAZARD_REPORT")
      handleMailHazardReport(sval);

    else if(key == "CLEAR_FOR_SHARE") {
      handleSendHazardReport();
    }

    else if(key == "SHARED_DATA") {
      handleReceiveHazardReport(sval);
    }

    else if(key == "DEPLOY") {
      if (sval=="true") {
        m_deployed = true;
        m_deploy_time = MOOSTime();
        m_rep_time = MOOSTime();
        m_options_set = true; // <----------------------------- put below?
      }
    }

    else if(key == "UHZ_MISSION_PARAMS") {
      if (!m_options_set) {
        handleMailMissionParams(sval);
      }
    }

//    else if(key == "HAZARDREPORT_OTHER") {
//      //m_hazard_set.findMinXPath(20);
//      string summary_report = m_hazard_set.getSpec("final_report");
//
//      Notify("HAZARDSET_REPORT", summary_report+sval);
//    }

    else if(key == "SEARCH_REPS") {
      if (m_traverse_time == 0) {
        m_traverse_time = m_search_time;
      }
      m_rep_time = MOOSTime();
      m_repeats++;
    }

    else
      reportRunWarning("Unhandled Mail: " + key);
  }

   return(true);
}

bool HazardMgr::handleMailSensorOptionsSummary(string str) {
  return true;
}

void HazardMgr::handleReceiveHazardReport(string str) {
  string temp_hazlabel;
  string temp_countfor;
  string temp_countagainst;
  int temp_existfor;
  int temp_existagainst;
//  m_combined_classifications = m_classifications;
  for ( int i = 0; i < str.length(); ++i )
  {
      temp_hazlabel += str[i];
      if (temp_hazlabel.length() == 3)
      {
        temp_countfor = str[i+1];
        temp_countagainst = str[i+2];
        i = i+2;
        if (m_classifications.find(atoi(temp_hazlabel.c_str())) == m_classifications.end()) {
          m_classifications[atoi(temp_hazlabel.c_str())] = make_pair(atoi(temp_countfor.c_str()),atoi(temp_countagainst.c_str()));
        } else {
//          temp_existfor = m_classifications[atoi(temp_hazlabel.c_str())].first;
//          temp_existagainst = m_classifications[atoi(temp_hazlabel.c_str())].second;
//          m_classifications[atoi(temp_hazlabel.c_str())] = make_pair(atoi(temp_countfor.c_str())+temp_existfor,atoi(temp_countagainst.c_str())+temp_existagainst);
            m_classifications[atoi(temp_hazlabel.c_str())] = make_pair(atoi(temp_countfor.c_str()),atoi(temp_countagainst.c_str()));
        }
//        cout << "LABEL: " << temp_hazlabel << " FOR:" << temp_countfor << " AGAINST:" << temp_countagainst << endl;
        temp_hazlabel = "";
        temp_countfor = "";
        temp_countagainst = "";
      }
  }

  for (m_class_iter = m_classifications.begin(); m_class_iter != m_classifications.end(); ++m_class_iter) {
    cout << "COMBINED KEY: " << m_class_iter->first << " VOTES: for=" << m_class_iter->second.first << " against=" << m_class_iter->second.second << endl;
  }
  cout << endl;
}

void HazardMgr::handleSendHazardReport() {
  string share_message = "";
//  for (m_class_iter = m_classifications.begin(); m_class_iter != m_classifications.end(); ++m_class_iter) {
//    cout << "KEY: " << m_class_iter->first << " VOTES: for=" << m_class_iter->second.first << " against=" << m_class_iter->second.second << endl;
  for(unsigned int i = 0; i < m_updates.size(); i++) {
//    cout << "KEY: " << m_updates[i] << " VOTES: for=" << m_classifications[m_updates[i]].first << " against=" << m_classifications[m_updates[i]].second << endl;
    stringstream m_message;
    m_message << m_updates[i];
    if (m_message.str().size() == 1) {
      m_message.str("");
      m_message.clear();
      m_message << "00" << m_updates[i];
    } else if (m_message.str().size() == 2) {
      m_message.str("");
      m_message.clear();
      m_message << "0" << m_updates[i];
    }
    m_message << m_classifications[m_updates[i]].first << m_classifications[m_updates[i]].second;
    share_message += m_message.str();
  }
  NodeMessage node_message;
  node_message.setSourceNode(m_host_community);
  node_message.setDestNode("all");
  node_message.setVarName("SHARED_DATA");
  node_message.setStringVal(share_message);
  string msg = node_message.getSpec();
  if (share_message.size()>0)
    Notify("NODE_MESSAGE_LOCAL", msg);
  m_updates.clear();
}

bool HazardMgr::handleMailHazardReport(string str) {
  m_hazard_reports++;

  XYHazard new_hazard = string2Hazard(str);
  string hazlabel = new_hazard.getLabel();
  string haztype = new_hazard.getType();
  if (m_classifications.find(atoi(hazlabel.c_str())) == m_classifications.end()) {
    m_classifications[atoi(hazlabel.c_str())] = make_pair(0,0);
    if (m_many_hazard_mode) {
      if (haztype == "hazard") {
        m_classifications[atoi(hazlabel.c_str())].first++;
      } else {
        m_classifications[atoi(hazlabel.c_str())].second++;
      }
    }
  }
  if (m_many_hazard_mode) {
    if (haztype == "benign") {
        m_classifications[atoi(hazlabel.c_str())].first--;
        m_classifications[atoi(hazlabel.c_str())].second++;
    }
  } else {
    if (haztype == "hazard") {
      m_classifications[atoi(hazlabel.c_str())].first++;
    } else {
      m_classifications[atoi(hazlabel.c_str())].second++;
    }
  }

  if(std::find(m_updates.begin(), m_updates.end(), atoi(hazlabel.c_str())) != m_updates.end()) {
      return(true);
  } else {
      m_updates.push_back(atoi(hazlabel.c_str()));
  }

//  for(int i=0; i < m_updates.size(); i++){
//   cout << "A: ";
//   cout << m_updates[i] << endl;
//  }

//  for (m_class_iter = m_classifications.begin(); m_class_iter != m_classifications.end(); ++m_class_iter) {
//    cout << "KEY: " << m_class_iter->first << " VOTES: for=" << m_class_iter->second.first << " against=" << m_class_iter->second.second << endl;
//  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool HazardMgr::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool HazardMgr::Iterate()
{
  AppCastingMOOSApp::Iterate();

  if (m_deployed) {
    m_total_time = MOOSTime() - m_deploy_time;
    m_search_time = MOOSTime() - m_rep_time;
    if (m_many_hazard_reset_time) {
      m_many_hazard_start_time = MOOSTime();
      m_many_hazard_reset_time = false;
      if (m_many_hazard_count >= 10) {
        m_many_hazard_mode = true;
        cout << "SWITCHING ON MANY HAZARD MODE!!!" << endl;
      } else {
        m_many_hazard_mode = false;
        cout << "SWITCHING OFF MANY HAZARD MODE..." << endl;
      }
      m_many_hazard_count = 0;
    } else if ((MOOSTime() - m_many_hazard_start_time) >= 200) {
      m_many_hazard_reset_time = true;
    }
  }

  if (m_max_time-m_total_time < 100) {
    Notify("STOP_VEHICLE", "true");
    if (m_report_count < 2) {
      handleMailReportRequest();
      m_report_count++;
    }
  }

  if(!m_sensor_config_requested)
    postSensorConfigRequest();

  if(m_sensor_config_set)
    postSensorInfoRequest();

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool HazardMgr::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(true);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if((param == "swath_width") && isNumber(value)) {
      m_swath_width_desired = atof(value.c_str());
      handled = true;
    }
    else if(((param == "sensor_pd") || (param == "pd")) && isNumber(value)) {
      m_pd_desired = atof(value.c_str());
      handled = true;
    }
    else if(param == "report_name") {
      value = stripQuotes(value);
      m_report_name = value;
      handled = true;
    }
    else if(param == "region") {
      XYPolygon poly = string2Poly(value);
      if(poly.is_convex())
	m_search_region = poly;
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  m_hazard_set.setSource(m_host_community);
  m_hazard_set.setName(m_report_name);
  m_hazard_set.setRegion(m_search_region);

  m_voted_hazard_set.setSource(m_host_community);
  m_voted_hazard_set.setName(m_report_name);
  m_voted_hazard_set.setRegion(m_search_region);

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void HazardMgr::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  m_Comms.Register("UHZ_DETECTION_REPORT", 0);
  m_Comms.Register("UHZ_CONFIG_ACK", 0);
  m_Comms.Register("UHZ_OPTIONS_SUMMARY", 0);
  m_Comms.Register("UHZ_MISSION_PARAMS", 0);
  m_Comms.Register("HAZARDSET_REQUEST", 0);
  m_Comms.Register("HAZARDREPORT_OTHER", 0);
  m_Comms.Register("SEARCH_REPS", 0);
  m_Comms.Register("UHZ_HAZARD_REPORT", 0);
  m_Comms.Register("CLEAR_FOR_SHARE", 0);
  m_Comms.Register("SHARED_DATA", 0);
  m_Comms.Register("DEPLOY", 0);
  m_Comms.Register("SEARCH_TIME", 0);
}

//---------------------------------------------------------
// Procedure: postSensorConfigRequest

void HazardMgr::postSensorConfigRequest()
{
  string request = "vname=" + m_host_community;

  request += ",width=" + doubleToStringX(m_swath_width_desired,2);
  request += ",pd="    + doubleToStringX(m_pd_desired,2);

  m_sensor_config_requested = true;
  m_sensor_config_reqs++;
  Notify("UHZ_CONFIG_REQUEST", request);
}

//---------------------------------------------------------
// Procedure: postSensorInfoRequest

void HazardMgr::postSensorInfoRequest()
{
  string request = "vname=" + m_host_community;

  m_sensor_report_reqs++;
  Notify("UHZ_SENSOR_REQUEST", request);
}

//---------------------------------------------------------
// Procedure: handleMailSensorConfigAck

bool HazardMgr::handleMailSensorConfigAck(string str)
{
  // Expected ack parameters:
  string vname, width, pd, pfa, pclass;

  // Parse and handle ack message components
  bool   valid_msg = true;
  string original_msg = str;

  vector<string> svector = parseString(str, ',');
  unsigned int i, vsize = svector.size();
  for(i=0; i<vsize; i++) {
    string param = biteStringX(svector[i], '=');
    string value = svector[i];

    if(param == "vname")
      vname = value;
    else if(param == "pd")
      pd = value;
    else if(param == "width")
      width = value;
    else if(param == "pfa")
      pfa = value;
    else if(param == "pclass")
      pclass = value;
    else
      valid_msg = false;

  }


  if((vname=="")||(width=="")||(pd=="")||(pfa=="")||(pclass==""))
    valid_msg = false;

  if(!valid_msg)
    reportRunWarning("Unhandled Sensor Config Ack:" + original_msg);


  if(valid_msg) {
    m_sensor_config_set = true;
    m_sensor_config_acks++;
    m_swath_width_granted = atof(width.c_str());
    m_pd_granted = atof(pd.c_str());
  }

  return(valid_msg);
}

//---------------------------------------------------------
// Procedure: handleMailDetectionReport
//      Note: The detection report should look something like:
//            UHZ_DETECTION_REPORT = vname=betty,x=51,y=11.3,label=12

bool HazardMgr::handleMailDetectionReport(string str)
{
  m_detection_reports++;
  m_many_hazard_count++;

  XYHazard new_hazard = string2Hazard(str);
  new_hazard.setType("hazard");

  string hazlabel = new_hazard.getLabel();

  if(hazlabel == "") {
    reportRunWarning("Detection report received for hazard w/out label");
    return(false);
  }

  int ix = m_hazard_set.findHazard(hazlabel);
  if(ix == -1)
    m_hazard_set.addHazard(new_hazard);
  else
    m_hazard_set.setHazard(ix, new_hazard);

  string event = "New Detection, label=" + new_hazard.getLabel();
  event += ", x=" + doubleToString(new_hazard.getX(),1);
  event += ", y=" + doubleToString(new_hazard.getY(),1);

  reportEvent(event);

  string req = "vname=" + m_host_community + ",label=" + hazlabel;

  Notify("UHZ_CLASSIFY_REQUEST", req);

  if (m_many_hazard_mode) {
    /******************************************************************************/
    if (m_classifications.find(atoi(hazlabel.c_str())) == m_classifications.end()) {
      m_classifications[atoi(hazlabel.c_str())] = make_pair(0,0);
    }
    m_classifications[atoi(hazlabel.c_str())].first++;

    if(std::find(m_updates.begin(), m_updates.end(), atoi(hazlabel.c_str())) != m_updates.end()) {
        return(true);
    } else {
        m_updates.push_back(atoi(hazlabel.c_str()));
    }
    /******************************************************************************/
  }

  return(true);
}


//---------------------------------------------------------
// Procedure: handleMailReportRequest

void HazardMgr::handleMailReportRequest()
{
  m_summary_reports++;

  stringstream label;
  double votes_for;
  double votes_against;
  double num_reps;
  int num_detects;
  if (m_traverse_time > 0) {
    num_reps = m_repeats+(m_search_time/m_traverse_time);
  } else {
    num_reps = 1;
  }
  for (m_class_iter = m_classifications.begin(); m_class_iter != m_classifications.end(); ++m_class_iter) {
    label << m_class_iter->first;
    num_detects = (m_class_iter->second.first + m_class_iter->second.second);
    votes_for = num_detects + m_class_iter->second.first;
    if (num_reps > num_detects) {
      votes_against = num_reps - num_detects + m_class_iter->second.second;
    } else {
      votes_against = m_class_iter->second.second;
    }

    if (votes_for >= m_vote_multiplier*votes_against) {
      XYHazard new_hazard = string2Hazard("label="+label.str()+",type=hazard");
      string hazlabel = new_hazard.getLabel();
      int ix = m_voted_hazard_set.findHazard(hazlabel);
      if(ix == -1)
        m_voted_hazard_set.addHazard(new_hazard);
      else
        m_voted_hazard_set.setHazard(ix, new_hazard);
    }

    string summary_report = m_voted_hazard_set.getSpec("final_report");
    Notify("HAZARDSET_REPORT", summary_report);

    cout << "COMBINED KEY: " << m_class_iter->first << " VOTES: for=" << m_class_iter->second.first << " against=" << m_class_iter->second.second << endl;
    cout << "COMBINED KEY CALC: " << m_class_iter->first << " VOTES: for=" << votes_for << " against=" << votes_against << endl;
    label.str("");
    label.clear();
  }

//  m_hazard_set.findMinXPath(20);
//  //unsigned int count    = m_hazard_set.findMinXPath(20);
//  string summary_report = m_hazard_set.getSpec("final_report");
//
//  Notify("HAZARDSET_REPORT", summary_report);
//
//  NodeMessage node_message;
//
//  node_message.setSourceNode(m_host_community);
//  node_message.setDestNode("all");
//  node_message.setVarName("HAZARDREPORT_OTHER");
//  node_message.setStringVal(summary_report);
//
//  string msg = node_message.getSpec();
//
//  Notify("NODE_MESSAGE_LOCAL", msg);
}


//---------------------------------------------------------
// Procedure: handleMailMissionParams
//   Example: UHZ_MISSION_PARAMS = penalty_missed_hazard=100,
//                       penalty_nonopt_hazard=55,
//                       penalty_false_alarm=35,
//                       penalty_max_time_over=200,
//                       penalty_max_time_rate=0.45,
//                       transit_path_width=25,
//                       search_region = pts={-150,-75:-150,-50:40,-50:40,-75}


void HazardMgr::handleMailMissionParams(string str)
{
  vector<string> svector = parseStringZ(str, ',', "{");
  unsigned int i, vsize = svector.size();
  for(i=0; i<vsize; i++) {
    string param = biteStringX(svector[i], '=');
    string value = svector[i];
    if (param=="max_time") {
      m_max_time = atof(value.c_str());
    } else if (param=="penalty_false_alarm") {
      m_pen_false_alarm = atof(value.c_str());
    } else if (param=="penalty_missed_hazard") {
      m_pen_missed_hazard = atof(value.c_str());
    }
    if (m_max_time <= 6000) {
      if (m_host_community == "jake") {
        Notify("SURVEY1_UPDATES", "points = format=lawnmower,label=kaspersearch,x=125,y=-252.5,height=255,width=550,lane_width=70,rows=ew,degs=0");
        Notify("SURVEY2_UPDATES", "points = format=lawnmower,label=kaspersearch,x=125,y=-217.5,height=255,width=550,lane_width=70,rows=ew,degs=0");
      } else {
        m_swath_width_desired = 25;
        //m_pd_desired = 0.525;
        //m_pd_desired = 0.56;
        m_pd_desired = 0.59;
        postSensorConfigRequest();
        Notify("SURVEY1_UPDATES", "points = format=lawnmower,label=kaspersearch,x=125,y=-217.5,height=255,width=550,lane_width=70,rows=ew,degs=0");
        Notify("SURVEY2_UPDATES", "points = format=lawnmower,label=kaspersearch,x=125,y=-252.5,height=255,width=550,lane_width=70,rows=ew,degs=0");
      }
    }
//    if (m_pen_false_alarm < m_pen_missed_hazard) {
//      m_vote_multiplier = 0.5;
//    } else {
//      m_vote_multiplier = 1.0;
//    }
    cout << "VOTE MULTIPLIER: " << m_vote_multiplier << endl;
    // This needs to be handled by the developer. Just a placeholder.
  }
}


//------------------------------------------------------------
// Procedure: buildReport()

//bool HazardMgr::buildReport()
//{
//  m_msgs << "Config Requested:"                                  << endl;
//  m_msgs << "    swath_width_desired: " << m_swath_width_desired << endl;
//  m_msgs << "             pd_desired: " << m_pd_desired          << endl;
//  m_msgs << "   config requests sent: " << m_sensor_config_reqs  << endl;
//  m_msgs << "                  acked: " << m_sensor_config_acks  << endl;
//  m_msgs << "------------------------ "                          << endl;
//  m_msgs << "Config Result:"                                     << endl;
//  m_msgs << "       config confirmed: " << boolToString(m_sensor_config_set) << endl;
//  m_msgs << "    swath_width_granted: " << m_swath_width_granted << endl;
//  m_msgs << "             pd_granted: " << m_pd_granted          << endl << endl;
//  m_msgs << "--------------------------------------------" << endl << endl;
//
//  m_msgs << "               sensor requests: " << m_sensor_report_reqs << endl;
//  m_msgs << "             detection reports: " << m_detection_reports  << endl << endl;
//
//  m_msgs << "   Hazardset Reports Requested: " << m_summary_reports << endl;
//  m_msgs << "      Hazardset Reports Posted: " << m_summary_reports << endl;
//  m_msgs << "                   Report Name: " << m_report_name << endl;
//
//  return(true);
//}





