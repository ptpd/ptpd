\name{ptpQualityRead}
\alias{ptpQualityRead}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Read a PTPd quality log into a data frame
}
\description{
Read a PTPd quality log into a data frame
}
\usage{
ptpQualityRead(file)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{file}{
%%     ~~Describe \code{file} here~~
}
}
\details{
%%  ~~ If necessary, more details than the description above ~~
}
\value{
%%  ~Describe the value returned
%%  If it is a LIST, use
%%  \item{comp1 }{Description of 'comp1'}
%%  \item{comp2 }{Description of 'comp2'}
%% ...
}
\references{
%% ~put references to the literature/web site here ~
}
\author{
%%  ~~who you are~~
}
\note{
%%  ~~further notes~~
}

%% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
%% ~~objects to See Also as \code{\link{help}}, ~~~
}
\examples{
## The function is currently defined as
function (file) 
{
    log = read.table(file, file = TRUE, sep = ",", col.names = c("packet", 
        "time"), blank.lines.skip = TRUE, header = FALSE)
    return(log)
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
