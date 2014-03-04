/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: CommunicationAngle.h                            */
/*    DATE:                                                 */
/************************************************************/

#ifndef CommunicationAngle_HEADER
#define CommunicationAngle_HEADER

#include "MOOS/libMOOS/MOOSLib.h"

class CommunicationAngle : public CMOOSApp
{
 public:
   CommunicationAngle();
   ~CommunicationAngle();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();
   bool CalculateTransmissionAngle();
   void CalculateConnectivityLocation();
   double CalculateSoundSpeed(double z);

 private: // Configuration variables
   double m_surface_sound_speed;
   double m_sound_speed_gradient;
   double m_water_depth;
   double m_time_interval;
   std::string m_veh_name;
   std::string m_collab_name;
   bool m_collab_ready;
   double m_x_pos;
   double m_y_pos;
   double m_z_pos;
   double m_heading;
   double m_speed;
   double m_collab_x_pos;
   double m_collab_y_pos;
   double m_collab_z_pos;
   double m_collab_heading;
   double m_collab_speed;
   std::string m_collab_nav_x;
   std::string m_collab_nav_y;
   std::string m_collab_nav_depth;
   std::string m_collab_nav_heading;
   std::string m_collab_nav_speed;
   double m_transmission_angle;
   double m_transmission_loss;
   double m_connection_x;
   double m_connection_y;
   double m_connection_z;
   double m_max_radius;

   double reference_angle;

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
