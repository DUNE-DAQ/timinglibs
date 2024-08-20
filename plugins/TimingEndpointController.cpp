/**
 * @file TimingEndpointController.cpp TimingEndpointController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

// #include "timinglibs/dal/TimingEndpointController.hpp"
#include "TimingEndpointController.hpp"
#include "timinglibs/dal/TimingEndpointController.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingendpointcontroller/Nljs.hpp"
#include "timinglibs/timingendpointcontroller/Structs.hpp"


#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingEndpointController::TimingEndpointController(const std::string& name)
  : dunedaq::timinglibs::TimingEndpointControllerBase(name, 6) // 2nd arg: how many hw commands can this module send?
  {}

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingEndpointController)

// Local Variables:
// c-basic-offset: 2
// End:
