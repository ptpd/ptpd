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

#define STATCONTAINER_MAX_SAMPLES 2560
#define PEIRCE_MAX_SAMPLES 60

/* "Permanent" i.e. non-moving statistics containers - useful for long term measurement */

typedef struct {

	int32_t mean;
	int32_t previous;
	int32_t bufferedMean;
	int32_t count;

} IntPermanentMean;

typedef struct {

	double mean;
	double previous;
	double bufferedMean;
	double count;

} DoublePermanentMean;

typedef struct {

	IntPermanentMean meanContainer;
	uint32_t squareSum;
	int32_t stdDev;

} IntPermanentStdDev;

typedef struct {

	DoublePermanentMean meanContainer;
	double squareSum;
	double stdDev;

} DoublePermanentStdDev;

typedef struct {
	uint64_t squareSum;
	double adev;
	uint32_t count;
	int32_t _prev;
	int32_t lastDiff;
} IntPermanentAdev;

typedef struct {
	double squareSum;
	double adev;
	uint32_t count;
	double _prev;
	double lastDiff;
} DoublePermanentAdev;

typedef struct {
	int32_t median;
	int32_t bucket[3];
	uint8_t count;
} IntPermanentMedian;

typedef struct {
	double median;
	double bucket[3];
	uint8_t count;
} DoublePermanentMedian;

void 	resetIntPermanentMean(IntPermanentMean* container);
int32_t feedIntPermanentMean(IntPermanentMean* container, int32_t sample);
void 	resetIntPermanentStdDev(IntPermanentStdDev* container);
int32_t feedIntPermanentStdDev(IntPermanentStdDev* container, int32_t sample);
void 	resetIntPermanentMedian(IntPermanentMedian* container);
int32_t feedIntPermanentMedian(IntPermanentMedian* container, int32_t sample);

void 	resetIntPermanentAdev(IntPermanentAdev* container);
double feedIntPermanentAdev(IntPermanentAdev* container, int32_t sample);


void 	resetDoublePermanentMean(DoublePermanentMean* container);
double 	feedDoublePermanentMean(DoublePermanentMean* container, double sample);
void 	resetDoublePermanentStdDev(DoublePermanentStdDev* container);
double 	feedDoublePermanentStdDev(DoublePermanentStdDev* container, double sample);
void 	resetDoublePermanentMedian(DoublePermanentMedian* container);
double 	feedDoublePermanentMedian(DoublePermanentMedian* container, double sample);

void 	resetDoublePermanentAdev(DoublePermanentAdev* container);
double feedDoublePermanentAdev(DoublePermanentAdev* container, double sample);


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
	int counter;
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
	double periodicStdDev;
	char identifier[10];

} DoubleMovingStdDev;

typedef struct {

	Boolean enabled;
	uint8_t	filterType;
	int	windowSize;
	uint8_t	windowType;
	uint16_t samplingInterval;

} StatFilterOptions;

typedef struct {

	IntMovingMean* meanContainer;
	int32_t output;
	int32_t* sortedSamples;
	char identifier[10];
	int counter;
	int lastBlocked;
	double blockingTime;
	uint32_t consecutiveBlocked;
	StatFilterOptions config;

} IntMovingStatFilter;

typedef struct {

	DoubleMovingMean* meanContainer;
	double output;
	double* sortedSamples;
	char identifier[10];
	int counter;
	int lastBlocked;
	double blockingTime;
	uint32_t consecutiveBlocked;
	StatFilterOptions config;

} DoubleMovingStatFilter;


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

IntMovingStatFilter* createIntMovingStatFilter(StatFilterOptions* config, const char* id);
void freeIntMovingStatFilter(IntMovingStatFilter** container);
void resetIntMovingStatFilter(IntMovingStatFilter* container);
Boolean feedIntMovingStatFilter(IntMovingStatFilter* container, int32_t sample);

DoubleMovingStatFilter* createDoubleMovingStatFilter(StatFilterOptions* config, const char* id);
void freeDoubleMovingStatFilter(DoubleMovingStatFilter** container);
void resetDoubleMovingStatFilter(DoubleMovingStatFilter* container);
Boolean feedDoubleMovingStatFilter(DoubleMovingStatFilter* container, double sample);

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


