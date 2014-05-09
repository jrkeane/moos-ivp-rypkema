/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: CTDMgr.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "CTDMgr.h"
#include <math.h>

using namespace std;

//---------------------------------------------------------
// Constructor

CTDMgr::CTDMgr() : m_prev_averaged_temp(-1.0), m_reverse(false), m_off(false), m_out_2_in(false), leave_count(1)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

CTDMgr::~CTDMgr()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool CTDMgr::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key   = msg.GetKey();
    string sval  = msg.GetString();

    if(key == "UCTD_MSMNT_REPORT") {
      string temperature = tokStringParse(sval, "temp", ',', '=');
      m_temperature = atof(temperature.c_str());
      if (m_points.size() < m_num_pts_avg) {
        m_points.push_back(m_temperature);
      } else {
        m_points.pop_front();
        m_points.push_back(m_temperature);
      }

      if (m_prev_averaged_temp > 0) {
        m_prev_averaged_temp = m_averaged_temp;
      }
      double sum = 0.0;
      for (std::deque<double>::iterator it = m_points.begin(); it != m_points.end(); ++it)
        sum = sum + *it;
      m_averaged_temp = sum/m_points.size();
      if (m_prev_averaged_temp < 0) {
        m_prev_averaged_temp = m_averaged_temp;
      }
      //cout << "TEMP: " << m_temperature << " PREV AVG TEMP: " << m_prev_averaged_temp << " AVG TEMP: " << m_averaged_temp << endl;

      if ((m_prev_averaged_temp < m_desired_val) && (m_averaged_temp > m_desired_val) && (!m_off)) {
        m_x_pos.push_back(m_x);
        m_y_pos.push_back(m_y);
        if (m_reverse) {
          m_Comms.Notify("SPIRALCW", "false");
          m_Comms.Notify("SPIRALCCW", "true");
          cout << "SPIRAL COUNTER-CLOCKWISE" << endl;
        } else {
          m_Comms.Notify("SPIRALCCW", "false");
          m_Comms.Notify("SPIRALCW", "true");
          cout << "SPIRAL CLOCKWISE" << endl;
        }
      } else if ((m_averaged_temp < m_desired_val) && (m_prev_averaged_temp > m_desired_val) && (!m_off)) {
        m_x_pos.push_back(m_x);
        m_y_pos.push_back(m_y);
        if (m_reverse) {
          m_Comms.Notify("SPIRALCCW", "false");
          m_Comms.Notify("SPIRALCW", "true");
          cout << "SPIRAL CLOCKWISE" << endl;
        } else {
          m_Comms.Notify("SPIRALCW", "false");
          m_Comms.Notify("SPIRALCCW", "true");
          cout << "SPIRAL COUNTER-CLOCKWISE" << endl;
        }
      }
    } else if(key == "NAV_X") {
      m_x = msg.GetDouble();
    } else if(key == "NAV_Y") {
      m_y = msg.GetDouble();
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

bool CTDMgr::OnConnectToServer()
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

bool CTDMgr::Iterate()
{
  m_iterations++;

  m_Comms.Notify("UCTD_SENSOR_REQUEST", "vname="+m_host_community);

  for(unsigned int i = 0; i < m_x_pos.size(); i++) {
    stringstream viewpoint;
    viewpoint << "x=" << m_x_pos[i] << ",y=" << m_y_pos[i] << ",label=" << i << ",vertex_size=10";
    m_Comms.Notify("VIEW_POINT", viewpoint.str());
  }

//  if (m_x_pos.size() > 2) {
//    CalculateBestFitLine();
//  }

  if(!m_polygon.contains(m_x, m_y) && (m_out_2_in)) {
    cout << "OUTSIDE OF OPERATING REGION" << endl;
    m_off = true;
    m_out_2_in = false;
    m_reverse = true;
    if (leave_count >= 1) {
      m_Comms.Notify("SPIRALCCW", "false");
      m_Comms.Notify("SPIRALCW", "false");
      m_Comms.Notify("SEARCH_FIRST", "false");
      m_Comms.Notify("SEARCH_SECOND", "false");
      m_Comms.Notify("RETURN", "true");
      m_Comms.Notify("SURVEY_UNDERWAY", "false");
      m_off = true;
      m_out_2_in = true;
      //CalculateBestFitLine();
    } else {
      m_Comms.Notify("SPIRALCCW", "false");
      m_Comms.Notify("SPIRALCW", "false");
      m_Comms.Notify("SEARCH_FIRST", "false");
      m_Comms.Notify("SEARCH_SECOND", "true");
    }
    leave_count++;
  } else if((m_polygon.contains(m_x, m_y)) && (!m_out_2_in)) {
    cout << "ENTERED OPERATING REGION" << endl;
    m_off = false;
    m_out_2_in = true;
  }

  return(true);
}

void CTDMgr::CalculateBestFitLine() {
  double _count = m_x_pos.size();
  double _sumx = 0;
  double _sumy = 0;
  double _sumxsq = 0;
  double _sumxy = 0;
  for(unsigned int i = 0; i < m_x_pos.size(); i++) {
    _sumx += m_x_pos[i];
    _sumy += m_y_pos[i];
    _sumxsq += pow(m_x_pos[i], 2);
    _sumxy += m_x_pos[i]*m_y_pos[i];
  }
  double _xmean = _sumx/_count;
  double _ymean = _sumy/_count;
  double _slope = (_sumxy - _sumx*_ymean)/(_sumxsq - _sumx*_xmean);
  double _yint = _ymean - _slope*_xmean;
  double x1 = -85;
  double x2 = 170;
  double y1 = _slope * x1 + _yint;
  double y2 = _slope * x2 + _yint;
  stringstream bestfit;
  bestfit << "pts={" << x1 << "," << y1 << ":" << x2 << "," << y2 << "},label=bestfit,edge_size=4";
  //m_Comms.Notify("VIEW_SEGLIST", bestfit.str());
  //m_Comms.Notify("BEST_FIT_UPDATES", bestfit.str());
  //m_Comms.Notify("BEST_FIT", "true");
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool CTDMgr::OnStartUp()
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

  if (!m_MissionReader.GetConfigurationParam("desired_value", m_desired_val)) {
    cerr << "No desired value variable specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetConfigurationParam("num_pts_to_average", m_num_pts_avg)) {
    cerr << "No number of points to average specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetValue("Community", m_host_community)) {
    cerr << "No community specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetConfigurationParam("reverse", m_reverse)) {
    cerr << "No reverse parameter specified! Quitting!" << endl;
    return(false);
  }

  string m_string_polygon;
  if (!m_MissionReader.GetValue("op_region", m_string_polygon)) {
    cerr << "No operating region specified! Quitting!" << endl;
    return false;
  }
  XYPolygon new_poly = string2Poly(m_string_polygon);
  if(!new_poly.is_convex())  // Should be convex - false otherwise
    return(false);
  m_polygon = new_poly;

  XYPolygon poly_duplicate = m_polygon;
  string poly_spec = poly_duplicate.get_spec();
  m_Comms.Notify("VIEW_POLYGON", poly_spec);

  m_timewarp = GetMOOSTimeWarp();

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void CTDMgr::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("UCTD_MSMNT_REPORT", 0);
  m_Comms.Register("NAV_X", 0);
  m_Comms.Register("NAV_Y", 0);
  m_Comms.Register("RETURN_SPIRAL", 0);
}

