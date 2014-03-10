/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PointAssign.h                                   */
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
  std::string m_veh_list;                 //list of vehicles from config
  std::vector<std::string> m_veh_vector;  //vector of vehicle names
  int m_num_vehicles;                     //number of vehicles (length of vector)
  std::string m_region_str;               //string specifying region
  std::vector<double> m_region;           //xmin,xmax,ymin,ymax region vector
  double m_x_pos;                         //xpos of point (for assigning to left or right region)
  double m_delta_x;                       //division of region into equal for each vehicle
  bool m_region_assign;                   //bool to check if we should assign veh by region
  bool m_pts_assigned;                    //bool to check if all points have been assigned to a veh
  unsigned int m_veh_counter;             //counter to assign alternating points to vehicles

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
