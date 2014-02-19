/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: Odometry.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "math.h"
#include "MBUtils.h"
#include "Odometry.h"

using namespace std;

//---------------------------------------------------------
// Constructor

Odometry::Odometry() : m_first_reading(false), m_moved(false), m_current_x(0.0), m_current_y(0.0), m_previous_x(0.0), m_previous_y(0.0), m_total_distance(0.0)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
{
  //appcasting mail handling superclass method
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "NAV_X")) {
       //store x position and post flag that we have a valid reading
       m_current_x = msg.GetDouble();
       if (!m_first_reading) m_first_reading = true;
    }

    else if (MOOSStrCmp(msg.GetKey(), "NAV_Y")) {
       //store y position
       m_current_y = msg.GetDouble();
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

bool Odometry::OnConnectToServer()
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

bool Odometry::Iterate()
{
  m_iterations++;

  //appcasting iterate superclass method
  AppCastingMOOSApp::Iterate();

  double diff;  //hold the current timestep odometry distance
  if ((m_first_reading) && !(m_moved)) {
     //if its the first reading, set the previous value to current value
     //and post flag that the vehicle has moved (not really needed)
     m_previous_x = m_current_x;
     m_previous_y = m_current_y;
     m_moved = true;
  }
  if (m_moved) {
     //vehicle has 'moved' so calculate the odometry and accumulate
     diff = sqrt(pow((m_current_x-m_previous_x),2)+pow((m_current_y-m_previous_y),2));
     m_total_distance += diff;
     m_previous_x = m_current_x;
     m_previous_y = m_current_y;
  }

  //publish our odometry measurement to MOOSDB
  m_Comms.Notify("ODOMETRY_DIST", m_total_distance);

  //appcasting post report superclass method
  AppCastingMOOSApp::PostReport();

  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Odometry::OnStartUp()
{
  //appcasting startup superclass method
  AppCastingMOOSApp::OnStartUp();

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

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void Odometry::RegisterVariables()
{
   //appcasting variable registration superclass method
   AppCastingMOOSApp::RegisterVariables();

   //register for vehicle navigation info
   m_Comms.Register("NAV_X");
   m_Comms.Register("NAV_Y");
  // m_Comms.Register("FOOBAR", 0);
}

bool Odometry::buildReport()
{
   m_msgs << "Total Distance Travelled: " << m_total_distance << " meters" << endl;

   return(true);
}
