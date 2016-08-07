#ifndef PTP_TMP_H_
#define PTP_TMP_H_

#include <stdio.h>

#define PTPINFO(x, ...)		printf("lib1588.info  : "x, ##__VA_ARGS__)
#define PTPDEBUG(x, ...)	printf("libg1588.debug: "x, ##__VA_ARGS__)

#endif /* PTP_TMP_H */
