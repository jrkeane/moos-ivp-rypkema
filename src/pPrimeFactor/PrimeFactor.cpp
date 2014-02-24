/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PrimeFactor.cpp                                 */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "PrimeFactor.h"
#include <iostream>
#include <math.h>
#include <sstream>

using namespace std;

//---------------------------------------------------------
// Constructor

PrimeFactor::PrimeFactor() : m_received_idx(1), m_calculated_idx(1), m_autotune(1)
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

PrimeFactor::~PrimeFactor()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool PrimeFactor::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "NUM_VALUE")) {
      //initialize a new struct with the given number, and push it onto the list of inputted numbers
      PrimeFactorStruct received_number;
      received_number.number = (uint64_t)msg.GetDouble();
      received_number.curr_number = received_number.number;
      received_number.received = m_received_idx;
      m_received_idx++;
      received_number.start_time = MOOSTime();
      received_number.curr_prime_factor = 2;
      m_numbers.push_front(received_number);
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

bool PrimeFactor::OnConnectToServer()
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

bool PrimeFactor::Iterate()
{
  m_iterations++;

  m_t_start = MOOSTime();

  CalcPrimeFactors(m_autotune);

  //perform autotuning of number of iterations of CalcPrimeFactors
  m_t_end = MOOSTime();
  m_t_diff = m_t_end - m_t_start;
  if (m_t_diff < m_t_period) {
    m_autotune *= floor(m_t_period/m_t_diff);
  } else {
    m_autotune /= ceil(m_t_diff/m_t_period);
  }
//  std::cout << m_autotune << std::endl;
//  std::cout << m_t_period << " | " << m_t_diff << std::endl;

  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PrimeFactor::OnStartUp()
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

  m_timewarp = GetMOOSTimeWarp();

  m_t_period = 1/GetAppFreq();

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void PrimeFactor::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("NUM_VALUE");
}

//---------------------------------------------------------
// Procedure: CalcPrimeFactors
//            Loops through list of received numbers, and
//            calculates the next prime factor of each
//            num_iterations times

void PrimeFactor::CalcPrimeFactors(uint64_t num_iterations)
{
  for (unsigned int idx = 0; idx < num_iterations; idx++) {
    for (m_numbers_iter = m_numbers.begin(); m_numbers_iter != m_numbers.end(); m_numbers_iter++) {
      CalcNextPrimeFactor(&(*m_numbers_iter));  //pass address of number struct currently pointed to by iterator
    }
  }
}

//---------------------------------------------------------
// Procedure: CalcNextPrimeFactor
//            Attempts to calculate the next prime factor
//            of a given number (passed by struct), by
//            checking the modulo of the number with
//            a possible factor - if modulo is zero
//            it is a true factor, otherwise increase
//            the factor by 2, and check again on the
//            next iteration; if modulo is zero, we
//            divide the number by the factor - when
//            the current possible factor squared is
//            greater than the current number, we are
//            finished - the current number is the final
//            (and maximum) factor


void PrimeFactor::CalcNextPrimeFactor(PrimeFactorStruct* pf_struct)
{
  if (pf_struct->curr_prime_factor*pf_struct->curr_prime_factor > pf_struct->curr_number) {
    //we are at the maximum factor - the current number is equivalent to the max factor
    if ((pf_struct->prime_factors.size() == 0) || (pf_struct->curr_number != pf_struct->prime_factors.back())) {
      //add the current number to the list of factors, only if it is not already in the list
      pf_struct->prime_factors.push_back(pf_struct->curr_number);

      std::cout << "Found the " << pf_struct->prime_factors.size() << "-th prime factor of (" << pf_struct->number << ")! It is: " << pf_struct->curr_number << std::endl;

    }

    //print to cout the list of prime factors
    std::cout << "Finished finding the prime factors of (" << pf_struct->number << ")! They are: ";
    PrintPrimeFactors(pf_struct);
    std::cout << endl;

    //publish the list of prime factors to the MOOSDB
    PublishPrimeFactors(pf_struct);
    //remove the number from the list of inputted numbers - we have determined all prime factors
    m_numbers_iter = m_numbers.erase(m_numbers_iter);
  } else {
    //check the current candidate to see if it is a prime factor
    if (pf_struct->curr_number % pf_struct->curr_prime_factor == 0) {
      //it is a prime factor, so add to list of prime factors (only if it is not on the list already), and divide the current number by this factor
      if ((pf_struct->prime_factors.size() == 0) || (pf_struct->curr_prime_factor != pf_struct->prime_factors.back())) {
        pf_struct->prime_factors.push_back(pf_struct->curr_prime_factor);

        std::cout << "Found the " << pf_struct->prime_factors.size() << "-th prime factor of (" << pf_struct->number << ")! It is: " << pf_struct->curr_prime_factor << std::endl;

      }
      pf_struct->curr_number /= pf_struct->curr_prime_factor;
    } else {
      //it is not a prime factor, so increment by 1 (if the current factor is 2), otherwise increment by 2 (since no even number is a prime factor)
      if (pf_struct->curr_prime_factor == 2) {
        pf_struct->curr_prime_factor += 1;
      } else {
        pf_struct->curr_prime_factor += 2;
      }
    }
  }
}

//---------------------------------------------------------
// Procedure: PrintPrimeFactors
//            Loops through list calculated prime factors
//            and prints each out to cout

void PrimeFactor::PrintPrimeFactors(PrimeFactorStruct* pf_struct)
{
  std::cout << "[";
  for (unsigned int idx = 0; idx < pf_struct->prime_factors.size()-1; idx++) {
    std::cout << pf_struct->prime_factors[idx] << ", ";
  }
  std::cout << pf_struct->prime_factors.back() << "]";
}

//---------------------------------------------------------
// Procedure: PrintPrimeFactors
//            Publish a given number struct to the MOOSDB
//            in the specified format:
//            PRIME_RESULT = "orig=30030,received=34,calculated=33,solve_time=2.03,primes=2:3:5:7:11:13,username=jane"

void PrimeFactor::PublishPrimeFactors(PrimeFactorStruct* pf_struct)
{
  std::stringstream publishString;
  publishString << std::fixed << std::setprecision(2);
  pf_struct->calculated = m_calculated_idx;
  m_calculated_idx++;
  pf_struct->end_time = MOOSTime();
  publishString << "orig=" << pf_struct->number << ",received=" << pf_struct->received << ",calculated=" << pf_struct->calculated << ",solve_time=" << (pf_struct->end_time-pf_struct->start_time) << ",primes=";
  for (unsigned int idx = 0; idx < pf_struct->prime_factors.size()-1; idx++) {
    publishString << pf_struct->prime_factors[idx] << ":";
  }
  publishString << pf_struct->prime_factors.back() << ",username=rypkema";
  std::cout << publishString.str() << std::endl;
  m_Comms.Notify("PRIME_RESULT", publishString.str());
}
