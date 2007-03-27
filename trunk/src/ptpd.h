/* ptpd.h */

#ifndef PTPD_H
#define PTPD_H

#include "constants.h"
#include "dep/constants_dep.h"
#include "dep/datatypes_dep.h"
#include "datatypes.h"
#include "dep/ptpd_dep.h"


/* arith.c */
UInteger32 crc_algorithm(Octet*,Integer16);
UInteger32 sum(Octet*,Integer16);
void fromInternalTime(TimeInternal*,TimeRepresentation*,Boolean);
void toInternalTime(TimeInternal*,TimeRepresentation*,Boolean*);
void normalizeTime(TimeInternal*);
void addTime(TimeInternal*,TimeInternal*,TimeInternal*);
void subTime(TimeInternal*,TimeInternal*,TimeInternal*);

/* bmc.c */
UInteger8 bmc(ForeignMasterRecord*,RunTimeOpts*,PtpClock*);
void m1(PtpClock*);
void s1(MsgHeader*,MsgSync*,PtpClock*);
void initData(RunTimeOpts*,PtpClock*);

/* probe.c */
void probe(RunTimeOpts*,PtpClock*);

/* protocol.c */
void protocol(RunTimeOpts*,PtpClock*);


#endif

