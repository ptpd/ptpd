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

#define STATCONTAINER_MAX_SAMPLES 60

/* "Permanent" i.e. non-moving statistics containers - useful for long term measurement */

typedef struct {

	int32_t mean;
	int32_t count;

} IntPermanentMean;

typedef struct {

	double mean;
	double count;

} DoublePermanentMean;

typedef struct {

	IntPermanentMean meanContainer;
	int32_t squareSum;
	int32_t stdDev;

} IntPermanentStdDev;

typedef struct {

	DoublePermanentMean meanContainer;
	double squareSum;
	double stdDev;

} DoublePermanentStdDev;

void 	resetIntPermanentMean(IntPermanentMean* container);
int32_t feedIntPermanentMean(IntPermanentMean* container, int32_t sample);
void 	resetIntPermanentStdDev(IntPermanentStdDev* container);
int32_t feedIntPermanentStdDev(IntPermanentStdDev* container, int32_t sample);

void 	resetDoublePermanentMean(DoublePermanentMean* container);
double 	feedDoublePermanentMean(DoublePermanentMean* container, double sample);
void 	resetDoublePermanentStdDev(DoublePermanentStdDev* container);
double 	feedDoublePermanentStdDev(DoublePermanentStdDev* container, double sample);

/* Moving statistics - up to last n samples */

typedef struct {

	int32_t mean;
	int32_t sum;
	int32_t* samples;
	Boolean full;
	int count;
	int capacity;

} IntMovingMean;

typedef struct {

	double mean;
	double sum;
	double* samples;
	Boolean full;
	int count;
	int capacity;

} DoubleMovingMean;

typedef struct {

	IntMovingMean* meanContainer;
	int32_t squareSum;
	int32_t stdDev;
	char* identifier[10];

} IntMovingStdDev;

typedef struct {

	DoubleMovingMean* meanContainer;
	double squareSum;
	double stdDev;
	char identifier[10];

} DoubleMovingStdDev;

typedef struct {

	IntMovingMean* meanContainer;
	int32_t median;
	int32_t* sortedSamples;
	char* identifier[10];

} IntMovingMedian;

typedef struct {

	DoubleMovingMean* meanContainer;
	double median;
	double* sortedSamples;
	char* identifier[10];

} DoubleMovingMedian;

IntMovingMean* createIntMovingMean(int capacity);
void freeIntMovingMean(IntMovingMean** container);
void resetIntMovingMean(IntMovingMean* container);
int32_t feedIntMovingMean(IntMovingMean* container, int32_t sample);

IntMovingStdDev* createIntMovingStdDev(int capacity);
void freeIntMovingStdDev(IntMovingStdDev** container);
void resetIntMovingStdDev(IntMovingStdDev* container);
int32_t feedIntMovingStdDev(IntMovingStdDev* container, int32_t sample);

DoubleMovingMean* createDoubleMovingMean(int capacity);
void freeDoubleMovingMean(DoubleMovingMean** container);
void resetDoubleMovingMean(DoubleMovingMean* container);
double feedDoubleMovingMean(DoubleMovingMean* container, double sample);

DoubleMovingStdDev* createDoubleMovingStdDev(int capacity);
void freeDoubleMovingStdDev(DoubleMovingStdDev** container);
void resetDoubleMovingStdDev(DoubleMovingStdDev* container);
double feedDoubleMovingStdDev(DoubleMovingStdDev* container, double sample);

IntMovingMedian* createIntMovingMedian(int capacity);
void freeIntMovingMedian(IntMovingMedian** container);
void resetIntMovingMedian(IntMovingMedian* container);
int32_t feedIntMovingMedian(IntMovingMedian* container, int32_t sample);

DoubleMovingMedian* createDoubleMovingMedian(int capacity);
void freeDoubleMovingMedian(DoubleMovingMedian** container);
void resetDoubleMovingMedian(DoubleMovingMedian* container);
double feedDoubleMovingMedian(DoubleMovingMedian* container, double sample);

void intStatsTest(int32_t sample);
void doubleStatsTest(double sample);

double getPeircesCriterion(int numObservations, int numDoubtful);

Boolean isIntPeircesOutlier(IntMovingStdDev *container, int32_t sample, double threshold);
Boolean isDoublePeircesOutlier(DoubleMovingStdDev *container, double sample, double threshold);

/**
 * \struct PtpEngineSlaveStats
 * \brief Ptpd clock statistics per port
 */
typedef struct
{
    Boolean statsCalculated;
    double ofmMean;
    double ofmStdDev;
    double owdMean;
    double owdStdDev;
    Boolean owdIsStable;
    double owdStabilityThreshold;
    int owdStabilityPeriod;
    DoublePermanentStdDev ofmStats;
    DoublePermanentStdDev owdStats;
} PtpEngineSlaveStats;

void clearPtpEngineSlaveStats(PtpEngineSlaveStats* stats);
void resetPtpEngineSlaveStats(PtpEngineSlaveStats* stats);

#endif /*STATISTICS_H_*/


