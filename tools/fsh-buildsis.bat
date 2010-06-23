@echo off
set script=%0%
set script=%script:.bat=%
perl -Swx "%script%" %*

