/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_MySpiral.h                                      */
/*    DATE:                                                 */
/************************************************************/

#ifndef ContourFollow_HEADER
#define ContourFollow_HEADER

#include <string>
#include "IvPBehavior.h"

class BHV_MySpiral : public IvPBehavior {
public:
  BHV_MySpiral(IvPDomain);
  ~BHV_MySpiral() {};

  bool         setParam(std::string, std::string);
  void         onSetParamComplete();
  void         onCompleteState();
  void         onIdleState();
  void         onHelmStart();
  void         postConfigStatus();
  void         onRunToIdleState();
  void         onIdleToRunState();
  IvPFunction* onRunState();

protected: // Local Utility functions
  void updateStateVars();

protected: // Configuration parameters

protected: // State variables
  double m_xpos;
  double m_ypos;
  double m_heading;
  double m_init_heading;
  int m_num_rot;
  double m_rudder;
  double m_desired_rudder;
  bool m_ccw;
  std::vector<double> m_contour_x;
  std::vector<double> m_contour_y;
  double m_reduction;
  double m_increment;
  double m_increment_factor;
  double m_desired_speed;
  double m_start_time;
  double m_elapsed_time;
  double m_time_length;
  bool m_enter_spiral;
  double m_min_rudder;
  double m_min_dist_bet_pts;
  bool m_spiral_cw;
  bool m_spiral_ccw;
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_MySpiral(domain);}
}
#endif
