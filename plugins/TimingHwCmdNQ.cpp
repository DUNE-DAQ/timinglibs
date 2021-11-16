/**
 * @file plugins/HSIEventNQ.cpp HSIEvent Network Queue Definition
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingcmd/msgp.hpp"

#include "nwqueueadapters/AdapterMacros.hpp"

DEFINE_DUNE_NWQUEUEADAPTERS(dunedaq::timinglibs::timingcmd::TimingHwCmd)