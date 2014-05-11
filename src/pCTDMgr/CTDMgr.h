/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: CTDMgr.h                                          */
/*    DATE:                                                 */
/************************************************************/

#ifndef CTDMgr_HEADER
#define CTDMgr_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include <deque>
#include "XYFormatUtilsPoly.h"

class CTDMgr : public CMOOSApp
{
 public:
   CTDMgr();
   ~CTDMgr();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();
   void CalculateBestFitLine();

 private: // Configuration variables

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
   double m_temperature;
   double m_desired_val;
   int m_num_pts_avg;
   double m_averaged_temp;
   double m_prev_averaged_temp;
   std::deque<double> m_points;
   std::vector<double> m_temps;
   std::vector<double> m_x_pos;
   std::vector<double> m_y_pos;
   std::vector<double> m_times;
   std::string m_host_community;
   bool m_reverse;
   std::string m_string_polygon;
   XYPolygon m_polygon;
   double m_x;
   double m_y;
   bool m_off;
   bool m_out_2_in;
   int leave_count;
   double m_bnd_x_1, m_bnd_y_1, m_bnd_x_2, m_bnd_y_2;
   bool m_got_point, m_calced_point, m_share_complete;
   double m_prev_time;
};

#endif
