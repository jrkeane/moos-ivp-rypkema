/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PrimeFactorTester.h                             */
/*    DATE:                                                 */
/************************************************************/

#ifndef PrimeFactorTester_HEADER
#define PrimeFactorTester_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include "stdint.h"
#include <queue>

class PrimeFactorTester : public CMOOSApp
{
 public:
   PrimeFactorTester();
   ~PrimeFactorTester();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();
   void CheckPrimeFactorisation(uint64_t num, std::vector<std::string> prime_factors);
   bool IsPrime(uint64_t num);

 private: // Configuration variables
   std::string m_prime_result;                    //result of pPrimeFactor (PRIME_RESULT)
   std::queue<std::string> m_prime_result_queue;  //queue of PRIME_RESULT strings - we may not be able to check each result quick enough

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
