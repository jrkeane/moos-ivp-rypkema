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
   std::vector<double> m_x_pts;
   std::vector<double> m_y_pts;
   std::vector<int> m_id_pts;
   bool m_got_all_pts;
   bool m_assigned_pts;
   bool m_received_pos;
   unsigned int m_num_pts;
   unsigned int m_pt_counter;
   XYSegList m_seglist;
   double m_x_pos;
   double m_y_pos;
   std::vector<double> m_x_pts_ordered;
   std::vector<double> m_y_pts_ordered;
   std::vector<int> m_id_pts_ordered;
   std::string m_update_var;
   std::vector<double> m_x_pts_missed;
   std::vector<double> m_y_pts_missed;
   std::vector<int> m_id_pts_missed;
   unsigned int m_wpt_idx;
   unsigned int m_visit_radius;
   double m_wpt_dist;
   bool m_received_wpt_idx;
   bool m_regen_path;

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
