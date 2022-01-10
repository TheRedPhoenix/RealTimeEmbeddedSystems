# if you cannot run this file as a script, remember to give it executable
# permissions... to do so, open a terminal, navigate to the folder containing this file
# and give the command:
# chmod +x build_and_run.sh
filename="syslog-prog-1.3.txt"
expected_number_of_lines=129
echo Clean the previously compiled files
make clean
echo  also remove any preexistent log file named $filename
rm $filename
echo Compile our program
make 
echo Eexecute it...
sudo ./fifothreads
echo store the last $expected_number_of_lines lines of the syslog file into a dedicate file for submission
sleep 1
tail /var/log/syslog --lines $expected_number_of_lines > $filename
echo check the result by opening the file $filename
