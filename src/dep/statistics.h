/**
 * @file    statistics.h
 * @authors Wojciech Owczarek
 * @date   Sun Jul 7 13:28:10 2013
 *
 * This header file contains the definitions of structures and functions
 * implementing standard statistical measures - means and standard deviations,
 * both complete (from t0 to tnow) and moving (from t0+n*windowsize to t0+(n+1)*windowsize)
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

/**
 * \struct PtpEngineSlaveStats
 * \brief Ptpd clock statistics per port
 */
typedef struct
{
    Boolean statsCalculated;
    Boolean ofmStatsUpdated;
    Boolean mpdStatsUpdated;
    double ofmMean;
    double ofmStdDev;
    double ofmMedian;
    double ofmMin;
    double ofmMinFinal;
    double ofmMax;
    double ofmMaxFinal;
    double mpdMean;
    double mpdStdDev;
    double mpdMedian;
    double mpdMin;
    double mpdMinFinal;
    double mpdMax;
    double mpdMaxFinal;
    UInteger16 windowNumber;
    Boolean mpdIsStable;
    double mpdStabilityThreshold;
    int mpdStabilityPeriod;
    DoublePermanentStdDev ofmStats;
    DoublePermanentStdDev mpdStats;
    DoublePermanentMedian ofmMedianContainer;
    DoublePermanentMedian mpdMedianContainer;
} PtpEngineSlaveStats;

void clearPtpEngineSlaveStats(PtpEngineSlaveStats* stats);
void resetPtpEngineSlaveStats(PtpEngineSlaveStats* stats);

#endif /*STATISTICS_H_*/


