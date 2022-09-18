/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Timothy Joseph
	UIN: 330000565
	Date: 9/17/2022
*/
#include "common.h"
#include <sys/wait.h>

#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m_ = MAX_MESSAGE; // 256 bytes
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m_ = atoi (optarg);
				break;
		}
	}

	//child process for server
	int fork_ = fork();
	if (fork_ == 0)
	{
		//Convert m_ from int to to c string
		std::string _m = std::to_string(m_);
		char* __m = new char[_m.size()];
		for (size_t i = 0; i < _m.size(); i++) {
			__m[i] = _m[i];
		}

		//run server
		char *const args[] = {(char*)"./server", (char*)"-m", __m, NULL};
		execvp(args[0], args);
		return 0;
	}

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	char buf[MAX_MESSAGE];
	double reply;
	
	// data point request
	if (t != -1) {
		datamsg x(p, t, e); 
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	// 1000 data points request
	else if (p != -1) {
		ofstream file;
		file.open("./received/x1.csv");
		if (!file.is_open()) throw std::invalid_argument("Cannot open file"); //check if file opened corrctly
		for (size_t i = 0; i < 1000; i++) {
			file << 0.004*i;
			for (size_t j = 1; j < 3; j++)
			{
				datamsg x(p, 0.004*i, j);
				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				chan.cread(&reply, sizeof(double)); //answer
				file << "," << reply;
			}
			file << endl;
		}
	}


    // file request
	
	//get file size from server
	else {
		filemsg fm(0, 0);
		string fname = filename;

		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());

		chan.cwrite(buf2, len);  // I want the file length;
		int64_t filesize;
		chan.cread(&filesize, sizeof(int64_t));

		cout << filesize << endl;

		char* buf3 = new char[m_];
		filemsg* file_req = (filemsg*)buf2;
		ofstream file;
		file.open("./received/" + filename);
		if (!file.is_open()) throw std::invalid_argument("Cannot open file"); //check if file opened corrctly
		int64_t i;


		//get file contents
		for (i = 0; i < filesize/m_; i++) {
			file_req->offset = m_ * i;
			file_req->length = m_;
			chan.cwrite(buf2, len);
			chan.cread(buf3, m_);

			//write into file
			for (int j = 0; j < m_; j++) file << buf3[j];
		
		}

		int remainder = filesize % m_;
		if (remainder > 0) {
			file_req->offset = m_ * i;
			file_req->length = remainder;
			chan.cwrite(buf2, len);
			chan.cread(buf3, remainder);

			for (int j = 0; j < m_; j++) file << buf3[j];
		}

		delete[] buf3;
		delete[] buf2;

	}

	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
