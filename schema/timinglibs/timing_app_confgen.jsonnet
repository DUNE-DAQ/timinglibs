// This is the standalone configuration schema for timinglibs

local moo = import "moo.jsonnet";

local stypes = import "daqconf/types.jsonnet";
local types = moo.oschema.hier(stypes).dunedaq.daqconf.types;

local sbootgen = import "daqconf/bootgen.jsonnet";
local bootgen = moo.oschema.hier(sbootgen).dunedaq.daqconf.bootgen;

local shsigen = import "daqconf/hsigen.jsonnet";
local hsigen = moo.oschema.hier(shsigen).dunedaq.daqconf.hsigen;

local stime = import "timinglibs/timingcmd.jsonnet";
local timingcmd = moo.oschema.hier(stime).dunedaq.timinglibs.timingcmd;
local stiminglibs_confgen = import "timinglibs/confgen.jsonnet";
local timinglibs_confgen = moo.oschema.hier(stiminglibs_confgen).dunedaq.timinglibs.confgen;

local ns = "dunedaq.timinglibs.timing_app_confgen";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local cs = {

   timing_endpoint_controller: s.record("timing_endpoint_controller", [
    s.field('host_tec',                       types.host, default='localhost', doc='Host to run the controller app on'),
    s.field("enable_hardware_state_recovery", types.flag, default=true,        doc="Enable (or not) hardware state recovery"),
    s.field('endpoint_device_name',           types.string,  default="",          doc='Device name of timing hw'),
    s.field('endpoint_clock_file',            types.path, default="",          doc='Path to custom PLL config file for device'),
    s.field('endpoint_address',               timinglibs_confgen.number, default=0,           doc='Endpoint address.'),
    s.field('endpoint_partition',             timinglibs_confgen.number, default=0,           doc='Endpoint partitiion.'),
  ]),

  timing_gen: s.record('app_confgen', [
    s.field('boot',                       bootgen.boot,                                  default=bootgen.boot,                                  doc='Boot parameters'),
    s.field('timing_hardware_interface',  timinglibs_confgen.timing_hardware_interface,  default=timinglibs_confgen.timing_hardware_interface,  doc='THI parameters'),
    s.field('timing_master_controller',   timinglibs_confgen.timing_master_controller,   default=timinglibs_confgen.timing_master_controller,   doc='TMC parameters'),
    s.field('timing_fanout_controller',   timinglibs_confgen.timing_fanout_controller,   default=timinglibs_confgen.timing_fanout_controller,   doc='TFC parameters'),
    s.field('hsi',                        hsigen.hsi,                                    default=hsigen.hsi,                                    doc='HSI parameters'),
    s.field('timing_endpoint_controller', self.timing_endpoint_controller,               default=self.timing_endpoint_controller,               doc='TEC parameters')
  ])
};

// Output a topologically sorted array.
stypes + sbootgen + shsigen + stime + stiminglibs_confgen + moo.oschema.sort_select(cs, ns)
// 