\name{ptpStats}
\alias{ptpStats}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Display statistics from a PTPd data frame
}
\description{
Show various statistics from a previously read PTPd data frame.
}
\usage{
ptpStats(ptplog)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{ptplog}{
    Data frame previously read with ptpLogRead()
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
ptpStats(log)}

## The function is currently defined as
function (ptplog) 
{
    cat("Offset", "\nmin:", min(ptplog$offset, na.rm = TRUE), 
        " max: ", max(ptplog$offset, na.rm = TRUE), " median: ", 
        median(ptplog$offset, na.rm = TRUE), " mean: ", mean(ptplog$offset, 
            na.rm = TRUE), "\nstd dev: ", sd(ptplog$offset, na.rm = TRUE), 
        " variance: ", var(ptplog$offset, na.rm = TRUE), "\n")
    cat("Delay", "\nmin:", min(ptplog$delay, na.rm = TRUE), " max: ", 
        max(ptplog$delay, na.rm = TRUE), " median: ", median(ptplog$delay, 
            na.rm = TRUE), " mean: ", mean(ptplog$delay, na.rm = TRUE), 
        "\nstd dev: ", sd(ptplog$delay, na.rm = TRUE), " variance: ", 
        var(ptplog$delay, na.rm = TRUE), "\n")
    cat("Master -> Slave", "\nmin:", min(ptplog$master.to.slave, 
        na.rm = TRUE), " max: ", max(ptplog$master.to.slave, 
        na.rm = TRUE), " median: ", median(ptplog$master.to.slave, 
        na.rm = TRUE), " mean: ", mean(ptplog$master.to.slave, 
        na.rm = TRUE), "\nstd dev: ", sd(ptplog$master.to.slave, 
        na.rm = TRUE), " variance: ", var(ptplog$master.to.slave, 
        na.rm = TRUE), "\n")
    cat("Slave -> Master", "\nmin:", min(ptplog$slave.to.master, 
        na.rm = TRUE), " max: ", max(ptplog$slave.to.master, 
        na.rm = TRUE), " median: ", median(ptplog$slave.to.master, 
        na.rm = TRUE), " mean: ", mean(ptplog$slave.to.master, 
        na.rm = TRUE), "\nstd dev: ", sd(ptplog$slave.to.master, 
        na.rm = TRUE), " variance: ", var(ptplog$slave.to.master, 
        na.rm = TRUE), "\n")
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
