#!/usr/bin/env Rscript
##
## 
##                This source code is part of
## 
##                        M D S C T K
## 
##       Molecular Dynamics Spectral Clustering ToolKit
## 
##                        VERSION 1.1.2
## Written by Joshua L. Phillips.
## Copyright (c) 2013, Joshua L. Phillips.
## check out http://github.com/douradopalmares/mdsctk/ for more information.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
## 
## If you want to redistribute modifications, please consider that
## derived work must not be called official MDSCTK. Details are found
## in the README & LICENSE files - if they are missing, get the
## official version at github.com/douradopalmares/mdsctk/.
## 
## To help us fund MDSCTK development, we humbly ask that you cite
## the papers on the package - you can find them in the top README file.
## 
## For more info, check our website at http://github.com/douradopalmares/mdsctk/
## 
##

if (Sys.getenv("MDSCTK_HOME")=="") {
    cat("\n")
    cat("Please set the MDSCTK_HOME environment variable\n")
    cat("before running this script.\n")
    cat("\n")
    q()
} else {
    source(paste(Sys.getenv("MDSCTK_HOME"),"/config.r",sep=""))
}

## Copyright stuff
cat("\n")
cat(paste("   MDSCTK ",MDSCTK_VERSION_MAJOR,".",MDSCTK_VERSION_MINOR,sep=""))
if (exists("program.name")) {
    cat(paste(" - ",program.name,sep=""))
}
cat("\n")
cat("   Copyright (C) 2013 Joshua L. Phillips\n")
cat("   MDSCTK comes with ABSOLUTELY NO WARRANTY; see LICENSE for details.\n")
cat("   This is free software, and you are welcome to redistribute it\n")
cat("   under certain conditions; see README.md for details.\n")
cat("\n")

if (!suppressPackageStartupMessages(require("argparse",character.only=TRUE))) {
    cat("   Please install the R package 'argparse' to use the MDSCTK R scripts.\n")
    cat("\n")
    q()
}

parser <- ArgumentParser()

## Functions
read.binary.int <- function(filename,n) {
  fd <- file(filename,open="rb")
  data <- readBin(fd,"integer",n=n,size=4)
  close(fd)
  return (data)
}

read.binary <- function(filename,n) {
  fd <- file(filename,open="rb")
  data <- readBin(fd,"double",n=n,size=8)
  close(fd)
  return (data)
}


dens.pointwise <- function(data,sigma) {
  p <- colSums(exp(-data^2 / (2 * sigma^2)))
  return (p / nrow(data))

}

prob.pointwise <- function(data,sigma) {
  p <- colSums(exp(-data^2 / (2 * sigma^2)))
  return (p / sum(p))

}

entropy.pointwise <- function(data,dens) {
  return (apply(apply(data,
                      1,
                      FUN=function(vec,dens){dens[vec]},
                      dens),
                2,FUN=function(vec){-sum(log2(vec))/length(vec)}))
}

normalmutualinf <- function(data) {
  s <- 0
  e <- 0
  pc <- colSums(data)
  pr <- rowSums(data)
  for (i in 1:dim(data)[1])
    for (j in 1:dim(data)[2])
      if (data[i,j] > 0) {
        e <- e + (data[i,j] * log2(data[i,j]))
        s <- s + (data[i,j] * log2(data[i,j] / (pr[i] * pc[j])))
      }
  return(s / -e)
}

cluster.sort <- function(data,f=median) {
  s <- seq(range(data)[1], range(data)[2])
  x <- rep(0,length(s))
  l <- list()
  for (i in 1:length(s)) {
    l[[i]] <- which(data == s[i])
    x[i] <- f(l[[i]])
  }
  ix <- sort(x, index.return=TRUE)$ix
  for (i in 1:length(s)) data[l[[ix[i]]]] <- i
  return (data)
}
