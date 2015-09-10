#include "mac_stat_tags.h"
#include "sis_stat_tags.h"

/*********************************
 * Stat Tag consolidation for the
 * MAC/SiS layers
 *********************************/

struct MAC_SIS_STATS
{
   SL_NAME = "MAC_SIS_STAT";
   using children = SL_MAKE_LIST
   (
      (mac::MAC_STATS)
      (sis::SIS_STATS)
   );
};

struct MAC_SIS_LOG
{
   SL_NAME = "MAC_SIS_LOG";
   using children = SL_MAKE_LIST
   (
      (mac::MAC_LOG)
      (sis::SIS_LOG)
   );
};

constexpr bool IsOperational = false;

template <bool IsOperational>
void initializeStatistics();

void handleCommandLineArgs(int argc, char** argv);

void genStats_MAC();
void genStats_SIS();
void genStats_HW_INTF();

