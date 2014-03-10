/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PointAssign.cpp                                 */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "PointAssign.h"

using namespace std;

//---------------------------------------------------------
// Constructor

PointAssign::PointAssign() : m_pts_assigned(false)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
  stringstream publishString;
  string visit_point;
  double left_x;
  double right_x;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    //get each point published, and assign corresponding point to correct vehicle
    if (MOOSStrCmp(msg.GetKey(), "VISIT_POINT")) {
      visit_point = msg.GetString();

      //if it is the first point message, forward it to each vehicle
      if (MOOSStrCmp(visit_point, "firstpoint")) {
        for (unsigned int i=0; i<m_veh_vector.size(); i++) {
          publishString.str("");
          publishString.clear();
          publishString << "VISIT_POINT_" << m_veh_vector[i];
          m_Comms.Notify(publishString.str(), "firstpoint");
          m_veh_counter = 0;
        }
      //if it is the last point message, forward it to each vehicle and flag that all points have been assigned
      } else if (MOOSStrCmp(visit_point, "lastpoint")) {
        for (unsigned int i=0; i<m_veh_vector.size(); i++) {
          publishString.str("");
          publishString.clear();
          publishString << "VISIT_POINT_" << m_veh_vector[i];
          m_Comms.Notify(publishString.str(), "lastpoint");
          m_pts_assigned = true;
        }
      //otherwise, check if we assign points by region or by alternating, and send the points accordingly
      } else {
        publishString.str("");
        publishString.clear();
        if (!m_region_assign) {
          publishString << "VISIT_POINT_" << m_veh_vector[m_veh_counter];
          m_Comms.Notify(publishString.str(), visit_point);
          cout << publishString.str() << " | " << visit_point << endl;
          if (m_veh_counter < m_num_vehicles-1) {
            m_veh_counter++;
          } else {
            m_veh_counter = 0;
          }
        } else {
          m_x_pos = atof(tokStringParse(visit_point, "x", ',', '=').c_str());
          m_delta_x = (m_region[1]-m_region[0])/m_num_vehicles;
          for (unsigned int i=0; i<m_num_vehicles; i++) {
            left_x = m_region[0] + i*m_delta_x;
            right_x = m_region[0] + (i+1)*m_delta_x;
            if ((m_x_pos > left_x) and (m_x_pos < right_x)) {
              publishString << "VISIT_POINT_" << m_veh_vector[i];
              m_Comms.Notify(publishString.str(), visit_point);
              cout << publishString.str() << " | " << visit_point << endl;
            }
          }
        }
      }
    }

#if 0 // Keep these around just for template
    string key   = msg.GetKey();
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool PointAssign::OnConnectToServer()
{
   // register for variables here
   // possibly look at the mission file?
   // m_MissionReader.GetConfigurationParam("Name", <string>);
   // m_Comms.Register("VARNAME", 0);

   RegisterVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PointAssign::Iterate()
{
  m_iterations++;
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PointAssign::OnStartUp()
{
  list<string> sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
    list<string>::iterator p;
    for(p=sParams.begin(); p!=sParams.end(); p++) {
      string original_line = *p;
      string param = stripBlankEnds(toupper(biteString(*p, '=')));
      string value = stripBlankEnds(*p);

      if(param == "FOO") {
        //handled
      }
      else if(param == "BAR") {
        //handled
      }
    }
  }

  m_timewarp = GetMOOSTimeWarp();

  //get all vehicles and their names (needed to publish correct points to each vehicle)
  if (!m_MissionReader.GetConfigurationParam("vname", m_veh_list)) {
    cerr << "No vehicle names specified! Quitting!" << endl;
    return(false);
  }

  m_veh_vector = parseString(m_veh_list, ',');
  for(unsigned int i=0; i<m_veh_vector.size(); i++) m_veh_vector[i] = stripBlankEnds(m_veh_vector[i]);
  m_num_vehicles = m_veh_vector.size();

  //get the region in which the points can exist
  if (!m_MissionReader.GetConfigurationParam("region", m_region_str)) {
    cerr << "No region specified! Quitting!" << endl;
    return(false);
  }

  vector<string> m_region_vec = parseString(m_region_str, ',');
  for(unsigned int i=0; i<m_region_vec.size(); i++) m_region.push_back(atof(stripBlankEnds(m_region_vec[i]).c_str()));

  //get the check if we should assign points by region
  string region_assign;
  if (!m_MissionReader.GetConfigurationParam("assign_by_region", region_assign)) {
    cerr << "No region sorting specified! Quitting!" << endl;
    return(false);
  }

  if (tolower(region_assign.c_str()[0]) == 't') {
    m_region_assign = true;
  } else {
    m_region_assign = false;
  }

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void PointAssign::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("VISIT_POINT", 0);
}

