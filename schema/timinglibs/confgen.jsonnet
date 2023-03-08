// This is the configuration schema for timinglibs

local moo = import "moo.jsonnet";
local sdc = import "daqconf/confgen.jsonnet";
local daqconf = moo.oschema.hier(sdc).dunedaq.daqconf.confgen;
local stime = import "timinglibs/timingcmd.jsonnet";
local timingcmd = moo.oschema.hier(stime).dunedaq.timinglibs.timingcmd;

local ns = "dunedaq.timinglibs.confgen";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local cs = {
  number: s.number  ("number", "i8", doc="a number"), // !?!?!
  firmware_type: s.enum("TimingFirmwareType", ["pdi", "pdii"]),

  timing_hardware_interface: s.record("timing_hardware_interface", [
    s.field('firmware_type',              self.firmware_type, default='pdii',       doc="Timing firmware type, pdi (NRZ) or pdii (no cdr)"),
    s.field('gather_interval',            self.number,        default=5e5,         doc='Gather interval for hardware polling [us]'),
    s.field('gather_interval_debug',      self.number,        default=10e7,        doc='Gather interval for hardware polling debug/i2c [us]'),
    s.field('host_thi',                   daqconf.Host,       default='localhost', doc='Host to run the (global) timing hardware interface app on'),
    s.field('timing_hw_connections_file', daqconf.Path,       default="${TIMING_SHARE}/config/etc/connections.xml", doc='Path to timing hardware connections file'),
    s.field('hsi_device_name',            daqconf.Str,        default="",  doc='Real HSI hardware only: device name of HSI hw'),
  ]),

  timing_master_controller: s.record("timing_master_controller", [
    s.field('host_tmc',                       daqconf.Host, default='localhost', doc='Host to run the (global) timing master controller app on'),
    s.field("enable_hardware_state_recovery", daqconf.Flag, default=true,        doc="Enable (or not) hardware state recovery"),
    s.field('master_device_name',             daqconf.Str,  default="",          doc='Device name of timing master hw'),
    s.field('master_endpoint_scan_period',    self.number,  default=0,           doc="Master controller continuously scans the endpoint addresses provided. This controls period of scan [ms]. 0 for disable."),
    s.field('master_clock_file',              daqconf.Path, default="",          doc='Path to custom PLL config file for master device'),
    s.field('master_clock_mode',              self.number,  default=-1,          doc='Fanout mode for master device.'),
    s.field('monitored_endpoints',            timingcmd.TimingEndpointScanAddresses, default=[], doc='List of endpoint addresses to be monitored.'),
  ]),

  timing_fanout_controller: s.record("timing_fanout_controller", [
    s.field("enable_hardware_state_recovery", daqconf.Flag, default=true,        doc="Enable (or not) hardware state recovery"),
    s.field('fanout_device_name',             daqconf.Str,  default="",          doc='Device name of timing fanout hw'),
    s.field('host_tfc',                       daqconf.Host, default='localhost', doc='Host to run the (global) timing master controller app on'),
    s.field('fanout_clock_file',              daqconf.Path, default="",          doc='Path to custom PLL config file for fanout device'),
  ]),

  timing_gen: s.record('timing_gen', [
    s.field('boot',                      daqconf.boot,                   default=daqconf.boot,                   doc='Boot parameters'),
    s.field('timing_hardware_interface', self.timing_hardware_interface, default=self.timing_hardware_interface, doc='THI parameters'),
    s.field('timing_master_controller',  self.timing_master_controller,  default=self.timing_master_controller,  doc='TMC parameters'),
    s.field('timing_fanout_controller',  self.timing_fanout_controller,  default=self.timing_fanout_controller,  doc='TFC parameters'),
  ]),

};

// Output a topologically sorted array.
sdc + stime + moo.oschema.sort_select(cs, ns)
