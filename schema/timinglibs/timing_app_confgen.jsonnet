// This is the configuration schema for timinglibs

local moo = import "moo.jsonnet";
local sdc = import "daqconf/confgen.jsonnet";
local daqconf = moo.oschema.hier(sdc).dunedaq.daqconf.confgen;
local stime = import "timinglibs/timingcmd.jsonnet";
local timingcmd = moo.oschema.hier(stime).dunedaq.timinglibs.timingcmd;
local stiminglibs_confgen = import "timinglibs/confgen.jsonnet";
local timinglibs_confgen = moo.oschema.hier(stiminglibs_confgen).dunedaq.timinglibs.confgen;

local ns = "dunedaq.timinglibs.timing_app_confgen";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local cs = {

   timing_endpoint_controller: s.record("timing_endpoint_controller", [
    s.field('host_tec',                       daqconf.Host, default='localhost', doc='Host to run the controller app on'),
    s.field("enable_hardware_state_recovery", daqconf.Flag, default=true,        doc="Enable (or not) hardware state recovery"),
    s.field('endpoint_device_name',           daqconf.Str,  default="",          doc='Device name of timing hw'),
    s.field('endpoint_clock_file',            daqconf.Path, default="",          doc='Path to custom PLL config file for device'),
    s.field('endpoint_address',               timinglibs_confgen.number, default=0,           doc='Endpoint address.'),
    s.field('endpoint_partition',             timinglibs_confgen.number, default=0,           doc='Endpoint partitiion.'),
  ]),

  timing_gen: s.record('app_confgen', [
    s.field('boot',                       daqconf.boot,                                  default=daqconf.boot,                                  doc='Boot parameters'),
    s.field('timing_hardware_interface',  timinglibs_confgen.timing_hardware_interface,  default=timinglibs_confgen.timing_hardware_interface,  doc='THI parameters'),
    s.field('timing_master_controller',   timinglibs_confgen.timing_master_controller,   default=timinglibs_confgen.timing_master_controller,   doc='TMC parameters'),
    s.field('timing_fanout_controller',   timinglibs_confgen.timing_fanout_controller,   default=timinglibs_confgen.timing_fanout_controller,   doc='TFC parameters'),
    s.field('hsi',                        daqconf.hsi,                                   default=daqconf.hsi,                                   doc='HSI parameters'),
    s.field('timing_endpoint_controller', self.timing_endpoint_controller,               default=self.timing_endpoint_controller,               doc='TEC parameters')
  ])
};

// Output a topologically sorted array.
sdc + stime + stiminglibs_confgen + moo.oschema.sort_select(cs, ns)
