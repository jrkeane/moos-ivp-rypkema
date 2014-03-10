/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: GenPath.cpp                                     */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "GenPath.h"
#include <math.h>

using namespace std;

//---------------------------------------------------------
// Constructor

GenPath::GenPath() : m_got_all_pts(false), m_assigned_pts(false), m_received_pos(false), m_received_wpt_idx(false), m_wpt_idx(0), m_regen_path(false)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

GenPath::~GenPath()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
  string visit_point;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "VISIT_POINT")) {
      visit_point = msg.GetString();

      //if its the firstpoint flag, init variables by clearing
      if (MOOSStrCmp(visit_point, "firstpoint")) {
        m_x_pts.clear();
        m_y_pts.clear();
        m_got_all_pts = false;
        m_assigned_pts = false;
        m_pt_counter = 0;
      //if its the laspoint flag, flag that we have received all points so that we can calculate tour
      } else if (MOOSStrCmp(visit_point, "lastpoint")) {
        for (unsigned int i=0; i<m_x_pts.size(); i++) {
          cout << m_x_pts[i] << "," << m_y_pts[i] << "," << m_id_pts[i] << endl;
        }
        m_got_all_pts = true;
        m_num_pts = m_x_pts.size();
      //otherwise, parse the point string and push it onto our position vectors
      } else {
        m_x_pts.push_back(atof(tokStringParse(visit_point, "x", ',', '=').c_str()));
        m_y_pts.push_back(atof(tokStringParse(visit_point, "y", ',', '=').c_str()));
        m_id_pts.push_back(atoi(tokStringParse(visit_point, "id", ',', '=').c_str()));
      }
    //get x pos
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_X")) {
      m_x_pos = msg.GetDouble();
      m_received_pos = true;
    //get y pos
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_Y")) {
      m_y_pos = msg.GetDouble();
      m_received_pos = true;
    //get the current waypoint of the path from the waypoint bhv
    } else if (MOOSStrCmp(msg.GetKey(), "WPT_INDEX")) {
      m_received_wpt_idx = true;
      unsigned int new_wpt = (unsigned int)msg.GetDouble();
      if (new_wpt > m_wpt_idx) {
        m_wpt_idx = new_wpt;
      }
      cout << m_wpt_idx << endl;
      if (m_wpt_idx > 0) {
        cout << "Distance to prev point: " << m_wpt_dist << endl;
        cout << m_x_pts_missed.size() << endl;
        //perform a check to see if we actually visited the previous point
        if (m_wpt_dist > m_visit_radius) {
          m_x_pts_missed.push_back(m_x_pts_ordered[m_wpt_idx-1]);
          m_y_pts_missed.push_back(m_y_pts_ordered[m_wpt_idx-1]);
          m_id_pts_missed.push_back(m_id_pts_ordered[m_wpt_idx-1]);
        }
      }
    //bhv may request a regeneration of the tour for any missed points
    } else if (MOOSStrCmp(msg.GetKey(), "GENPATH_REGENERATE")) {
      m_regen_path = true;
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

bool GenPath::OnConnectToServer()
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

bool GenPath::Iterate()
{
  m_iterations++;

  //we have received all points, we havent assigned them, and we have vehicle nav info
  if ((m_got_all_pts) and (!m_assigned_pts) and (m_received_pos)) {
    double curr_x = m_x_pos;
    double curr_y = m_y_pos;
    unsigned int curr_id = 0;
    //loop through all received points, and do a greedy tour of the points (shortest distance from each point to the next) O(n^2)
    while (m_pt_counter < m_num_pts) {
      int shortest_pt_idx = -1;
      double shortest_dist = -1;
      //check remaining points for the closest one
      for (unsigned int i=0; i<m_x_pts.size(); i++) {
        double distance = sqrt(pow(curr_x-m_x_pts[i],2)+pow(curr_y-m_y_pts[i],2));
        cout << i << " " << m_id_pts[i] << ": " << distance << " | " << curr_id << ": " << shortest_dist << endl;
        if ((shortest_pt_idx < 0) or (distance < shortest_dist)) {
          shortest_dist = distance;
          shortest_pt_idx = i;
        }
      }
      //update our current point, and remove it from the list of points that have not been assigned
      curr_x = m_x_pts[shortest_pt_idx];
      curr_y = m_y_pts[shortest_pt_idx];
      curr_id = m_id_pts[shortest_pt_idx];
      m_seglist.add_vertex(curr_x, curr_y);
      m_x_pts_ordered.push_back(curr_x);
      m_y_pts_ordered.push_back(curr_y);
      m_id_pts_ordered.push_back(curr_id);
      m_x_pts.erase(m_x_pts.begin()+shortest_pt_idx);
      m_y_pts.erase(m_y_pts.begin()+shortest_pt_idx);
      m_id_pts.erase(m_id_pts.begin()+shortest_pt_idx);
      m_pt_counter++;
    }
    //sanity check on the point ordering
    cout << "ordered points" << endl;
    for (unsigned int i=0; i<m_x_pts_ordered.size(); i++) {
      cout << m_id_pts_ordered[i] << ": " << m_x_pts_ordered[i] << "," << m_y_pts_ordered[i] << endl;
    }
    //if we have points to visit, publish the tour to the bhv update variable
    if (m_num_pts > 0) {
      m_assigned_pts = true;
      string update_string = "points = ";
      update_string += m_seglist.get_spec();
      m_Comms.Notify(m_update_var, update_string);
      //if we have performed this assignment as a result of a regeneration request, tell vehicle to redeploy on regenerated tour
      if (m_regen_path) {
        m_regen_path = false;
        m_Comms.Notify("DEPLOY", "true");
        m_Comms.Notify("RETURN", "false");
        m_Comms.Notify("STATION_KEEP", "false");
        m_Comms.Notify("FINAL_RETURN", "false");
      }
    }
  }

  //once we have a waypoint of the tour, continually calculate the distance to the current waypoint
  if (m_received_wpt_idx) {
    m_wpt_dist = sqrt(pow(m_x_pos-m_x_pts_ordered[m_wpt_idx],2)+pow(m_y_pos-m_y_pts_ordered[m_wpt_idx],2));
  }

  //if we a regeneration has been requested, clear our calculated path, and regenerate a new one (in the above loop) using the vector of missed points
  if (m_regen_path) {
    m_received_wpt_idx = false;
    m_assigned_pts = false;
    m_wpt_idx = 0;
    m_pt_counter = 0;
    m_num_pts = m_x_pts_missed.size();
    m_x_pts = m_x_pts_missed;
    m_y_pts = m_y_pts_missed;
    m_id_pts = m_id_pts_missed;
    m_x_pts_missed.clear();
    m_y_pts_missed.clear();
    m_id_pts_missed.clear();
    m_x_pts_ordered.clear();
    m_y_pts_ordered.clear();
    m_id_pts_ordered.clear();
    m_seglist.clear();
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GenPath::OnStartUp()
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

  //get the name of the update variable for wpt bhv
  if (!m_MissionReader.GetConfigurationParam("update_var", m_update_var)) {
    cerr << "No update variable specified! Quitting!" << endl;
    return(false);
  }

  //get the radius that certifies visitation of a point
  if (!m_MissionReader.GetConfigurationParam("visit_radius", m_visit_radius)) {
    cerr << "No visit radius specified! Quitting!" << endl;
    return(false);
  }

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void GenPath::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("VISIT_POINT", 0);
  m_Comms.Register("NAV_X", 0);
  m_Comms.Register("NAV_Y", 0);
  m_Comms.Register("WPT_INDEX", 0);
  m_Comms.Register("GENPATH_REGENERATE", 0);
}

