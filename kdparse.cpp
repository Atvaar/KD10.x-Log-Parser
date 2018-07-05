//~ kdparse.cpp
//~ Copyright 2016 aross <aross@AROSS-LT>
//~ this program is to parse killdisk reports into a database and then archive the file

#include <iostream>
#include <string>
#include <dirent.h>
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <regex>
#include <fstream>
#include <streambuf>
#include <sql.h> //for adding database functions
#include <sqltypes.h>//for adding database functions
#include <sqlext.h>//for adding database functions
#include <vector>

using namespace std;
//~ using namespace odbc;  //for adding database functions //not needed using std

struct DriveData {//structure for individual drive data
	string HDSerial;
	string WipeReport;
	string HDModel;
	string HDCapacity;//was int
	string HDProtocol;
	string HDAttatchment;
	string EraseMethod;
	string WipeStart;
	string WipeFinished;
	string SessionStart;
	string SessionEnded;
	string WipeStatus;
};//end struct DriveData
	
class Parser {
  private:
    string dataString;//content of input file
	string fileOut;//.csv
    string reportStamp;//parsed data//report time date string common to all reports in the file
    string reportString;
	string loadFile(string);
	string findReport(string);
	string findSession(string);
	string findDrives(string, DriveData);
	int upLoad(DriveData);
  public:
    int start_Parser (string, string);
};

int Parser::start_Parser (string x, string y) {//initial call fires off all other routines
	bool errFlag = false;
	string fileIn = x;//.log
	string filePath = y;//path to the folder in use
	fileOut = fileIn;
	fileOut = fileOut.erase((fileOut.find(".")+1),3) + "csv";
	dataString = loadFile(filePath + fileIn);
	//cout << dataString << endl;//debugging
	reportString = findReport(dataString);
	
	while (dataString.length() > 0){//loop sessions... while !npos findSession()
		dataString = findSession(dataString);
	}
	//move .log file
	if (errFlag == true){ return 1;}
	else {return errFlag;}
}//end set_values(string, string)

string Parser::loadFile(string subjectA){//read contents to string
	ifstream t;
	t.open(subjectA);
	string buffer;
	string line;
	while(t){
		getline(t, buffer);// ... Append line to buffer and go on
		line = line + buffer + "\n";
	}
	t.close();
	//~ cout << line << endl;//debug
	return line;
}//end loadFile(string)

string Parser::findSession(string ReportString){//find session begin and end times
	string sessionData;
	string sessionTime;
	string sessionBegining;
	string sessionFinishing;
	string theMethod;
	
	//finding session start
	string firstMarker = "---------------------------------------Erase Session Begin----------------------------------------\n";
	if (ReportString.find(firstMarker) == string::npos) return "";
	size_t sessionInit = ReportString.find(firstMarker);
	string stopHere = " Sending email report...";
	if (ReportString.find(stopHere) >= string::npos)
	{
		cout << "Session not sent" << endl;
		stopHere = " Log saved to:";
	}
	size_t sessionExit = ReportString.find(stopHere) + std::strlen(stopHere.c_str());
	//~ sessionData = ReportString.substr(ReportString.find(" Sending email report...") - 19,19);//SUCK SESS!	
	sessionData = ReportString.substr(sessionInit, sessionExit-sessionInit);//SUCK SESS!
	//~ cout << "This is the session Data:\n" << sessionData << endl;//debugging
	size_t beginStart = ReportString.find(firstMarker) + firstMarker.length();
	size_t beginStop;
	if (ReportString.find(" Active@ KillDisk v. 10.1.1 started") < string::npos){///WORK IT GUS BABY!!!
		//wtf = ReportString.find(" Active@ KillDisk v. 10.1.1 started");
		beginStop = ReportString.find(" Active@ KillDisk v. 10.1.1 started");//Change if Version Changed to if Fund then
	}
	else if(ReportString.find(" 10.2.7 started") < string::npos){
		//wtf = ReportString.find(" 10.2.7 started");
		beginStop = ReportString.find(" 10.2.7 started");//Change if Version Changed to if Fund then
		cout << "found this:  10.2.7 started" << endl;
	}
	else if(ReportString.find(" 10.2.8 started") < string::npos){
		//wtf = ReportString.find(" 10.2.8 started");
		beginStop = ReportString.find(" 10.2.8 started");//Change if Version Changed to if Fund then
	}
	//size_t beginStop = wtf;
	//~ cout << "begin-" << beginStart << ".  end-" << beginStop << "." << endl;//debug
	//~ sessionBegining = ReportString.substr(beginStop - 19,19);
	sessionBegining = ReportString.substr(beginStart,beginStop - beginStart);
	cout << "Where it begins:" << sessionBegining << "!" << endl;//debug
	
	//finding session end
	firstMarker = "---------------------------------------Erase Session End------------------------------------------\n";
	beginStart = ReportString.find(firstMarker) + firstMarker.length();
	cout << "Repiort String: " << endl << ReportString << endl;;
	beginStop = ReportString.find(stopHere);
	cout << "found beginStop: " << beginStop << endl;
	//~ sessionFinishing = ReportString.substr(beginStart,beginStop - beginStart);
	sessionFinishing = ReportString.substr(beginStop - 19,19);
	//~ cout << "Where it ends:" << sessionFinishing << "!" << endl;//debug
	//~ cout << "to Remove from Report String:" << finalStop << endl;
	
	//finding wipemethod
	firstMarker = "Erase method: ";
	beginStart = ReportString.find(firstMarker) + firstMarker.length();
	beginStop = ReportString.find(" Passes:");//debug
	theMethod = ReportString.substr(beginStart,beginStop - beginStart);
	if (theMethod.find(",") < string::npos){
			theMethod.erase(theMethod.find(","), 1);
		}
	//cout << "Wipe Method:" << theMethod << "!" << endl;//debug
	
	//save time stamps to dummy and snip to just drive data
	DriveData dummy;
	dummy.WipeReport = reportString;//Parser Level Variable
	dummy.SessionStart = sessionBegining;//Session Level Variable
	dummy.SessionEnded = sessionFinishing;//Session Level Variable
	dummy.EraseMethod = theMethod;//Session Level Variable

	//~ cout << "Data Length: " << sessionData.length() << endl;
	while (sessionData.length() > 0){
		sessionData = findDrives(sessionData, dummy);
	}//%%%%%%%%% WORK HERE GUS %%%%%%%%%%%% dont forget to get rid of the \n's
	
	//if error save session string to error.log
	//cut off alread parsed session from ReportString
	ReportString = ReportString.erase(0, sessionExit);
	return ReportString;
}//end findSession()


//find end of last report sent and the last time stamp is the data
string Parser::findReport(string data){
	cout << "Trying to get Report Time Stamp!" << endl;
	string startHere = "---------------------------------------Erase Session End------------------------------------------\n";
	string stopHere = " Sending email report...";
	if (data.find(stopHere) >= string::npos)
	{
		cout << "saved not sent" << endl;
		stopHere = " Log saved to:";
	}
	size_t going = data.rfind(startHere)+startHere.length();
	size_t stopping = data.rfind(stopHere)-going;
	string reportStamp = data.substr(going,stopping);
	
	cout << "found reportStamp" << reportStamp << endl;
	reportStamp = data.substr(data.rfind(stopHere)-19,19);
	cout << "report time stamp is: " << reportStamp << endl;//debugging finding report stamp
	return reportStamp;
}//end findReport(string)

//This is the sub parser to find individual drive information from the session data//
string Parser::findDrives(string session2Drives, DriveData common){
	DriveData foundDrive;
	foundDrive.WipeReport = common.WipeReport;
	foundDrive.SessionStart = common.SessionStart;
	foundDrive.SessionEnded	= common.SessionEnded;
	foundDrive.EraseMethod = common.EraseMethod;
	string firstMarker = "Erase Fixed Disk";
	string secondMarker = " GB\n";
	if ((session2Drives.find(" MB\n") < session2Drives.find(" GB\n")) && (session2Drives.find(" MB\n") < session2Drives.find(" TB\n"))){
		secondMarker = " MB\n";
	}
	else if ((session2Drives.find(" TB\n") < session2Drives.find(" GB\n")) && (session2Drives.find(" TB\n") < session2Drives.find(" MB\n"))){
	//else if (session2Drives.find(" TB\n") < string::npos){
		secondMarker = " TB\n";
	}
	string segment;
	if (session2Drives.find(firstMarker) == string::npos) return "";
	size_t starter = session2Drives.find(firstMarker);
	size_t ender = session2Drives.find(secondMarker);
	session2Drives = session2Drives.erase(0, starter);
	starter = session2Drives.find(firstMarker);
	ender = session2Drives.find(secondMarker);
	segment = session2Drives.substr(0, ender);
	cout << segment << "\n";
	//$$$$$$$$$$$$$NO SERIAL NUMBER GUS!!!!!!!!!!_____________------------
//start if 
	//~ string capture = segment.substr(segment.find(") - ") + 4, string::npos);
	//~ cout << "Drive Capacity:" << capture << "!" << endl;//debug
	foundDrive.HDCapacity = segment.substr(segment.find(") - ") + 4, string::npos) + secondMarker.substr(0,3);
	//MAKE TRY CATCH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//~ foundDrive.HDCapacity = std::atoi(capture.c_str());
	//~ cout << "Drive Capacity:  " << foundDrive.HDCapacity << endl;//debug
	segment = segment.erase(segment.find(") - "), string::npos);
	//~ cout << "Bit off the end:\n" << segment << endl;//debug
	
	foundDrive.HDSerial = segment.substr(segment.find("(S/N: ") + 6, string::npos);
	segment = segment.erase(segment.find(" (S/N: "), string::npos);
	//~ if (segment.find("(S/N: ") < string::npos){
		//~ foundDrive.HDSerial = segment.substr(segment.find("(S/N: ") + 6, string::npos);
		//~ segment = segment.erase(segment.find(" (S/N: "), string::npos);
	//~ }
	//~ else {
		//~ foundDrive.HDSerial = "notfound";
	//~ }
	//~ cout << "Bit off the end:\n" << segment << endl;//debug
	segment = segment.erase(0,segment.find(" Disk")+5);
	segment = segment.erase(0,segment.find(" ")+1);
	//~ cout << "Bit off the front:\n" << segment << endl;//debug
	foundDrive.HDProtocol = segment.substr(0,segment.find(" "));
	//~ cout << "Protocol:  " << foundDrive.HDProtocol << endl;//debug
	segment = segment.erase(0,foundDrive.HDProtocol.length()+1);
	//~ cout << "Bit off the front:\n" << segment << "!" << endl;//debug
	
	//MAKE THIS AN IF FOR DIFFERENT ATTACHMENTS--------PATA-----FIBRE-----ETC
	if (segment.find("SCSI Disk Device")< string::npos){
		foundDrive.HDAttatchment = segment.substr(segment.find("SCSI Disk Device"),string::npos);
		segment = segment.erase(segment.find(" SCSI Disk Device"),string::npos - segment.find(" SCSI Disk Device"));
		foundDrive.HDModel = segment;
	}
	else {
		if (segment.find("    ")< string::npos){
			segment = segment.erase(segment.find("    "),4);}
		foundDrive.HDModel = segment;
		foundDrive.HDAttatchment = "UNK";
	}
	
	if ((session2Drives.find("SANITIZING FAILED") < session2Drives.find("Finished ")) && (session2Drives.find("SANITIZING FAILED") < string::npos)){
		//~ cout << "ZOMG!" << endl;//debug
		segment = session2Drives.substr(0, session2Drives.find("SANITIZING FAILED") + 37);
		segment = segment.erase(0,segment.find("Started: "));
		foundDrive.WipeStart = segment.substr(9,segment.find("\n")-9);
		//~ cout << "Wipe Start:  " << foundDrive.WipeStart << endl;//debug
		segment = segment.erase(0,segment.find("\n")+1);
		foundDrive.WipeFinished = segment.substr(segment.find("   SANITIZING FAILED ") + 21, string::npos);
		//~ cout << "Wipe End:  " << foundDrive.WipeFinished << endl;
		segment = segment.erase(segment.find("   SANITIZING FAILED ")-1,string::npos);
		foundDrive.WipeStatus = "FAILED";
		session2Drives = session2Drives.erase(0,session2Drives.find("   SANITIZING FAILED ") + 21);
	}
	else if ((session2Drives.find("%\n   Stopped ") < session2Drives.find("Finished ")) && (session2Drives.find("%\n   Stopped ") < string::npos)){
		//~ cout << "OH NOES!" << endl;//debug
		//"%\n   Stopped "
		segment = session2Drives.substr(0, session2Drives.find("%\n   Stopped ") + 32);
		segment = segment.erase(0,segment.find("Started: "));
		foundDrive.WipeStart = segment.substr(9,segment.find("\n")-9);
		//~ cout << "Wipe Start:  " << foundDrive.WipeStart << endl;//debug
		segment = segment.erase(0,segment.find("\n")+1);
		foundDrive.WipeFinished = segment.substr(segment.find("%\n   Stopped ") + 13, string::npos);
		//~ cout << "Wipe End:  " << foundDrive.WipeFinished << endl;//debug
		segment = segment.erase(segment.find("%\n   Stopped ")+1,string::npos);
		foundDrive.WipeStatus = "STOPPED";
		session2Drives = session2Drives.erase(0,session2Drives.find("%\n   Stopped ") + 14);
		//~ cout << "Session Data:" << session2Drives << endl; //debug
	}
	else if (session2Drives.find("   Finished ") != string::npos){
		//~ cout << "WORD YO!" << endl;//debug
		segment = session2Drives.substr(0, session2Drives.find("Finished ") + 28);
		segment = segment.erase(0,segment.find("Started: "));
		foundDrive.WipeStart = segment.substr(9,segment.find("\n")-9);
		//~ cout << "Wipe Start:  " << foundDrive.WipeStart << endl;//debug
		segment = segment.erase(0,segment.find("\n")+1);
		foundDrive.WipeFinished = segment.substr(segment.find("   Finished ") + 12, string::npos);
		//~ cout << "Wipe End:  " << foundDrive.WipeFinished << endl;//debug
		segment = segment.erase(segment.find("   Finished ")-1,string::npos);
		foundDrive.WipeStatus = "PASSED";
		session2Drives = session2Drives.erase(0,session2Drives.find("   Finished ") + 12);
	}
	else foundDrive.WipeStatus = "UNKNOWN";//~ cout << "AWE SNAP!" << endl;//debug
	//~ cout << "Wipe Result:  " << foundDrive.WipeStatus << endl;//debug
	int woohoo = upLoad(foundDrive);//append to file or send to DB
//end if}
	cout << endl;
	return session2Drives;
}//end parseDrives()

int Parser::upLoad(DriveData spitWad){
	//open out file
	ofstream byebye;
	//~ cout << "OTAY GO!" << endl;
	string MyOutput = "C:\\\\KDDATA\\killdata.csv";
	//~ string MyOutput = "C:\\\\KDDATA\\" + fileOut;
	byebye.open(MyOutput.c_str(),std::ios::out | std::ios::app);
	if (byebye.fail())
        throw std::ios_base::failure(std::strerror(errno));
	//put data in comma separated string
	string outData;
	outData = spitWad.HDSerial + "," + spitWad.WipeReport + "," + spitWad.HDModel + "," + spitWad.HDCapacity + "," + spitWad.HDProtocol + "," + spitWad.HDAttatchment + "," + spitWad.EraseMethod + "," + spitWad.WipeStart + "," + spitWad.WipeFinished + "," + spitWad.SessionStart + "," + spitWad.SessionEnded + "," + spitWad.WipeStatus;
	cout << outData << "!\n";
	//append to CSV file
    byebye << outData.c_str() << std::endl;
	byebye.close();
	//if good send 0 and if not send 1 for error
	return 0;
}

//######################################################################
int main(int argc, char **argv){//#################MAIN#################
	HANDLE hFind;
    WIN32_FIND_DATA FindData;
	int DerpInt;
	string target;
	
	//change target to fill in from command line data
	if (argc < 2)
	{
		target = "C:\\Users\\aross\\Documents\\KillDisk\\";
		cout << "\tThis app parses KillDisk .log files to insert into a database!" << endl;
		cout << "\tAdd the target directory by typing the path with double backslashes instead of single." << endl;
		cout << "\tExample: C:\\\\Users\\\\aross\\\\Documents\\\\KillDisk\\\\" << endl;
	}//if just launched
	else
	{
		target = argv[1];
		cout << "testing command line arguments:  " << target << endl;
	}

    // Find the first file
    Parser Picard;
    hFind = FindFirstFile(string(target + "*.log").c_str(), &FindData);
    DerpInt = Picard.start_Parser(string(FindData.cFileName), string(target));
    cout << "doin it " << DerpInt << endl;//error checking

    // Look for more
    while (FindNextFile(hFind, &FindData))
    {
		Parser Ryker;//make it so Number One!
		DerpInt = Ryker.start_Parser(string(FindData.cFileName), string(target));
		cout << "doin it " << DerpInt << endl;//error checking
    }
    FindClose(hFind);
	return 0;
}//end main()

