\name{ptpCompare}
\alias{ptpCompare}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Graphically compare two PTP data frames.
}
\description{
Graphically compare two PTP data frames.
}
\usage{
ptpCompare(loga, logb, value, output)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{loga}{
    Data frame previously returned by ptpLogRead()
}
  \item{logb}{
    Second data frame previously returned by ptpLogRead()
}
  \item{value}{
    Optional data to graph.  If no value is supplied then all of the
    major columns will be graphed together including "offset", "delay",
    "master.to.slave", and "slave.to.master"

}
  \item{output}{
    Optional output file.  When a file is output it is stored as a PNG.
}
}
\details{
%%  ~~ If necessary, more details than the description above ~~
}
\value{
NULL
}
\references{
http://ptpd.sf.net
}
\author{
George Neville-Neil
}
\note{
%%  ~~further notes~~
}

%% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
%% ~~objects to See Also as \code{\link{help}}, ~~~
}
\examples{
\dontrun{logA = ptpLogRead("fileA.out")
logB = ptpLogRead("fileB.out")
ptpCompare(logA, logB)}

## The function is currently defined as
function (loga, logb, value, output) 
{
    if (!missing(output)) 
        png(filename = output, height = 960, width = 1280, bg = "white")
    if (missing(value)) {
        ymin = min(min(loga$offset, na.rm = TRUE), min(loga$delay, 
            na.rm = TRUE), min(loga$master.to.slave, na.rm = TRUE), 
            min(loga$slave.to.master, na.rm = TRUE))
        ymax = max(max(loga$offset, na.rm = TRUE), max(loga$delay, 
            na.rm = TRUE), max(loga$master.to.slave, na.rm = TRUE), 
            max(loga$slave.to.master, na.rm = TRUE))
        plot(loga$delay, y = NULL, xaxt = "n", type = "n", ylim = range(ymin, 
            ymax), main = "PTP Results", xlab = "Time", ylab = "Nanoseconds")
        points(loga$delay, y = NULL, cex = 0.1, col = "black", 
            pch = 24)
        points(loga$offset, y = NULL, cex = 0.1, col = "blue", 
            pch = 21)
        points(loga$master.to.slave, y = NULL, cex = 0.1, col = "purple", 
            pch = 22)
        points(loga$slave.to.master, y = NULL, cex = 0.1, col = "green", 
            pch = 23)
        points(logb$delay, y = NULL, cex = 0.1, col = "black", 
            pch = 24)
        points(logb$offset, y = NULL, cex = 0.1, col = "deeppink", 
            pch = 21)
        points(logb$master.to.slave, y = NULL, cex = 0.1, col = "blue", 
            pch = 22)
        points(logb$slave.to.master, y = NULL, cex = 0.1, col = "red", 
            pch = 23)
        legend(100, ymax, c("Delay1", "Offset1", "M->S_1", "S->M_1", 
            "Delay2", "Offset2", "M->S_2", "S->M_w"), col = c("black", 
            "blue", "purple", "green", "black", "deeppink", "blue", 
            "red"), pch = 21:24)
    }
    else {
        ymin = min(min(loga[[value]], na.rm = TRUE), min(logb[[value]], 
            na.rm = TRUE))
        ymax = max(max(loga[[value]], na.rm = TRUE), max(logb[[value]], 
            na.rm = TRUE))
        plot(loga[[value]], y = NULL, xaxt = "n", type = "n", 
            ylim = range(ymin, ymax), main = "PTP Results", xlab = "Time", 
            ylab = "Nanoseconds")
        points(loga[[value]], y = NULL, cex = 0.1, col = "red", 
            pch = 21)
        points(logb[[value]], y = NULL, cex = 0.1, col = "blue", 
            pch = 22)
        legend(100, ymax, c(paste(value, "1"), paste(value, "2")), 
            col = c("red", "blue"), pch = 21:22)
    }
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
