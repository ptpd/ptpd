\name{ptpLogRead}
\alias{ptpLogRead}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Read a PTPd log file into a data frame and several time series
}
\description{
Read a PTPd log file into a data frame and several time series.
}
\usage{
ptpLogRead(file)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{file}{
File containing the log data.
}
}
\details{
ptpLogRead is the primary function for working with PTPd log files.  It
is used to read a log file into a data frame that can be used by the
other calls in ptplib.
}
\value{
  \item{log}{Data frame containing the full log}
  \item{ts}{Merged time series of next four value (offset, delay, m->s, s->m)}
  \item{offset}{Zoo time series of the offset from the master.}
  \item{delay}{Zoo time series of the measured round trip delay.}
  \item{master.to.slave}{Zoo time series of the M->S one way time.}
  \item{slave.to.master}{Zoo time series of the S->M one way time.}
}
\references{
http://ptpd.sf.net
}
\author{
George V. Neville-Neil
}
\note{
}

\seealso{
ptpCompare, ptpGraph, ptpStats
}
\examples{
\dontrun{log = ptpLogRead("file.out")}

## The function is currently defined as
function (file) 
{
    log = read.table(file, fill = TRUE, sep = ",", col.names = c("timestamp", 
        "state", "clockID", "delay", "offset", "master.to.slave", 
        "slave.to.master", "drift", "packet"), blank.lines.skip = TRUE, 
        header = FALSE, skip = 100)
    return(log)
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{IO}
\keyword{data}% __ONLY ONE__ keyword per line
