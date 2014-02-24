/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PrimeFactor.h                                   */
/*    DATE:                                                 */
/************************************************************/

#ifndef PrimeFactor_HEADER
#define PrimeFactor_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include <list>
#include "stdint.h"

class PrimeFactor : public CMOOSApp
{
 public:
   struct PrimeFactorStruct{
     uint64_t number;                     //received number
     uint64_t curr_number;                //number will be continually divided by primes
     uint64_t received;                   //index at which number was received
     uint64_t calculated;                 //index at which number was received
     double start_time;                    //start time of calculation
     double end_time;                      //end time of calculation
     std::vector<uint64_t> prime_factors; //list of computed prime factors for this number
     uint64_t curr_prime_factor;          //current prime factor of the number
   };

   PrimeFactor();
   ~PrimeFactor();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   void RegisterVariables();
   void CalcPrimeFactors(uint64_t num_iterations);          //loop through list of received numbers num_iterations times, calculating next num_iterations prime factors
   void CalcNextPrimeFactor(PrimeFactorStruct* pf_struct);  //calculate the next prime factor for a given input number
   void PrintPrimeFactors(PrimeFactorStruct* pf_struct);    //print vector of prime factors
   void PublishPrimeFactors(PrimeFactorStruct* pf_struct);  //publish list of prime factors to MOOSDB

 private: // Configuration variables
   std::list<PrimeFactorStruct> m_numbers;                //list of received numbers
   std::list<PrimeFactorStruct>::iterator m_numbers_iter; //iterator for list of numbers
   uint64_t m_received_idx;                               //number of received numbers
   uint64_t m_calculated_idx;                             //number of calculated numbers
   double m_t_start;
   double m_t_end;
   double m_t_diff;
   double m_t_period;
   unsigned int m_autotune;

 private: // State variables
   unsigned int m_iterations;
   double       m_timewarp;
};

#endif
