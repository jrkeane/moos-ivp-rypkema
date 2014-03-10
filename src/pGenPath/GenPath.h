/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: GenPath.h                                       */
/*    DATE:                                                 */
/************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include "GeomUtils.h"
#include "XYSegList.h"

class GenPath : public CMOOSApp
{
 public:
   GenPath();
   ~GenPath();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();

 private: // Configuration variables
   std::vector<double> m_x_pts;         //vectors of received assigned points (x,y,and id)
   std::vector<double> m_y_pts;
   std::vector<int> m_id_pts;
   bool m_got_all_pts;                  //flag to check that all points received
   bool m_assigned_pts;                 //flag to check that all points have been assigned to a path (greedy)
   bool m_received_pos;                 //flag to check that NAV info has been received
   unsigned int m_num_pts;              //number of received points
   unsigned int m_pt_counter;           //point counter to loop through all points during assignment
   XYSegList m_seglist;                 //seglist for publishing updated paths
   double m_x_pos;                      //veh x pos
   double m_y_pos;                      //veh y pos
   std::vector<double> m_x_pts_ordered; //vector of points ordered as a path (x,y,and id)
   std::vector<double> m_y_pts_ordered;
   std::vector<int> m_id_pts_ordered;
   std::string m_update_var;            //string that is the update variable for waypoint bhv
   std::vector<double> m_x_pts_missed;  //vector of points missed during a tour (x,y,and id)
   std::vector<double> m_y_pts_missed;
   std::vector<int> m_id_pts_missed;
   unsigned int m_wpt_idx;              //index published by waypoint bhv, to check for missed points
   unsigned int m_visit_radius;         //our custom radius to certify visitation
   double m_wpt_dist;                   //continous calculation of distance to current wpt
   bool m_received_wpt_idx;             //flag to check that wpt behaviour has started
   bool m_regen_path;                   //flag to regenerate path for missed points

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
