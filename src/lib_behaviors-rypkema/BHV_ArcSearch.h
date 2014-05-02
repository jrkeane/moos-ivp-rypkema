/************************************************************/
/*    NAME: Nick Rypkema                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ArcSearch.h                                      */
/*    DATE:                                                 */
/************************************************************/

#ifndef ArcSearch_HEADER
#define ArcSearch_HEADER

#include <string>
#include "IvPBehavior.h"
#include "XYPoint.h"

class BHV_ArcSearch : public IvPBehavior {
public:
  BHV_ArcSearch(IvPDomain);
  ~BHV_ArcSearch() {};

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

protected: // Configuration parameters
  double   m_arrival_radius;
  double   m_desired_speed;
  double   m_center_x;
  double   m_center_y;
  double   m_radius;
  double   m_swath;
  double   m_angle;
  double   m_num_points;
  std::vector<double> m_x_points;
  std::vector<double> m_y_points;

protected: // State variables
  double   m_osx;
  double   m_osy;
  bool     m_startup;
  int      m_wpt_idx;
  int      m_wpt_final_idx;
  double   m_nextpt_x;
  double   m_nextpt_y;
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_ArcSearch(domain);}
}
#endif
