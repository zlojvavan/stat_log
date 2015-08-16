stat_log
========

This library is an extensible C++ library for logging and statistic collection.  I got the idea for this after "reinventing the wheel" on several projects at work; each time i was either forced to use a pre-existing solution in the code-base I was maintaining or roll my own.  At last, I decided that I would create a re-usable library with the following top-level
requirements:

1. _The user shall be able to define a hierarchy of statistics at compile time_. From the perspective of the user, the purpose of this hierarchy is to simply organize the statistics.  For example, suppose the user is writing software for a wireless network application.  Suppose, for simplicity, there are only 3 components of this software
  * **MAC**: (Multiple-Access)  This component may be responsible relaying user traffic from an IP component sitting above it. 
    Specifically, the MAC's roles: 
      * Fragmentation and Reassembly of IP packets into MAC frames.
      * Buffering/Queuing of said frames.
      * Neighbor discovery protocol.
      * Spectral deconfliction protocol to avoid wireless contention.
  * **SIS**: (Signal-in-Space) This component is responsible for
      * Relaying MAC and control frames between the MAC and the Hardware Interface layers.
      * Collecting per neighbor wireless link statistics (drops, average snr, etc.) for the purpose of optimizing the spectral  
        deconfliction protocol. 
  * **Hardware Interface**:  This component is responsible for interfacing with the hardware (e.g., an FPGA) that is responsible     for doing the wireless signal processing. 
  Given this (highly simplified) design, the user may define the following statistic hierarchy:
  ```cpp
  MAC{
    IP_PKTS_DOWN,
    IP_PKTS_UP,
    BUFFER_OVERFLOW
  }
  SIS{
    MAC_PKTS_DOWN
    MAC_PKTS_UP
    PER_NBR_DROPS
    PER_NBR_SNR,
    PER_NBR_LINK_STATUS
  }
  HW_INTERFACE
  {
     MISC_FPGA_FAULT,
     BUFFER_OVERFLOW
  }
  ```
  Note this hierachy allows the user to reuse the "BUFFER_OVERFLOW" statistic name.
  
2. _Extremely simple mechanism for updating statistics.  Furthermore, this mechanism should be completely agnostic to the details of what statistical values are to reported._  Here is one candidate interface which meets this requirement:

  ```cpp
  statLog.addStat<MAC::IP_PKTS_DOWN>(1);
  ```
  Or for single dimensioned statistics:
  
  ```cpp
  statLog.addStat<SIS::PER_NBR_SNR>(nbrId, thisSnrValue);
  ```
  Or for multi-dimensioned statistics:
  
   ```cpp
  statLog.addStat<SIS::PER_NBR_LINK_STATUS>(nbrId, LINK_TYPE_SYMMETRIC, 1);
  ```
  etc.
  
  Suppose for the MAC::IP_PKTS_DOWN statistic, we initially just want to keep a simple counter of all the IP packets
  flowing down into the MAC layer.  To do this we can setup a mapping (using template magic I discuss later) between the
  MAC::IP_PKTS_DOWN tag and a "Simple Counter" statistic.  During the application debugging phase, however, we realize
  that such a simple statistic is not rich enough for troubleshooting purposes.  To this end, we can re-map the 
  MAC::IP_PKTS_DOWN tag to a "Time-Series" statistic; this statisic may, for example, store time-stamped values in a
  buffer that can be graphed at a later time.  
  
  Whichever statistic we end up using, the statistic generating site would remain the same:
  ```cpp
  statLog.addStat<MAC::IP_PKTS_DOWN>(1);
  ```
  (A re-compile is necessary to switch the statistic type, however.)
  
3. Clean separation from "Operational" and "Control and/or Observational" modes

TODO: work in progress (i welcome feedback even at this early stage)

