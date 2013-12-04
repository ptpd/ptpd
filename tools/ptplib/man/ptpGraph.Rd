\name{ptpGraph}
\alias{ptpGraph}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Graph data from a PTPd log data frame
}
\description{
Graph data previously read from a PTPd log file into a PTPd log data frame.
}
\usage{
ptpGraph(logframe, value, output)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{logframe}{
    Data frame previously read by ptpdLogRead().
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
\dontrun{log = ptpLogRead("file.out")
ptpGraph(log)}

## The function is currently defined as
function (logframe, value, output) 
{
    if (!missing(output)) 
        png(filename = output, height = 960, width = 1280, bg = "white")
    if (missing(value)) {
        ymin = min(min(logframe$offset, na.rm = TRUE), min(logframe$delay, 
            na.rm = TRUE), min(logframe$master.to.slave, na.rm = TRUE), 
            min(logframe$slave.to.master, na.rm = TRUE))
        ymax = max(max(logframe$offset, na.rm = TRUE), max(logframe$delay, 
            na.rm = TRUE), max(logframe$master.to.slave, na.rm = TRUE), 
            max(logframe$slave.to.master, na.rm = TRUE))
        plot(logframe$delay, y = NULL, xaxt = "n", type = "n", 
            ylim = range(ymin, ymax), main = "PTP Results", xlab = "Time", 
            ylab = "Nanoseconds")
        legend(100, ymax, c("Delay", "Offset", "M->S", "S->M"), 
            col = c("orange", "blue", "red", "green"), pch = 21:24)
        points(logframe$offset, y = NULL, cex = 0.1, col = "blue", 
            pch = 21)
        points(logframe$master.to.slave, y = NULL, cex = 0.1, 
            col = "red", pch = 22)
        points(logframe$slave.to.master, y = NULL, cex = 0.1, 
            col = "green", pch = 23)
        points(logframe$delay, y = NULL, cex = 0.1, col = "orange", 
            pch = 24)
    }
    else {
        ymin = min(logframe[[value]], na.rm = TRUE)
        ymax = max(logframe[[value]], na.rm = TRUE)
        plot(logframe[[value]], y = NULL, xaxt = "n", type = "n", 
            ylim = range(ymin, ymax), main = "PTP Results", xlab = "Time", 
            ylab = "Nanoseconds")
        points(logframe[[value]], y = NULL, cex = 0.1, col = "red", 
            pch = 21)
    }
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
