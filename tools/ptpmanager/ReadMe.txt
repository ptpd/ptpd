To test the application with PTPd,
---------------------------------
In himanshu-2012/src, do:
make
sudo ./ptpd2 -W -c -f log.txt -P -B

Then in himanshu-2012/tools/ptpmanager
make
cd Build/
sudo ./ptpmanager [IP of PTPd] [interface of PTPd]

Now send the management messages and when you terminate PTPd, all logs will be there in a file 'himanshu-2012/src/log.txt'.
