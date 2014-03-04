/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: CommunicationAngle.cpp                          */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "CommunicationAngle.h"
#include <cmath>

using namespace std;

//---------------------------------------------------------
// Constructor

CommunicationAngle::CommunicationAngle() : m_collab_ready(false)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

CommunicationAngle::~CommunicationAngle()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool CommunicationAngle::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "VEHICLE_NAME")) {
      m_veh_name = msg.GetString();
    } else if (MOOSStrCmp(msg.GetKey(), "COLLABORATOR_NAME")) {
      m_collab_name = msg.GetString();
      m_collab_ready = true;
      std::stringstream collab_var;
      collab_var << m_collab_name << "_NAV_X";
      m_Comms.Register(collab_var.str(), 0);
      m_collab_nav_x = collab_var.str();
      collab_var.str("");
      collab_var.clear();
      collab_var << m_collab_name << "_NAV_Y";
      m_Comms.Register(collab_var.str(), 0);
      m_collab_nav_y = collab_var.str();
      collab_var.str("");
      collab_var.clear();
      collab_var << m_collab_name << "_NAV_DEPTH";
      m_Comms.Register(collab_var.str(), 0);
      m_collab_nav_depth = collab_var.str();
      collab_var.str("");
      collab_var.clear();
      collab_var << m_collab_name << "_NAV_HEADING";
      m_Comms.Register(collab_var.str(), 0);
      m_collab_nav_heading = collab_var.str();
      collab_var.str("");
      collab_var.clear();
      collab_var << m_collab_name << "_NAV_SPEED";
      m_Comms.Register(collab_var.str(), 0);
      m_collab_nav_speed = collab_var.str();
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_X")) {
      m_x_pos = msg.GetDouble();
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_Y")) {
      m_y_pos = msg.GetDouble();
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_DEPTH")) {
      m_z_pos = msg.GetDouble();
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_HEADING")) {
      m_heading = msg.GetDouble();
    } else if (MOOSStrCmp(msg.GetKey(), "NAV_SPEED")) {
      m_speed = msg.GetDouble();
    } else if (MOOSStrCmp(msg.GetKey(), "ELEV_ANGLE_REF")) {
      reference_angle = msg.GetDouble()*PI/180;
    }

    if (m_collab_ready) {
      if (MOOSStrCmp(msg.GetKey(), m_collab_nav_x)) {
        m_collab_x_pos = msg.GetDouble();
      } else if (MOOSStrCmp(msg.GetKey(), m_collab_nav_y)) {
        m_collab_y_pos = msg.GetDouble();
      } else if (MOOSStrCmp(msg.GetKey(), m_collab_nav_depth)) {
        m_collab_z_pos = msg.GetDouble();
      } else if (MOOSStrCmp(msg.GetKey(), m_collab_nav_heading)) {
        m_collab_heading = msg.GetDouble();
      } else if (MOOSStrCmp(msg.GetKey(), m_collab_nav_speed)) {
        m_collab_speed = msg.GetDouble();
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

bool CommunicationAngle::OnConnectToServer()
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

bool CommunicationAngle::Iterate()
{
  m_iterations++;

  std::stringstream publishString;
  if (CalculateTransmissionAngle()) {
    publishString << "elev_angle=" << m_transmission_angle << ",transmission_loss=" << m_transmission_loss << ",id=rypkema@mit.edu";
    m_Comms.Notify("ACOUSTIC_PATH", publishString.str());
    cout << publishString.str() << endl;
    publishString.str("");
    publishString.clear();
    publishString << "x=" << m_x_pos << ",y=" << m_y_pos << ",depth=" << m_z_pos << ",id=rypkema@mit.edu";
    m_Comms.Notify("CONNECTIVITY_LOCATION", publishString.str());
    cout << publishString.str() << endl;
  } else {
    CalculateConnectivityLocation();
    publishString << "elev_angle=NaN,transmission_loss=NaN,id=rypkema@mit.edu";
    m_Comms.Notify("ACOUSTIC_PATH", publishString.str());
    cout << publishString.str() << endl;
    publishString.str("");
    publishString.clear();
    publishString << "x=" << m_connection_x << ",y=" << m_connection_y << ",depth=" << m_connection_z << ",id=rypkema@mit.edu";
    m_Comms.Notify("CONNECTIVITY_LOCATION", publishString.str());
    cout << publishString.str() << endl;
  }
  cout << endl;

  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool CommunicationAngle::OnStartUp()
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

  if (!m_MissionReader.GetConfigurationParam("surface_sound_speed", m_surface_sound_speed)) {
    cerr << "No surface sound speed specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetConfigurationParam("sound_speed_gradient", m_sound_speed_gradient)) {
    cerr << "No sound speed gradient specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetConfigurationParam("water_depth", m_water_depth)) {
    cerr << "No water depth specified! Quitting!" << endl;
    return(false);
  }

  if (!m_MissionReader.GetConfigurationParam("time_interval", m_time_interval)) {
    cerr << "No time interval specified! Quitting!" << endl;
    return(false);
  }

  SetAppFreq(1/m_time_interval);

  m_timewarp = GetMOOSTimeWarp();

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void CommunicationAngle::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("VEHICLE_NAME", 0);
  m_Comms.Register("COLLABORATOR_NAME", 0);
  m_Comms.Register("NAV_X", 0);
  m_Comms.Register("NAV_Y", 0);
  m_Comms.Register("NAV_DEPTH", 0);
  m_Comms.Register("NAV_HEADING", 0);
  m_Comms.Register("NAV_SPEED", 0);
  m_Comms.Register("ELEV_ANGLE_REF", 0);
}

//---------------------------------------------------------
// Procedure: CalculateTransmissionAngle
//            Calculates the elevation angle from source to
//            receiver, assuming linear sound speed profile
//            and returns true if an acoustic path exists
//            (ray does not hit the seafloor) or false if
//            it does not exist

bool CommunicationAngle::CalculateTransmissionAngle()
{
  //calculate transmission/elevation angle using trigonometry
  //calculate vanishing depth
  double z_van = -(m_surface_sound_speed/m_sound_speed_gradient);
  //calculate maximum radius of circle
  m_max_radius = -z_van+m_water_depth;
  //convert from (x,y,z) coordinates to (r,z) coordinates, centering z axis on source
  double source_r = 0;
  double source_z = m_z_pos;
  double receiv_r = sqrt(pow(m_collab_x_pos-m_x_pos, 2) + pow(m_collab_y_pos-m_y_pos, 2));
  double receiv_z = m_collab_z_pos;
  //calculate difference in (r,z) between receiver and source, then calculate chord length
  double dr = abs(receiv_r-source_r);
  double dz = abs(receiv_z-source_z);
  double chord = sqrt(pow(dr ,2) + pow(dz ,2));
  //calculate angle of this difference triangle
  double theta = atan2(dz, dr);
  //calculate the center of the chord in (r,z)
  double chord_center_r;
  if (source_r > receiv_r) {
    chord_center_r = receiv_r + dr/2;
  } else {
    chord_center_r = source_r + dr/2;
  }
  double chord_center_z;
  if (source_z > receiv_z) {
    chord_center_z = receiv_z + dz/2;
  } else {
    chord_center_z = source_z + dz/2;
  }
  //calculate difference in (r,z) between center of chord and center of circle using similar triangles
  double w = chord_center_z-z_van;
  double v = (w/dr)*dz;
  double u = sqrt(pow(w, 2) + pow(v, 2));
  //we can now calculate the center of the transmission circle
  double circle_center_r;
  if (source_z > receiv_z) {
    circle_center_r = chord_center_r-v;
  } else {
    circle_center_r = chord_center_r+v;
  }
  double circle_center_z = chord_center_z-w;
  //calculate the radius of the transmission circle
  double radius = sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2));
  //calculate the angle between line connecting center of circle and center of chord and line connecting center of circle and startpoint of chord
  double theta_0 = atan2(chord/2, u);
  //calculate the transmission angle (angle between chord and line asymptotic to circle at source point +/- the angle of the difference triangle)
  double trans_angle;
  if (source_z > receiv_z) {
    trans_angle = theta - theta_0;
  } else {
    trans_angle = -theta - theta_0;
  }
  double arc_length = 2*theta_0*radius;

  //print out sanity checks
  cout << "dr,dz: " << dr << ", " << dz << " | " << (sqrt(pow(m_collab_x_pos-m_x_pos, 2) + pow(m_collab_y_pos-m_y_pos, 2))) << endl;
  cout << "source (r,z): " << source_r << ", " << source_z << endl;
  cout << "source (x,y,z): " << m_x_pos << ", " << m_y_pos << ", " << m_z_pos << endl;
  cout << "receiver (r,z): " << receiv_r << ", " << receiv_z << endl;
  cout << "receiver (x,y,z): " << m_collab_x_pos << ", " << m_collab_y_pos << ", " << m_collab_z_pos << endl;
  cout << "chord center (r,z): " << chord_center_r << ", " << chord_center_z << endl;
  cout << "w,v: " << w << ", " << v << endl;
  cout << "theta: " << (theta*180/PI) << endl;
  cout << "theta_0: " << (theta_0*180/PI) << endl;
  cout << "theta+-theta_0: " << ((theta*180/PI)-(theta_0*180/PI)) << " | " << ((-theta*180/PI)-(theta_0*180/PI)) << endl;
  cout << "circle center (r,z): " << circle_center_r << ", " << circle_center_z << endl;
  cout << "circle radius: " << radius << " (check) " << (sqrt(pow(receiv_z-circle_center_z, 2) + pow(receiv_r-circle_center_r, 2))) << " - " << (sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2))) << " = " << ((sqrt(pow(receiv_z-circle_center_z, 2) + pow(receiv_r-circle_center_r, 2)))-(sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2))))<< endl;
  cout << "transmission angle: " << (trans_angle*180/PI) << endl;

  //calculate the transmission loss using the Lab 6 given equations
  //choose a delta theta
  double d_theta = 0.001*PI/180;
  //calculate original radius using eq 6
  double radius_orig = CalculateSoundSpeed(m_z_pos)/(m_sound_speed_gradient*cos(trans_angle));
  //calculate receiver angle by rearranging the second last eq
  double receiv_angle = acos((m_collab_z_pos+m_surface_sound_speed/m_sound_speed_gradient)/radius_orig);
  cout << "received angle: " << receiv_angle*180/PI << endl;
  //use receiver angle to calculate r_i using fourth last eq
  double r_1 = radius_orig*sin(trans_angle) - radius_orig*sin(receiv_angle);
  cout << "receiver r1: " << r_1 << endl;
  //use receiver angle to calculate arc length using fifth last eq
  double arc_length2 = radius_orig*(trans_angle-receiv_angle);
  cout << "arc length: " << arc_length << ", " << arc_length2 << endl;
  //calculate a new radius using delta theta and eq 6
  double radius_new = CalculateSoundSpeed(m_z_pos)/(m_sound_speed_gradient*cos(trans_angle+d_theta));
  //use new radius, arc length, and delta theta to calculate r_i+1 using third last eq
  double r_2 = radius_new*sin(trans_angle+d_theta) + radius_new*sin(arc_length2/radius_new - (trans_angle+d_theta));
  cout << "receiver r2: " << r_2 << endl;
  //calculate J(s) using eq 5 and calculated r_i and r_i+1
  double J_s = (r_1/sin(receiv_angle))*((r_2-r_1)/d_theta);
  cout << "receiver J: " << J_s << endl;
  //calculate p(s) using eq 3 and J(s)
  double p_s = sqrt(abs((CalculateSoundSpeed(m_collab_z_pos)*cos(trans_angle))/(CalculateSoundSpeed(m_z_pos)*J_s)));
  cout << "receiver pressure: " << p_s << endl;
  //finally calculate transmission loss in decibels
  double trans_loss = -20*log10(p_s);
  cout << "transmission loss: " << trans_loss << endl;

  cout << "transmission angle comparison (us, ref, diff) " << (trans_angle*180/PI) << ", " << (reference_angle*180/PI) << ", " << (trans_angle*180/PI)-(reference_angle*180/PI) << endl;
  cout << endl;

  cout << "maximum radius: " << m_max_radius << endl;

  //check if circle radius is larger than depth of evironment
  if (abs(radius_orig) > m_max_radius) {
    return(false);
  } else {
    m_transmission_angle = (trans_angle*180/PI);
    m_transmission_loss = trans_loss;
    return(true);
  }
}

//---------------------------------------------------------
// Procedure: CalculateSoundSpeed
//            Calculates the sound speed at a given depth
//            z, assuming a linear sound speed profile

double CommunicationAngle::CalculateSoundSpeed(double z)
{
  return m_surface_sound_speed + m_sound_speed_gradient*z;
}

//---------------------------------------------------------
// Procedure: CalculateConnectivityLocation
//            Calculates the nearest point on a connectivi-
//            ty circle to move the source to if a direct
//            path does not exist

void CommunicationAngle::CalculateConnectivityLocation()
{
  //Calculate a new position for the source (x,y,z) that minimzes distance travelled to find a new valid transmission path
  //alter maximum radius using a small buffer off seafloor (1 meter)
  double seafloor_buffer = 1;
  m_max_radius -= seafloor_buffer;
  //convert from (x,y,z) coordinates to (r,z) coordinates, centering z axis on source
  double z_van = -(m_surface_sound_speed/m_sound_speed_gradient);
  double source_r = 0;
  double source_z = m_z_pos;
  double receiv_r = sqrt(pow(m_collab_x_pos-m_x_pos, 2) + pow(m_collab_y_pos-m_y_pos, 2));
  double receiv_z = m_collab_z_pos;
  //recalculate a transmission circle center using a maximum radius and trig
  double w = m_collab_z_pos-z_van;
  double v = sqrt(pow(m_max_radius,2)-pow(w,2));
  double circle_center_z = z_van;
  double circle_center_r = receiv_r-v;
  cout << "circle center (r,z): " << circle_center_r << ", " << circle_center_z << endl;
  cout << "circle radius: " << m_max_radius << " (check) " << (sqrt(pow(receiv_z-circle_center_z, 2) + pow(receiv_r-circle_center_r, 2))) << " - " << (sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2))) << " = " << ((sqrt(pow(receiv_z-circle_center_z, 2) + pow(receiv_r-circle_center_r, 2)))-(sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2))))<< endl;
  //calculate difference in (r,z) using this new circle center
  double dr = circle_center_r - source_r;
  double dz = circle_center_z - source_z;
  cout << "(dr, dz): " << dr << ", " << dz << endl;
  //calculate distance between new circle center and source position
  double source_R = sqrt(pow(source_z-circle_center_z, 2) + pow(source_r-circle_center_r, 2));
  cout << "source dist, ratio: " << source_R << ", " << m_max_radius/source_R << endl;
  cout << "(dr_new, dz_new): " << m_max_radius/source_R*dr << ", " << m_max_radius/source_R*dz << endl;
  //calculate a new (r,z) position of the source using similar triangles (closest point of approach in (r,z))
  double r_new = circle_center_r-m_max_radius/source_R*dr;
  m_connection_z = circle_center_z-m_max_radius/source_R*dz;
  cout << "new source (r,z): " << r_new << ", " << m_connection_z << endl;
  cout << "new source circle radius: " << (sqrt(pow(m_connection_z-circle_center_z, 2) + pow(r_new-circle_center_r, 2))) << endl;
  //calculate the distance between the new source position and receiver position in r
  double r_diff_new = sqrt(pow(receiv_r-r_new, 2) + pow(receiv_z-m_connection_z, 2));
  cout << "old, new r difference, ratio: " << sqrt(pow(receiv_r-source_r, 2) + pow(receiv_z-source_z, 2)) << ", " << r_diff_new << ", " << r_diff_new/receiv_r << endl;
  //calculate difference in (x,y) using current source and receiver positions
  double dx = m_collab_x_pos-m_x_pos;
  double dy = m_collab_y_pos-m_y_pos;
  cout << "(dx_new, dy_new): " << r_diff_new/receiv_r*dx << ", " << r_diff_new/receiv_r*dy << endl;
  //calculate a new (x,y) position of source (along r line) using similar triangles
  m_connection_x = m_collab_x_pos-r_diff_new/receiv_r*dx;
  m_connection_y = m_collab_y_pos-r_diff_new/receiv_r*dy;
  cout << "new source (x,y,z): " << m_connection_x << ", " << m_connection_y << ", " << m_connection_z << endl;
}
