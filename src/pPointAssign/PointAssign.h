/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT                                             */
/*    FILE: PointAssign.h                                          */
/*    DATE:                                                 */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include "MOOS/libMOOS/MOOSLib.h"

class PointAssign : public CMOOSApp
{
 public:
   PointAssign();
   ~PointAssign();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();

 private: // Configuration variables
  std::string m_veh_list;
  std::vector<std::string> m_veh_vector;
  int m_num_vehicles;
  std::string m_region_str;
  std::vector<double> m_region;
  double m_x_pos;
  double m_delta_x;
  bool m_region_assign;
  bool m_pts_assigned;
  unsigned int m_veh_counter;

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
