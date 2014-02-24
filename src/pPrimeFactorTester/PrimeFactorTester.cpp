/************************************************************/
/*    NAME: Nick Rypkema                                    */
/*    ORGN: MIT                                             */
/*    FILE: PrimeFactorTester.cpp                           */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "PrimeFactorTester.h"

using namespace std;

//---------------------------------------------------------
// Constructor

PrimeFactorTester::PrimeFactorTester()
{
  m_iterations = 0;
  m_timewarp   = 1;
}

//---------------------------------------------------------
// Destructor

PrimeFactorTester::~PrimeFactorTester()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool PrimeFactorTester::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
  std::string prime_result;

  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    if (MOOSStrCmp(msg.GetKey(), "PRIME_RESULT")) {
    //get a PRIME_RESULT and push it onto our queue for processing
      prime_result = msg.GetString();
      m_prime_result_queue.push(prime_result);
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

bool PrimeFactorTester::OnConnectToServer()
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

bool PrimeFactorTester::Iterate()
{
  std::vector<std::string> result_vector;
  std::string orig_num_str;
  std::string left;
  uint64_t orig_num;
  std::string primes_str;
  std::vector<std::string> primes;

  m_iterations++;

  //pop an PRIME_RESULT from our queue and check its validity
  if (!m_prime_result_queue.empty()) {
    m_prime_result = m_prime_result_queue.front();
    result_vector = parseString(m_prime_result, ',');
    orig_num_str = result_vector[0];
    left = biteString(orig_num_str, '=');
    orig_num = strtoull(orig_num_str.c_str(), NULL, 0);
    primes_str = result_vector[4];
    left = biteString(primes_str, '=');
    primes = parseString(primes_str, ':');
    CheckPrimeFactorisation(orig_num, primes);
    m_prime_result_queue.pop();
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PrimeFactorTester::OnStartUp()
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

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void PrimeFactorTester::RegisterVariables()
{
  // m_Comms.Register("FOOBAR", 0);
  m_Comms.Register("PRIME_RESULT", 0);
}

//---------------------------------------------------------
// Procedure: CheckPrimeFactorisation()
//            Checks validity of PRIME_RESULT

void PrimeFactorTester::CheckPrimeFactorisation(uint64_t num, std::vector<std::string> prime_factors)
{
  std::stringstream publishString;
  uint64_t factor;
  uint64_t orig_num = num;

  std::cout << "Checking the factorization of " << num << ", with factors ";
  for (unsigned int i=0; i<prime_factors.size()-1; i++) std::cout << prime_factors[i] << ",";
  std::cout << prime_factors[prime_factors.size()-1] << " ..." << std::endl;

  for (unsigned int i=0; i<prime_factors.size(); i++) {
  //check if all factors are true factors and true primes
    factor = strtoull(prime_factors[prime_factors.size()-i-1].c_str(), NULL, 0);
    if (num % factor != 0) {
      std::cout << factor << " is not a factor of " << orig_num << " - INVALID!" << std::endl;

      publishString << m_prime_result << ",valid=false" << std::endl;
      m_Comms.Notify("PRIME_RESULT_VALID", publishString.str());
      return;
    }
    if (!IsPrime(factor)) {
      std::cout << factor << " is not a prime number - INVALID!" << std::endl;

      publishString << m_prime_result << ",valid=false" << std::endl;
      m_Comms.Notify("PRIME_RESULT_VALID", publishString.str());
      return;
    }
  }
  for (unsigned int i=0; i<prime_factors.size(); i++) {
  //check if the factors perform a true factorization (no missing factors)
    factor = strtoull(prime_factors[prime_factors.size()-i-1].c_str(), NULL, 0);
    while (num % factor == 0) {
      num /= factor;
      std::cout << factor << " ";
    }
  }
  if (num == 1) {
    std::cout << "== " << orig_num << " - VALID!" << std::endl;

    publishString << m_prime_result << ",valid=true" << std::endl;
    m_Comms.Notify("PRIME_RESULT_VALID", publishString.str());
  } else {
    std::cout << "!= " << orig_num << " - INVALID!" << std::endl;

    publishString << m_prime_result << ",valid=false" << std::endl;
    m_Comms.Notify("PRIME_RESULT_VALID", publishString.str());
  }
}

//---------------------------------------------------------
// Procedure: IsPrime()
//            Checks if a number is prime

bool PrimeFactorTester::IsPrime(uint64_t num)
{
  for (uint64_t i=2; i*i<=num; i++) {
    if (num % i == 0) return false;
  }
  return true;
}
