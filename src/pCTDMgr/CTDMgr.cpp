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
#include "NodeMessage.h"
#include "GeomUtils.h"
#include "AngleUtils.h"

using namespace std;

//---------------------------------------------------------
// Constructor

CTDMgr::CTDMgr() : m_prev_averaged_temp(-1.0), m_reverse(false), m_off(false), m_out_2_in(false), leave_count(1), m_got_point(false), m_calced_point(false), m_share_complete(false), m_prev_time(0.0)
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
      string x_val = tokStringParse(sval, "x", ',', '=');
      string y_val = tokStringParse(sval, "y", ',', '=');
      string time_val = tokStringParse(sval, "utc", ',', '=');
      double dbl_x = atof(x_val.c_str());
      double dbl_y = atof(y_val.c_str());
      double dbl_time = atof(time_val.c_str());
      m_temperature = atof(temperature.c_str());
      m_temps.push_back(m_temperature);
      m_x_pos.push_back(dbl_x);
      m_y_pos.push_back(dbl_y);
      m_times.push_back(dbl_time);
    } else if(key == "NAV_X") {
      m_x = msg.GetDouble();
    } else if(key == "NAV_Y") {
      m_y = msg.GetDouble();
    } else if(key == "GET_POINT") {
      if (sval == "true") {
        double max_temp, min_temp;
        double avg_temp;
        int idx;
        min_temp = *min_element(m_temps.begin(), m_temps.end());
        max_temp = *max_element(m_temps.begin(), m_temps.end());
        avg_temp = min_temp+(max_temp-min_temp)/2;
        std::cout << "AVERAGE TEMP: " << avg_temp << std::endl;
        std::vector<double>::iterator it = std::lower_bound(m_temps.begin(), m_temps.end(), avg_temp);
        idx = it - m_temps.begin();
        std::cout << idx << std::endl;
        std::cout << "AVERAGES: " << m_temps[idx] << ", " << m_x_pos[idx] << ", " << m_y_pos[idx] << ", " << m_times[idx] << std::endl;
        stringstream viewpoint;
        viewpoint << "x=" << m_x_pos[idx] << ",y=" << m_y_pos[idx];
        m_Comms.Notify("VIEW_POINT", viewpoint.str());
        m_bnd_x_1 = m_x_pos[idx];
        m_bnd_y_1 = m_y_pos[idx];
        string share_message = viewpoint.str();
        NodeMessage node_message;
        node_message.setSourceNode(m_host_community);
        node_message.setDestNode("all");
        node_message.setVarName("GET_POINT_SHARED");
        node_message.setStringVal(share_message);
        string msg = node_message.getSpec();
        Notify("NODE_MESSAGE_LOCAL", msg);
        m_calced_point = true;
      }
    } else if(key == "GET_POINT_SHARED") {
      std::cout << "SHARED POINT: " << sval << std::endl;
      m_bnd_x_2 = atof(tokStringParse(sval, "x", ',', '=').c_str());
      m_bnd_y_2 = atof(tokStringParse(sval, "y", ',', '=').c_str());
      m_got_point = true;
    } else if(key == "CLEAR_FOR_SHARE") {
      if (m_calced_point && !m_got_point) {
        //still havent received other point, lets continually send ours
        stringstream viewpoint;
        viewpoint << "x=" << m_bnd_x_1 << ",y=" << m_bnd_y_1;
        string share_message = viewpoint.str();
        NodeMessage node_message;
        node_message.setSourceNode(m_host_community);
        node_message.setDestNode("all");
        node_message.setVarName("GET_POINT_SHARED");
        node_message.setStringVal(share_message);
        string msg = node_message.getSpec();
        Notify("NODE_MESSAGE_LOCAL", msg);
      } else if (m_share_complete) {
        int start_idx = m_temps.size()-75;
        double first_time = m_times[start_idx];
        stringstream temp_message;
        temp_message << fixed << setprecision(1) << first_time << setprecision(2);
        temp_message << "t" << setw(5) << setfill('0') << m_temps[start_idx];
        temp_message << "x" << setw(5) << setfill('0') << m_x_pos[start_idx];
        temp_message << "y" << setw(5) << setfill('0') << m_y_pos[start_idx];
        for (int i=start_idx+1; i < m_temps.size(); i++) {
          double diff_time = m_times[i] - first_time;
          temp_message << "u" << setw(5) << setfill('0') << diff_time;
          temp_message << "t" << setw(5) << setfill('0') << m_temps[i];
          temp_message << "x" << setw(5) << setfill('0') << m_x_pos[i];
          temp_message << "y" << setw(5) << setfill('0') << m_y_pos[i];
        }
        string share_message = temp_message.str();
        NodeMessage node_message;
        node_message.setSourceNode(m_host_community);
        node_message.setDestNode("all");
        node_message.setVarName("GET_TEMPS_SHARED");
        node_message.setStringVal(share_message);
        string msg = node_message.getSpec();
        Notify("NODE_MESSAGE_LOCAL", msg);
      }
    } else if(key == "GET_TEMPS_SHARED") {
      double first_time;
      double utc;
      double temp;
      double x;
      double y;
      stringstream temp_message;
      vector<string> str_vector = parseString(sval, 'u');
      for(unsigned int i=0; i<str_vector.size(); i++) {
        string str = str_vector[i];
        std::vector<std::string> v;
        std::size_t prev_pos = 0, pos;
        while ((pos = str.find_first_of("txy", prev_pos)) != std::string::npos)
        {
            if (pos > prev_pos)
                v.push_back(str.substr(prev_pos, pos-prev_pos));
            prev_pos= pos+1;
        }
        if (prev_pos< str.length())
            v.push_back(str.substr(prev_pos, std::string::npos));

        if (i==0) {
          first_time = atof(v[0].c_str());
          utc = first_time;
          temp = atof(v[1].c_str());
          x = atof(v[2].c_str());
          y = atof(v[3].c_str());
        } else {
          utc = first_time + atof(v[0].c_str());
          temp = atof(v[1].c_str());
          x = atof(v[2].c_str());
          y = atof(v[3].c_str());
        }

        temp_message << m_host_community << fixed << ",utc=" << utc << ",x=" << x << ",y=" << y << ",temp=" << temp;
        Notify("UCTD_MSMNT_REPORT_SHARED", temp_message.str());

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

  if (m_calced_point && m_got_point && !m_share_complete) {
    //wait until we receive other point
    std::cout << m_bnd_x_2 << ":" << m_bnd_y_2 << std::endl;
    std::cout << m_bnd_x_1 << ":" << m_bnd_y_1 << std::endl;
    stringstream update;
    stringstream update_str;
    update << "update_search_" << m_host_community;
    //format=lawnmower, label=foxtrot, x=0, y=40, height=60, width=180,lane_width=15, rows=north-south, degs=45
    double center_x, center_y;
    center_x = m_bnd_x_1 + (m_bnd_x_2-m_bnd_x_1)/2;
    center_y = m_bnd_y_1 + (m_bnd_y_2-m_bnd_y_1)/2;
    if (m_reverse) {
      center_y -= 2;
    }
    double angle = radToHeading(atan2(m_bnd_y_1 - m_bnd_y_2, m_bnd_x_1 - m_bnd_x_2)+PI/2);
    double length = sqrt(pow(m_bnd_y_1 - m_bnd_y_2,2) + pow(m_bnd_x_1 - m_bnd_x_2,2))-60;
    update_str << "points=format=lawnmower,x=" << center_x << ",y=" << center_y << ",height=50,width=" << length << ",lane_width=35,rows=north-south,degs=" << angle;
    Notify(update.str(), update_str.str());
    Notify("SURVEY", "true");
    Notify("STATION", "false");
    stringstream viewseglist;
    viewseglist << "pts={" << m_bnd_x_1 << "," << m_bnd_y_1 << ":" << m_bnd_x_2 << "," << m_bnd_y_2;
    viewseglist << "},label=estimate,edge_size=2";
    Notify("VIEW_SEGLIST", viewseglist.str());
    m_share_complete = true;
  }

//  for(unsigned int i = 0; i < m_x_pos.size(); i++) {
//    stringstream viewpoint;
//    viewpoint << "x=" << m_x_pos[i] << ",y=" << m_y_pos[i] << ",label=" << i << ",vertex_size=10";
//    m_Comms.Notify("VIEW_POINT", viewpoint.str());
//  }

//  if (m_x_pos.size() > 2) {
//    CalculateBestFitLine();
//  }

  if(!m_polygon.contains(m_x, m_y) && (m_out_2_in)) {
    cout << "OUTSIDE OF OPERATING REGION" << endl;
//    m_off = true;
//    m_out_2_in = false;
//    m_reverse = true;
//    if (leave_count >= 1) {
//      m_Comms.Notify("SPIRALCCW", "false");
//      m_Comms.Notify("SPIRALCW", "false");
//      m_Comms.Notify("SEARCH_FIRST", "false");
//      m_Comms.Notify("SEARCH_SECOND", "false");
//      m_Comms.Notify("RETURN", "true");
//      m_Comms.Notify("SURVEY_UNDERWAY", "false");
//      m_off = true;
//      m_out_2_in = true;
//      //CalculateBestFitLine();
//    } else {
//      m_Comms.Notify("SPIRALCCW", "false");
//      m_Comms.Notify("SPIRALCW", "false");
//      m_Comms.Notify("SEARCH_FIRST", "false");
//      m_Comms.Notify("SEARCH_SECOND", "true");
//    }
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
  m_Comms.Register("GET_POINT", 0);
  m_Comms.Register("GET_POINT_SHARED", 0);
  m_Comms.Register("GET_TEMPS_SHARED", 0);
  m_Comms.Register("CLEAR_FOR_SHARE", 0);
}

